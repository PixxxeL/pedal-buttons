#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <boost/program_options.hpp>
#include "SerialPortManager.h"
#include "SerialPort.h"
#include "Config.h"
#include "KeySender.h"
#include "logger.h"

namespace fs = std::filesystem;
namespace po = boost::program_options;

void SerialPortManager::readFormPort(std::string portName) {
    SerialPort port;
    DWORD bytesRead = 0;
    uint8_t byte;
    if (port.connect(portName)) {
        std::cout << "Подключен к Arduino на порту: " << portName << std::endl;
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
            std::cout << "\rПолучен ответ от Arduino: " << line << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        std::cout << "\nРазорвано соединение со стороны Arduino" << std::endl;
    }
    else {
        std::cout << "На порту " << portName << " невозможно подключиться к Arduino." << std::endl;
        std::cout << "Попробуйте установить другой порт параметром -p" << std::endl;
        std::cout << "и подключить Arduino." << std::endl;
    }
}

std::string SerialPortManager::printPortsList(int maxPort) {
    std::cout << "Доступные [x] COM-порты:" << std::endl;
    for (int i = 1; i <= maxPort; i++) {
        std::string portName = "COM" + std::to_string(i);
        SerialPort testedPort;
        if (testedPort.connect(portName)) {
            std::cout << "[x] " << portName << std::endl;
            testedPort.disconnect();
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
        std::cerr << "Файл конфигурации не найден: " << userPath << std::endl;
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

    std::cerr << "Файл конфигурации не найден ни рядом с exe (" << exeDir << "),\n"
              << "ни в рабочей папке (" << fs::current_path() << ")" << std::endl;
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
        std::cerr << "Конфигурация не загружена. Работа невозможна." << std::endl;
        return;
    }

    std::cout << "Загружен конфиг: " << iniFile << ", приложение: " << config.getAppName() << std::endl;
    SerialPortManager::readFormPort("COM" + std::to_string(SerialPortManager::port));
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
        std::cerr << "Ошибка: " << e.what() << std::endl;
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
