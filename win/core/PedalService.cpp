#include "PedalService.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <utility>

#include "Config.h"
#include "KeySender.h"
#include "Logger.h"
#include "SerialPort.h"


namespace core {

PedalService::PedalService(std::string configPath) : configPath(std::move(configPath)) {}

std::string PedalService::toEventKey(const std::string& line) {
    std::string result = line;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    std::replace(result.begin(), result.end(), '-', '_');
    return result;
}

void PedalService::run(const std::string& portName) {
    SerialPort serialPort;

    if (!serialPort.connect(portName)) {
        LOG_ERROR << "На порту " << portName << " невозможно подключиться к Arduino.";
        return;
    }

    LOG_INFO << "Подключен к Arduino на порту: " << portName;

    while (serialPort.isConnected()) {
        std::string line;
        uint8_t byte = 0;
        DWORD bytesRead = 0;

        while (serialPort.isConnected()) {
            if (serialPort.readData(&byte, 1, &bytesRead) && bytesRead == 1) {
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

        LOG_DEBUG << "Пакет от Arduino: " << line;

        Config config;
        if (!config.load(configPath)) {
            LOG_WARNING << "Не удалось перечитать конфиг";
            continue;
        }

        const std::string eventKey = toEventKey(line);
        const auto keys = config.getKeys(eventKey);
        if (keys.empty()) {
            LOG_WARNING << "Неизвестное событие или нет привязки: " << line;
            continue;
        }

        LOG_INFO << "Отправка клавиш: " << KeySender::describe(keys);
        KeySender::send(keys);
    }

    LOG_INFO << "Разорвано соединение со стороны Arduino";
}

}
