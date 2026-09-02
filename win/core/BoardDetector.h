#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>


namespace core {

struct DetectionResult {
    bool found = false;
    std::string port;
    std::string firmware;
};

class BoardDetector {
public:
    BoardDetector() = default;
    ~BoardDetector();

    BoardDetector(const BoardDetector&) = delete;
    BoardDetector& operator=(const BoardDetector&) = delete;

    static const char* firmwarePrefix();

    void start();
    void cancel();
    bool isRunning() const;
    bool take(DetectionResult& result);
    std::string status() const;

private:
    void scan();
    bool probe(const std::string& portName, std::string& firmware);
    void setStatus(const std::string& text);

    std::thread worker;
    std::atomic<bool> running{false};
    std::atomic<bool> cancelled{false};
    std::atomic<bool> finished{false};

    mutable std::mutex mutex;
    std::string statusText;
    DetectionResult result;
};

}
