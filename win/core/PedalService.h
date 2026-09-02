#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <string>

#include "Config.h"


namespace core {

class PedalService {
public:
    explicit PedalService(std::string configPath);

    bool loadConfig();
    void run(const std::string& portName);
    void stop();

private:
    static std::string toEventKey(const std::string& line);

    void reloadConfigIfChanged();
    void handleLine(const std::string& line);

    std::string configPath;
    Config config;
    std::filesystem::file_time_type configWriteTime;
    std::chrono::steady_clock::time_point lastConfigCheck;
    std::atomic<bool> running;
};

}
