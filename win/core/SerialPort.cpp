#include "SerialPort.h"

#include <windows.h>


namespace core {

namespace {

std::string describeError(DWORD code) {
    std::string text = "код " + std::to_string(code);
    switch (code) {
        case ERROR_FILE_NOT_FOUND:
            return text + " — порт не существует";
        case ERROR_ACCESS_DENIED:
            return text + " — порт занят другим процессом";
        case ERROR_OPERATION_ABORTED:
            return text + " — операция прервана, устройство отключено";
        case ERROR_DEVICE_NOT_CONNECTED:
        case ERROR_DEV_NOT_EXIST:
            return text + " — устройство отключено";
        default:
            return text;
    }
}

}

struct SerialPort::Impl {
    HANDLE handle = INVALID_HANDLE_VALUE;
    bool connected = false;
    std::string lastError;
};

SerialPort::SerialPort() : impl(std::make_unique<Impl>()) {}

SerialPort::~SerialPort() {
    disconnect();
}

bool SerialPort::connect(const std::string& portName, unsigned int baudRate) {
    disconnect();
    impl->lastError.clear();

    const std::string fullPortName = "\\\\.\\" + portName;

    impl->handle = CreateFileA(fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (impl->handle == INVALID_HANDLE_VALUE) {
        impl->lastError = describeError(GetLastError());
        return false;
    }

    DCB parameters = { 0 };
    parameters.DCBlength = sizeof(parameters);

    if (!GetCommState(impl->handle, &parameters)) {
        impl->lastError = describeError(GetLastError());
        CloseHandle(impl->handle);
        impl->handle = INVALID_HANDLE_VALUE;
        return false;
    }

    parameters.BaudRate = static_cast<DWORD>(baudRate);
    parameters.ByteSize = 8;
    parameters.StopBits = ONESTOPBIT;
    parameters.Parity = NOPARITY;

    if (!SetCommState(impl->handle, &parameters)) {
        impl->lastError = describeError(GetLastError());
        CloseHandle(impl->handle);
        impl->handle = INVALID_HANDLE_VALUE;
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 100;
    if (!SetCommTimeouts(impl->handle, &timeouts)) {
        impl->lastError = describeError(GetLastError());
        CloseHandle(impl->handle);
        impl->handle = INVALID_HANDLE_VALUE;
        return false;
    }

    PurgeComm(impl->handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    impl->connected = true;
    return true;
}

void SerialPort::disconnect() {
    if (impl->handle != INVALID_HANDLE_VALUE) {
        CloseHandle(impl->handle);
        impl->handle = INVALID_HANDLE_VALUE;
    }
    impl->connected = false;
}

ReadResult SerialPort::read(uint8_t* buffer, std::size_t size, std::size_t& bytesRead) {
    bytesRead = 0;

    if (!impl->connected || buffer == nullptr || size == 0) {
        return ReadResult::Error;
    }

    DWORD received = 0;
    if (!ReadFile(impl->handle, buffer, static_cast<DWORD>(size), &received, NULL)) {
        impl->lastError = describeError(GetLastError());
        impl->connected = false;
        return ReadResult::Error;
    }

    if (received == 0) {
        DWORD errors = 0;
        COMSTAT status = { 0 };
        if (!ClearCommError(impl->handle, &errors, &status)) {
            impl->lastError = describeError(GetLastError());
            impl->connected = false;
            return ReadResult::Error;
        }
        return ReadResult::Timeout;
    }

    bytesRead = received;
    return ReadResult::Data;
}

bool SerialPort::write(const uint8_t* data, std::size_t size) {
    if (!impl->connected) {
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(impl->handle, data, static_cast<DWORD>(size), &written, NULL)) {
        impl->lastError = describeError(GetLastError());
        impl->connected = false;
        return false;
    }
    return written == size;
}

bool SerialPort::isConnected() const {
    return impl->connected;
}

const std::string& SerialPort::lastError() const {
    return impl->lastError;
}

}
