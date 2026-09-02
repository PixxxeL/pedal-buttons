#include "Logger.h"

#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <system_error>


namespace fs = std::filesystem;

namespace core {

const char* levelName(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "trace";
        case LogLevel::Debug: return "debug";
        case LogLevel::Info: return "info";
        case LogLevel::Warning: return "warning";
        case LogLevel::Error: return "error";
        case LogLevel::Fatal: return "fatal";
    }
    return "unknown";
}

std::string formatTime(std::chrono::system_clock::time_point time) {
    const auto asTimeT = std::chrono::system_clock::to_time_t(time);
    const auto sinceEpoch = time.time_since_epoch();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(sinceEpoch).count() % 1000;

    std::tm parts = {};
    localtime_s(&parts, &asTimeT);

    std::ostringstream out;
    out << std::put_time(&parts, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << millis;
    return out.str();
}

std::string formatRecord(const LogRecord& record) {
    return formatTime(record.time) + " [" + levelName(record.level) + "] " + record.message;
}

void LogSink::setMinLevel(LogLevel level) {
    minLevel = level;
}

bool LogSink::accepts(LogLevel level) const {
    return level >= minLevel;
}

void ConsoleSink::write(const LogRecord& record) {
    std::ostream& out = record.level >= LogLevel::Error ? std::cerr : std::cout;
    out << formatRecord(record) << std::endl;
}

FileSink::FileSink(std::string path, std::uintmax_t rotationSize)
    : path(std::move(path)), rotationSize(rotationSize) {
    std::error_code error;
    const fs::path parent = fs::path(this->path).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent, error);
    }
    stream.open(this->path, std::ios::out | std::ios::app);
}

bool FileSink::isOpen() const {
    return stream.is_open();
}

void FileSink::rotateIfNeeded() {
    if (!stream.is_open() || rotationSize == 0) {
        return;
    }
    if (static_cast<std::uintmax_t>(stream.tellp()) < rotationSize) {
        return;
    }

    stream.close();

    std::error_code error;
    fs::rename(path, path + ".1", error);
    stream.open(path, std::ios::out | std::ios::trunc);
}

void FileSink::write(const LogRecord& record) {
    if (!stream.is_open()) {
        return;
    }
    stream << formatRecord(record) << std::endl;
    rotateIfNeeded();
}

MemorySink::MemorySink(std::size_t capacity) : capacity(capacity) {}

void MemorySink::write(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(mutex);
    records.push_back(record);
    while (records.size() > capacity) {
        records.pop_front();
    }
}

std::vector<LogRecord> MemorySink::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::vector<LogRecord>(records.begin(), records.end());
}

void MemorySink::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    records.clear();
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::addSink(std::shared_ptr<LogSink> sink) {
    if (!sink) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex);
    sinks.push_back(std::move(sink));
}

void Logger::removeSink(const std::shared_ptr<LogSink>& sink) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto it = sinks.begin(); it != sinks.end(); ++it) {
        if (*it == sink) {
            sinks.erase(it);
            return;
        }
    }
}

void Logger::clearSinks() {
    std::lock_guard<std::mutex> lock(mutex);
    sinks.clear();
}

void Logger::write(LogLevel level, std::string message) {
    LogRecord record{level, std::chrono::system_clock::now(), std::move(message)};

    std::lock_guard<std::mutex> lock(mutex);
    for (const auto& sink : sinks) {
        if (sink->accepts(record.level)) {
            sink->write(record);
        }
    }
}

LogStream::LogStream(LogLevel level) : level(level) {}

LogStream::~LogStream() {
    Logger::instance().write(level, stream.str());
}

}
