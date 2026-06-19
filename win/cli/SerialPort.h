#pragma once

#include <windows.h>
#include <string>


class SerialPort {
private:
    HANDLE hSerial;
    bool connected;

public:
    SerialPort();
    ~SerialPort();
    bool connect(const std::string& portName, DWORD baudRate = CBR_9600); //CBR_115200
    void disconnect();
    bool writeData(const uint8_t* data, DWORD length) const;
    bool readData(uint8_t* buffer, DWORD length, DWORD* bytesRead) const;
    bool isConnected() const;
};

