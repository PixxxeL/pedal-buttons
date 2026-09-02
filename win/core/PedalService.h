#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>

#include "Config.h"
#include "EventQueue.h"
#include "PedalEvent.h"


namespace core {

class SerialPort;

class PedalService {
public:
    explicit PedalService(std::string configPath);
    ~PedalService();

    PedalService(const PedalService&) = delete;
    PedalService& operator=(const PedalService&) = delete;

    bool loadConfig();

    void setNotifier(std::function<void()> notifier);
    void setAutoReconnect(bool enabled);
    bool autoReconnectEnabled() const;
    void setPaused(bool value);
    bool isPaused() const;

    void start(const std::string& portName);
    void stop();
    void join();
    void restart(const std::string& portName);

    bool isRunning() const;
    bool isConnected() const;
    const std::string& portName() const;

    EventQueue<PedalEvent>& events();

private:
    static std::string toEventKey(const std::string& line);

    void workerLoop();
    bool runSession(SerialPort& serialPort);
    void reloadConfigIfChanged();
    void handleLine(const std::string& line);
    void emit(PedalEvent event);
    bool sleepWhileRunning(std::chrono::milliseconds duration);

    std::string configPath;
    std::string port;
    Config config;
    std::filesystem::file_time_type configWriteTime;
    std::chrono::steady_clock::time_point lastConfigCheck;

    std::atomic<bool> running;
    std::atomic<bool> connected;
    std::atomic<bool> autoReconnect;
    std::atomic<bool> paused;
    std::function<void()> notifier;
    std::thread worker;
    EventQueue<PedalEvent> queue;
};

}
