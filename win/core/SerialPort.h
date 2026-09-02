#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>


namespace core {

enum class ReadResult {
    Data,
    Timeout,
    Error
};

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool connect(const std::string& portName, unsigned int baudRate = 9600);
    void disconnect();

    ReadResult read(uint8_t* buffer, std::size_t size, std::size_t& bytesRead);
    bool write(const uint8_t* data, std::size_t size);

    bool isConnected() const;
    const std::string& lastError() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
