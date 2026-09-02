#include "SerialPort.h"


namespace core {

SerialPort::SerialPort() : handle(INVALID_HANDLE_VALUE), connected(false) {}

SerialPort::~SerialPort() {
    disconnect();
}

bool SerialPort::connect(const std::string& portName, DWORD baudRate) {
    const std::string fullPortName = "\\\\.\\" + portName;

    handle = CreateFileA(fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB parameters = { 0 };
    parameters.DCBlength = sizeof(parameters);

    if (!GetCommState(handle, &parameters)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return false;
    }

    parameters.BaudRate = baudRate;
    parameters.ByteSize = 8;
    parameters.StopBits = ONESTOPBIT;
    parameters.Parity = NOPARITY;

    if (!SetCommState(handle, &parameters)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 100;
    if (!SetCommTimeouts(handle, &timeouts)) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return false;
    }

    connected = true;
    return true;
}

void SerialPort::disconnect() {
    if (connected) {
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        connected = false;
    }
}

bool SerialPort::writeData(const uint8_t* data, DWORD length) const {
    if (!connected) {
        return false;
    }
    DWORD bytesWritten = 0;
    return WriteFile(handle, data, length, &bytesWritten, NULL) != FALSE;
}

bool SerialPort::readData(uint8_t* buffer, DWORD length, DWORD* bytesRead) const {
    if (!connected || buffer == nullptr || bytesRead == nullptr || handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    return ReadFile(handle, buffer, length, bytesRead, NULL) != FALSE;
}

bool SerialPort::isConnected() const {
    return connected;
}

}
