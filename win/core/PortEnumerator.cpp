#include "PortEnumerator.h"

#include <windows.h>

#include <string>


namespace core {

std::set<std::string> listPortsFromRegistry() {
    std::set<std::string> active;

    HKEY key = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return active;
    }

    char valueName[256];
    char valueData[256];
    DWORD index = 0;

    while (true) {
        DWORD valueNameSize = sizeof(valueName);
        DWORD valueDataSize = sizeof(valueData);
        DWORD type = 0;

        if (RegEnumValueA(key, index++, valueName, &valueNameSize, NULL, &type,
                reinterpret_cast<LPBYTE>(valueData), &valueDataSize) != ERROR_SUCCESS) {
            break;
        }

        if (type == REG_SZ) {
            active.insert(std::string(valueData, valueDataSize > 0 ? valueDataSize - 1 : 0));
        }
    }

    RegCloseKey(key);
    return active;
}

std::set<std::string> probePorts(int maxPort) {
    std::set<std::string> active;

    for (int i = 1; i <= maxPort; i++) {
        const std::string portName = "COM" + std::to_string(i);
        const std::string fullPath = "\\\\.\\" + portName;

        HANDLE handle = CreateFileA(fullPath.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            active.insert(portName);
        }
    }

    return active;
}

}
