#include "SerialPort.h"


SerialPort::SerialPort() : hSerial(INVALID_HANDLE_VALUE), connected(false) {}

SerialPort::~SerialPort() {
    disconnect();
}

bool SerialPort::connect(const std::string& portName, DWORD baudRate) { //CBR_115200
    std::string fullPortName = "\\\\.\\" + portName;

    hSerial = CreateFileA(fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        CloseHandle(hSerial);
        return false;
    }

    connected = true;
    return true;
}

void SerialPort::disconnect() {
    if (connected) {
        CloseHandle(hSerial);
        connected = false;
    }
}

bool SerialPort::writeData(const uint8_t* data, DWORD length) const {
    if (!connected) {
        return false;
    }
    DWORD bytesWritten;
    return WriteFile(hSerial, data, length, &bytesWritten, NULL);
}

bool SerialPort::readData(uint8_t* buffer, DWORD length, DWORD* bytesRead) const {
    if (!connected || buffer == nullptr || bytesRead == nullptr || hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }
    return ReadFile(hSerial, buffer, length, bytesRead, NULL);
}

bool SerialPort::isConnected() const {
    return connected;
}
