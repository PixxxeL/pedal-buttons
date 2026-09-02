#include "PedalService.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <system_error>
#include <utility>
#include <vector>

#include "KeySender.h"
#include "LineAssembler.h"
#include "Logger.h"
#include "SerialPort.h"


namespace fs = std::filesystem;

namespace core {

namespace {

constexpr std::size_t readBufferSize = 256;
constexpr std::chrono::milliseconds configCheckInterval{1000};

}

PedalService::PedalService(std::string configPath)
    : configPath(std::move(configPath)), running(false) {}

bool PedalService::loadConfig() {
    if (!config.load(configPath)) {
        return false;
    }

    std::error_code error;
    configWriteTime = fs::last_write_time(configPath, error);
    lastConfigCheck = std::chrono::steady_clock::now();

    LOG_INFO << "Загружен конфиг: " << configPath << ", приложение: " << config.getAppName();
    return true;
}

void PedalService::reloadConfigIfChanged() {
    const auto now = std::chrono::steady_clock::now();
    if (now - lastConfigCheck < configCheckInterval) {
        return;
    }
    lastConfigCheck = now;

    std::error_code error;
    const auto writeTime = fs::last_write_time(configPath, error);
    if (error || writeTime == configWriteTime) {
        return;
    }

    Config updated;
    if (!updated.load(configPath)) {
        LOG_WARNING << "Конфиг изменился, но перечитать его не удалось. Работаем на прежнем.";
        configWriteTime = writeTime;
        return;
    }

    config = std::move(updated);
    configWriteTime = writeTime;
    LOG_INFO << "Конфиг перечитан, приложение: " << config.getAppName();
}

std::string PedalService::toEventKey(const std::string& line) {
    std::string result = line;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    std::replace(result.begin(), result.end(), '-', '_');
    return result;
}

void PedalService::handleLine(const std::string& line) {
    if (line.empty()) {
        return;
    }

    LOG_DEBUG << "Пакет от Arduino: " << line;

    const auto keys = config.getKeys(toEventKey(line));
    if (keys.empty()) {
        LOG_WARNING << "Неизвестное событие или нет привязки: " << line;
        return;
    }

    LOG_INFO << "Отправка клавиш: " << KeySender::describe(keys);
    KeySender::send(keys);
}

void PedalService::stop() {
    running = false;
}

void PedalService::run(const std::string& portName) {
    SerialPort serialPort;

    if (!serialPort.connect(portName)) {
        LOG_ERROR << "На порту " << portName << " невозможно подключиться к Arduino: "
            << serialPort.lastError();
        return;
    }

    LOG_INFO << "Подключен к Arduino на порту: " << portName;

    running = true;
    LineAssembler assembler;
    std::vector<uint8_t> buffer(readBufferSize);
    std::string line;

    while (running && serialPort.isConnected()) {
        std::size_t bytesRead = 0;
        const ReadResult result = serialPort.read(buffer.data(), buffer.size(), bytesRead);

        if (result == ReadResult::Error) {
            LOG_ERROR << "Ошибка чтения с порта " << portName << ": " << serialPort.lastError();
            break;
        }

        if (result == ReadResult::Data) {
            assembler.append(buffer.data(), bytesRead);
            while (assembler.next(line)) {
                handleLine(line);
            }
        }

        reloadConfigIfChanged();
    }

    serialPort.disconnect();
    running = false;
    LOG_INFO << "Соединение с портом " << portName << " закрыто";
}

}
