#include "Autostart.h"

#include <windows.h>

#include <string>
#include <vector>

#include "../core/Logger.h"


namespace ui {

namespace {

const wchar_t* runKeyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const wchar_t* valueName = L"PedalButtons";

std::wstring executablePath() {
    std::vector<wchar_t> buffer(MAX_PATH);
    DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (length == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buffer.resize(buffer.size() * 2);
        length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }
    return std::wstring(buffer.data(), length);
}

std::wstring commandLine() {
    return L"\"" + executablePath() + L"\"";
}

}

bool isAutostartEnabled() {
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, runKeyPath, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }

    wchar_t stored[1024] = { 0 };
    DWORD size = sizeof(stored) - sizeof(wchar_t);
    DWORD type = 0;
    const LSTATUS status = RegQueryValueExW(key, valueName, nullptr, &type,
        reinterpret_cast<LPBYTE>(stored), &size);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return false;
    }

    return std::wstring(stored) == commandLine();
}

bool setAutostartEnabled(bool enabled) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, runKeyPath, 0, nullptr, 0,
            KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        LOG_ERROR << "Не удалось открыть ключ автозапуска в реестре";
        return false;
    }

    LSTATUS status = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring command = commandLine();
        status = RegSetValueExW(key, valueName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    }
    else {
        status = RegDeleteValueW(key, valueName);
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }

    RegCloseKey(key);

    if (status != ERROR_SUCCESS) {
        LOG_ERROR << "Не удалось изменить автозапуск, код " << status;
        return false;
    }

    LOG_INFO << (enabled ? "Автозапуск включён" : "Автозапуск выключен");
    return true;
}

}
