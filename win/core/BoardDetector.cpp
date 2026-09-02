#include "BoardDetector.h"

#include <chrono>
#include <cstdint>
#include <vector>

#include "IniDocument.h"
#include "LineAssembler.h"
#include "Logger.h"
#include "PortEnumerator.h"
#include "SerialPort.h"


namespace core {

namespace {

constexpr std::chrono::milliseconds probeTimeout{2500};
constexpr std::chrono::milliseconds queryInterval{500};
constexpr std::size_t readBufferSize = 128;

bool startsWith(const std::string& text, const char* prefix) {
    return text.rfind(prefix, 0) == 0;
}

bool looksLikeUsbDevice(const PortInfo& port) {
    const std::string id = IniDocument::toLower(port.hardwareId);
    return startsWith(id, "usb") || startsWith(id, "ftdibus");
}

bool looksLikeBluetooth(const PortInfo& port) {
    return startsWith(IniDocument::toLower(port.hardwareId), "bthenum");
}

std::vector<PortInfo> orderCandidates() {
    std::vector<PortInfo> preferred;
    std::vector<PortInfo> rest;

    for (const auto& port : listPorts()) {
        if (looksLikeBluetooth(port)) {
            continue;
        }
        if (looksLikeUsbDevice(port)) {
            preferred.push_back(port);
        }
        else {
            rest.push_back(port);
        }
    }

    preferred.insert(preferred.end(), rest.begin(), rest.end());
    return preferred;
}

}

const char* BoardDetector::firmwarePrefix() {
    return "pedal-buttons";
}

BoardDetector::~BoardDetector() {
    cancel();
    if (worker.joinable()) {
        worker.join();
    }
}

void BoardDetector::start() {
    if (running) {
        return;
    }
    if (worker.joinable()) {
        worker.join();
    }

    cancelled = false;
    finished = false;
    running = true;

    {
        std::lock_guard<std::mutex> lock(mutex);
        result = DetectionResult();
        statusText = "Поиск платы...";
    }

    worker = std::thread(&BoardDetector::scan, this);
}

void BoardDetector::cancel() {
    cancelled = true;
}

bool BoardDetector::isRunning() const {
    return running;
}

std::string BoardDetector::status() const {
    std::lock_guard<std::mutex> lock(mutex);
    return statusText;
}

void BoardDetector::setStatus(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex);
    statusText = text;
}

bool BoardDetector::take(DetectionResult& value) {
    if (!finished) {
        return false;
    }
    if (worker.joinable()) {
        worker.join();
    }
    finished = false;

    std::lock_guard<std::mutex> lock(mutex);
    value = result;
    return true;
}

bool BoardDetector::probe(const std::string& portName, std::string& firmware) {
    SerialPort port;
    if (!port.connect(portName)) {
        LOG_DEBUG << "Порт " << portName << " не открылся: " << port.lastError();
        return false;
    }

    LineAssembler assembler;
    std::vector<uint8_t> buffer(readBufferSize);
    std::string line;

    const auto deadline = std::chrono::steady_clock::now() + probeTimeout;
    auto nextQuery = std::chrono::steady_clock::now();

    while (!cancelled && std::chrono::steady_clock::now() < deadline) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextQuery) {
            const uint8_t query = '?';
            port.write(&query, 1);
            nextQuery = now + queryInterval;
        }

        std::size_t bytesRead = 0;
        const ReadResult status = port.read(buffer.data(), buffer.size(), bytesRead);
        if (status == ReadResult::Error) {
            return false;
        }
        if (status != ReadResult::Data) {
            continue;
        }

        assembler.append(buffer.data(), bytesRead);
        while (assembler.next(line)) {
            if (startsWith(line, firmwarePrefix())) {
                firmware = line;
                return true;
            }
        }
    }

    return false;
}

void BoardDetector::scan() {
    const auto candidates = orderCandidates();

    if (candidates.empty()) {
        setStatus("Подходящих портов не найдено");
        LOG_WARNING << "Поиск платы: подходящих портов нет";
        running = false;
        finished = true;
        return;
    }

    LOG_INFO << "Поиск платы: проверяю " << candidates.size() << " порт(ов)";

    for (const auto& candidate : candidates) {
        if (cancelled) {
            break;
        }

        setStatus("Проверяю " + candidate.name + "...");

        std::string firmware;
        if (probe(candidate.name, firmware)) {
            LOG_INFO << "Плата найдена на " << candidate.name << ": " << firmware;
            setStatus("Плата найдена на " + candidate.name);

            std::lock_guard<std::mutex> lock(mutex);
            result.found = true;
            result.port = candidate.name;
            result.firmware = firmware;
            running = false;
            finished = true;
            return;
        }
    }

    if (cancelled) {
        setStatus("Поиск отменён");
        LOG_INFO << "Поиск платы отменён";
    }
    else {
        setStatus("Плата не найдена");
        LOG_WARNING << "Поиск платы: ни один порт не ответил приветствием";
    }

    running = false;
    finished = true;
}

}
