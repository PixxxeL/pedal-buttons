#pragma once

#include <windows.h>

#include <cstdint>
#include <string>


namespace core {

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool connect(const std::string& portName, DWORD baudRate = CBR_9600);
    void disconnect();
    bool writeData(const uint8_t* data, DWORD length) const;
    bool readData(uint8_t* buffer, DWORD length, DWORD* bytesRead) const;
    bool isConnected() const;

private:
    HANDLE handle;
    bool connected;
};

}
