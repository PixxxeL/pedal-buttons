#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <boost/program_options.hpp>
#include <boost/log/trivial.hpp>
#include "SerialPortManager.h"
#include "SerialPort.h"
#include "Config.h"
#include "KeySender.h"
#include "logger.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;

typedef int (WINAPI *MessageBoxTimeoutA_t)(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);

class CooldownMessageBox {
private:
    static constexpr DWORD TIMEOUT_MS = 3000;
    static inline bool isOpen = false;
    static inline bool inCooldown = false;

public:
    static void show(const std::string& text) {
        if (isOpen || inCooldown) return;

        HMODULE hUser32 = LoadLibraryA("user32.dll");
        if (!hUser32) return;

        auto pMsgBox = (MessageBoxTimeoutA_t)GetProcAddress(hUser32, "MessageBoxTimeoutA");
        if (!pMsgBox) { FreeLibrary(hUser32); return; }

        isOpen = true;
        inCooldown = true;

        std::thread([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
            inCooldown = false;
        }).detach();

        pMsgBox(NULL, text.c_str(), "Pedal Buttons", MB_OK | MB_ICONERROR, 0, 0);
        isOpen = false;
        FreeLibrary(hUser32);
    }
};

static std::string toEventKey(const std::string& line) {
    std::string result = line;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::toupper(c); });
    std::replace(result.begin(), result.end(), '-', '_');
    return result;
}

void SerialPortManager::readFormPort(std::string portName, const std::string& configPath) {
    SerialPort port;
    DWORD bytesRead = 0;
    uint8_t byte;
    if (port.connect(portName)) {
        BOOST_LOG_TRIVIAL(info) << "Подключен к Arduino на порту: " << portName;
        while (port.isConnected()) {
            std::string line;
            while (port.isConnected()) {
                if (port.readData(&byte, 1, &bytesRead) && bytesRead == 1) {
                    if (byte == '\n') {
                        break;
                    }
                    if (byte != '\r') {
                        line += static_cast<char>(byte);
                    }
                }
            }
            if (line.empty()) {
                continue;
            }
            BOOST_LOG_TRIVIAL(debug) << "Пакет от Arduino: " << line;

            Config config;
            if (!config.load(configPath)) {
                BOOST_LOG_TRIVIAL(warning) << "Не удалось перечитать конфиг";
                continue;
            }

            std::string eventKey = toEventKey(line);
            auto keys = config.getKeys(eventKey);
            if (!keys.empty()) {
                std::string keysStr;
                for (size_t i = 0; i < keys.size(); i++) {
                    if (i > 0) keysStr += "+";
                    keysStr += keys[i];
                }
                BOOST_LOG_TRIVIAL(info) << "Отправка клавиш: " << keysStr;
                KeySender::send(keys);
            } else {
                BOOST_LOG_TRIVIAL(warning) << "Неизвестное событие или нет привязки: " << line;
            }
        }
        BOOST_LOG_TRIVIAL(info) << "Разорвано соединение со стороны Arduino";
    }
    else {
        BOOST_LOG_TRIVIAL(error) << "На порту " << portName << " невозможно подключиться к Arduino.";
    }
}

static std::set<std::string> getActivePortsViaCreateFile(int maxPort) {
    std::set<std::string> active;
    for (int i = 1; i <= maxPort; i++) {
        std::string portName = "COM" + std::to_string(i);
        std::string fullPath = "\\\\.\\" + portName;
        HANDLE h = CreateFileA(fullPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            active.insert(portName);
        }
    }
    return active;
}

static std::set<std::string> getActivePortsViaRegistry() {
    std::set<std::string> active;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char valueName[256];
        DWORD valueNameSize;
        char valueData[256];
        DWORD valueDataSize;
        DWORD index = 0;
        while (true) {
            valueNameSize = sizeof(valueName);
            valueDataSize = sizeof(valueData);
            DWORD type;
            if (RegEnumValueA(hKey, index++, valueName, &valueNameSize, NULL, &type, (LPBYTE)valueData, &valueDataSize) != ERROR_SUCCESS) break;
            if (type == REG_SZ) {
                std::string port(valueData, valueDataSize > 0 ? valueDataSize - 1 : 0);
                active.insert(port);
            }
        }
        RegCloseKey(hKey);
    }
    return active;
}

std::string SerialPortManager::printPortsList(int maxPort) {
    std::cout << "Доступные [x] COM-порты:" << std::endl;

    // auto activePorts = getActivePortsViaCreateFile(maxPort);
    auto activePorts = getActivePortsViaRegistry();

    for (int i = 1; i <= maxPort; i++) {
        std::string portName = "COM" + std::to_string(i);
        if (activePorts.count(portName)) {
            std::cout << "[x] " << portName << std::endl;
        }
        else {
            std::cout << "[ ] " << portName << std::endl;
        }
    }
    return "";
}

static std::string findConfigFile(const std::string& userPath) {
    if (!userPath.empty()) {
        if (fs::exists(userPath)) return userPath;
        BOOST_LOG_TRIVIAL(error) << "Файл конфигурации не найден: " << userPath;
        CooldownMessageBox::show("Файл конфигурации не найден:\n" + userPath);
        return "";
    }

    const std::string iniName = "pedal-buttons.ini";

    char exePath[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path byExe = exeDir / iniName;
    if (fs::exists(byExe)) return byExe.string();

    fs::path byCwd = fs::current_path() / iniName;
    if (fs::exists(byCwd)) return byCwd.string();

    BOOST_LOG_TRIVIAL(error) << "Файл конфигурации не найден ни рядом с exe (" << exeDir
              << "), ни в рабочей папке (" << fs::current_path() << ")";
    CooldownMessageBox::show("Файл pedal-buttons.ini не найден\nни рядом с exe, ни в рабочей папке.");
    return "";
}

void SerialPortManager::run(unsigned int argc, char** argv) {
    SerialPortManager::parseArgs(argc, argv);
    initLogging("pedal-buttons.log");

    if (SerialPortManager::isShowList) {
        SerialPortManager::printPortsList(SerialPortManager::portCount);
    }

    Config config;
    std::string iniFile = findConfigFile(SerialPortManager::configPath);
    if (iniFile.empty() || !config.load(iniFile)) {
        BOOST_LOG_TRIVIAL(error) << "Конфигурация не загружена. Работа невозможна.";
        CooldownMessageBox::show("Конфигурация не загружена.\nРабота невозможна.");
        return;
    }

    BOOST_LOG_TRIVIAL(info) << "Загружен конфиг: " << iniFile << ", приложение: " << config.getAppName();
    SerialPortManager::readFormPort("COM" + std::to_string(SerialPortManager::port), iniFile);
}

void SerialPortManager::parseArgs(unsigned int argc, char** argv) {
    po::options_description desc("Управление ножными кнопками");
    desc.add_options()
        ("help", "Показать справку")
        ("list,l", po::bool_switch(&SerialPortManager::isShowList)
            ->default_value(false), "Показать доступные сериальные порты")
        ("portCount,c", po::value<unsigned int>(&SerialPortManager::portCount)
            ->default_value(9)
            ->notifier(SerialPortManager::validatePortCount),
            "Сколько портов сканировать? (от 1 до 20)")
        ("port,p", po::value<unsigned int>(&SerialPortManager::port)
            ->default_value(9)
            ->notifier(SerialPortManager::validatePort),
            "Номер порта для подключения (от 1 до portCount)")
        ("ini,i", po::value<std::string>(&SerialPortManager::configPath)
            ->default_value(""),
            "Путь к файлу конфигурации .ini")
        ;
    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            exit(0);
        }
    }
    catch (const std::exception& e) {
        BOOST_LOG_TRIVIAL(error) << "Ошибка: " << e.what();
        std::cout << desc << std::endl;
        return;
    }
}

void SerialPortManager::validatePortCount(unsigned int value) {
    if (value < 1 || value > 20) {
        throw po::validation_error(
            po::validation_error::invalid_option_value,
            "portCount",
            std::to_string(value)
        );
    }
}

void SerialPortManager::validatePort(unsigned int value) {
    if (value < 1 || value > SerialPortManager::portCount) {
        throw po::validation_error(
            po::validation_error::invalid_option_value,
            "port",
            std::to_string(value)
        );
    }
}

bool SerialPortManager::isShowList = false;
unsigned int SerialPortManager::portCount = 9;
unsigned int SerialPortManager::port = 9;
std::string SerialPortManager::configPath = "";
