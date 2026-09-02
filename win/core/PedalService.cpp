#include "PedalService.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iterator>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "AppActivator.h"
#include "KeyBinding.h"
#include "KeySender.h"
#include "LineAssembler.h"
#include "Logger.h"
#include "SerialPort.h"


namespace fs = std::filesystem;

namespace core {

namespace {

constexpr std::size_t readBufferSize = 256;
constexpr std::chrono::milliseconds configCheckInterval{1000};
constexpr std::chrono::milliseconds sleepSlice{100};

const std::chrono::milliseconds reconnectDelays[] = {
    std::chrono::milliseconds{1000},
    std::chrono::milliseconds{2000},
    std::chrono::milliseconds{5000}
};

}

PedalService::PedalService(std::string configPath)
    : configPath(std::move(configPath)), running(false), connected(false), autoReconnect(true), paused(false) {}

PedalService::~PedalService() {
    stop();
    join();
}

bool PedalService::loadConfig() {
    if (!config.load(configPath)) {
        return false;
    }

    std::error_code error;
    configWriteTime = fs::last_write_time(configPath, error);
    lastConfigCheck = std::chrono::steady_clock::now();

    LOG_INFO << "Загружен конфиг: " << configPath << ", профиль: " << config.activeProfile();
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
    LOG_INFO << "Конфиг перечитан, профиль: " << config.activeProfile();
}

std::string PedalService::toEventKey(const std::string& line) {
    std::string result = line;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    std::replace(result.begin(), result.end(), '-', '_');
    return result;
}

void PedalService::emit(PedalEvent event) {
    queue.push(std::move(event));
    if (notifier) {
        notifier();
    }
}

void PedalService::handleLine(const std::string& line) {
    if (line.empty()) {
        return;
    }

    LOG_DEBUG << "Пакет от Arduino: " << line;

    const std::string eventKey = toEventKey(line);
    const KeySequence keys = config.binding(eventKey);

    PedalEvent event;
    event.name = eventKey;
    event.detail = formatKeySequence(keys);

    if (isEmpty(keys)) {
        LOG_WARNING << "Неизвестное событие или нет привязки: " << line;
        event.type = PedalEventType::Unknown;
        emit(std::move(event));
        return;
    }

    if (paused) {
        LOG_INFO << "Пауза, клавиши не отправлены: " << event.detail;
        event.type = PedalEventType::Pedal;
        emit(std::move(event));
        return;
    }

    const std::string rule = config.windowMatch();
    if (!rule.empty()) {
        const ActivationResult activation = activateTarget(rule, config.executablePath());

        if (activation == ActivationResult::Launched) {
            LOG_INFO << "Целевое окно не найдено, " << describeActivation(activation)
                << ". Нажми педаль ещё раз, когда оно откроется.";
            event.type = PedalEventType::Pedal;
            emit(std::move(event));
            return;
        }

        if (activation == ActivationResult::NotFound || activation == ActivationResult::Failed) {
            LOG_WARNING << "Правило \"" << rule << "\": " << describeActivation(activation)
                << ", клавиши уйдут в активное окно";
        }
        else if (activation == ActivationResult::Activated) {
            LOG_DEBUG << "Правило \"" << rule << "\": " << describeActivation(activation);
        }
    }

    KeySender::send(keys);
    LOG_INFO << "Отправка клавиш: " << event.detail;

    event.type = PedalEventType::Pedal;
    emit(std::move(event));
}

bool PedalService::sleepWhileRunning(std::chrono::milliseconds duration) {
    std::chrono::milliseconds slept{0};
    while (running && slept < duration) {
        const auto step = (std::min)(sleepSlice, duration - slept);
        std::this_thread::sleep_for(step);
        slept += step;
    }
    return running;
}

void PedalService::setNotifier(std::function<void()> value) {
    notifier = std::move(value);
}

void PedalService::setAutoReconnect(bool enabled) {
    autoReconnect = enabled;
}

bool PedalService::autoReconnectEnabled() const {
    return autoReconnect;
}

void PedalService::setPaused(bool value) {
    paused = value;
}

bool PedalService::isPaused() const {
    return paused;
}

void PedalService::start(const std::string& portName) {
    if (running) {
        return;
    }
    join();

    port = portName;
    running = true;
    worker = std::thread(&PedalService::workerLoop, this);
}

void PedalService::restart(const std::string& portName) {
    stop();
    join();
    start(portName);
}

void PedalService::stop() {
    running = false;
}

void PedalService::join() {
    if (worker.joinable()) {
        worker.join();
    }
}

bool PedalService::isRunning() const {
    return running;
}

bool PedalService::isConnected() const {
    return connected;
}

const std::string& PedalService::portName() const {
    return port;
}

EventQueue<PedalEvent>& PedalService::events() {
    return queue;
}

bool PedalService::runSession(SerialPort& serialPort) {
    LineAssembler assembler;
    std::vector<uint8_t> buffer(readBufferSize);
    std::string line;
    bool clean = true;

    while (running && serialPort.isConnected()) {
        std::size_t bytesRead = 0;
        const ReadResult result = serialPort.read(buffer.data(), buffer.size(), bytesRead);

        if (result == ReadResult::Error) {
            LOG_ERROR << "Ошибка чтения с порта " << port << ": " << serialPort.lastError();
            clean = false;
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

    return clean;
}

void PedalService::workerLoop() {
    std::size_t attempt = 0;
    std::string lastFailure;

    while (running) {
        SerialPort serialPort;

        if (serialPort.connect(port)) {
            attempt = 0;
            lastFailure.clear();

            LOG_INFO << "Подключен к Arduino на порту: " << port;
            connected = true;

            PedalEvent opened;
            opened.type = PedalEventType::Connected;
            opened.name = port;
            emit(opened);

            const bool clean = runSession(serialPort);

            serialPort.disconnect();
            connected = false;

            PedalEvent closed;
            closed.type = PedalEventType::Disconnected;
            closed.name = port;
            emit(closed);

            if (clean) {
                LOG_INFO << "Соединение с портом " << port << " закрыто";
            }
        }
        else {
            std::string failure = serialPort.lastError();
            if (failure != lastFailure) {
                LOG_ERROR << "На порту " << port << " невозможно подключиться к Arduino: " << failure;
                lastFailure = failure;
            }
            else {
                LOG_DEBUG << "Попытка подключения к " << port << " снова неудачна: " << failure;
            }

            PedalEvent problem;
            problem.type = PedalEventType::Failure;
            problem.name = port;
            problem.detail = std::move(failure);
            emit(std::move(problem));
        }

        if (!running || !autoReconnect) {
            break;
        }

        const std::size_t index = (std::min)(attempt, std::size(reconnectDelays) - 1);
        const auto delay = reconnectDelays[index];
        attempt++;

        if (attempt == 1) {
            LOG_INFO << "Переподключение к " << port << " через "
                << delay.count() / 1000 << " с";
        }

        if (!sleepWhileRunning(delay)) {
            break;
        }
    }

    connected = false;
    running = false;
}

}
