#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <fstream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>


namespace core {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

const char* levelName(LogLevel level);

struct LogRecord {
    LogLevel level;
    std::chrono::system_clock::time_point time;
    std::string message;
};

std::string formatTime(std::chrono::system_clock::time_point time);
std::string formatRecord(const LogRecord& record);

class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogRecord& record) = 0;

    void setMinLevel(LogLevel level);
    bool accepts(LogLevel level) const;

private:
    LogLevel minLevel = LogLevel::Trace;
};

class ConsoleSink : public LogSink {
public:
    void write(const LogRecord& record) override;
};

class FileSink : public LogSink {
public:
    explicit FileSink(std::string path, std::uintmax_t rotationSize = 10 * 1024 * 1024);
    void write(const LogRecord& record) override;
    bool isOpen() const;

private:
    void rotateIfNeeded();

    std::string path;
    std::uintmax_t rotationSize;
    std::ofstream stream;
};

class MemorySink : public LogSink {
public:
    explicit MemorySink(std::size_t capacity = 2000);
    void write(const LogRecord& record) override;
    std::vector<LogRecord> snapshot() const;
    std::uint64_t version() const;
    void clear();

private:
    mutable std::mutex mutex;
    std::size_t capacity;
    std::uint64_t sequence = 0;
    std::deque<LogRecord> records;
};

class Logger {
public:
    static Logger& instance();

    void addSink(std::shared_ptr<LogSink> sink);
    void removeSink(const std::shared_ptr<LogSink>& sink);
    void clearSinks();
    void write(LogLevel level, std::string message);

private:
    Logger() = default;

    mutable std::mutex mutex;
    std::vector<std::shared_ptr<LogSink>> sinks;
};

class LogStream {
public:
    explicit LogStream(LogLevel level);
    ~LogStream();

    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

    template <typename T>
    LogStream& operator<<(const T& value) {
        stream << value;
        return *this;
    }

private:
    LogLevel level;
    std::ostringstream stream;
};

}

#define LOG_TRACE core::LogStream(core::LogLevel::Trace)
#define LOG_DEBUG core::LogStream(core::LogLevel::Debug)
#define LOG_INFO core::LogStream(core::LogLevel::Info)
#define LOG_WARNING core::LogStream(core::LogLevel::Warning)
#define LOG_ERROR core::LogStream(core::LogLevel::Error)
#define LOG_FATAL core::LogStream(core::LogLevel::Fatal)
