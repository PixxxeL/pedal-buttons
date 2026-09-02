#include "PortEnumerator.h"

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

#include <algorithm>
#include <cstdlib>
#include <vector>


namespace core {

namespace {

std::string toUtf8(const std::wstring& text) {
    if (text.empty()) {
        return "";
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        return "";
    }

    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        result.data(), length, nullptr, nullptr);
    return result;
}

std::wstring readProperty(HDEVINFO devices, SP_DEVINFO_DATA& info, DWORD property) {
    DWORD requiredSize = 0;
    SetupDiGetDeviceRegistryPropertyW(devices, &info, property, nullptr, nullptr, 0, &requiredSize);
    if (requiredSize == 0) {
        return L"";
    }

    std::vector<BYTE> buffer(requiredSize + sizeof(wchar_t), 0);
    if (!SetupDiGetDeviceRegistryPropertyW(devices, &info, property, nullptr,
            buffer.data(), requiredSize, nullptr)) {
        return L"";
    }

    return std::wstring(reinterpret_cast<const wchar_t*>(buffer.data()));
}

std::wstring readPortName(HDEVINFO devices, SP_DEVINFO_DATA& info) {
    const HKEY key = SetupDiOpenDevRegKey(devices, &info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE) {
        return L"";
    }

    wchar_t value[64] = { 0 };
    DWORD size = sizeof(value) - sizeof(wchar_t);
    DWORD type = 0;

    const LSTATUS status = RegQueryValueExW(key, L"PortName", nullptr, &type,
        reinterpret_cast<LPBYTE>(value), &size);
    RegCloseKey(key);

    if (status != ERROR_SUCCESS || type != REG_SZ) {
        return L"";
    }
    return std::wstring(value);
}

void stripPortSuffix(std::string& description, const std::string& portName) {
    const std::string suffix = " (" + portName + ")";
    if (description.size() > suffix.size() &&
        description.compare(description.size() - suffix.size(), suffix.size(), suffix) == 0) {
        description.erase(description.size() - suffix.size());
    }
}

int portNumber(const std::string& name) {
    if (name.rfind("COM", 0) != 0) {
        return 0;
    }
    return std::atoi(name.c_str() + 3);
}

}

std::vector<PortInfo> listPorts() {
    std::vector<PortInfo> ports;

    const HDEVINFO devices = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (devices == INVALID_HANDLE_VALUE) {
        return ports;
    }

    SP_DEVINFO_DATA info = { 0 };
    info.cbSize = sizeof(info);

    for (DWORD index = 0; SetupDiEnumDeviceInfo(devices, index, &info); index++) {
        const std::string name = toUtf8(readPortName(devices, info));
        if (name.rfind("COM", 0) != 0) {
            continue;
        }

        std::wstring friendlyName = readProperty(devices, info, SPDRP_FRIENDLYNAME);
        if (friendlyName.empty()) {
            friendlyName = readProperty(devices, info, SPDRP_DEVICEDESC);
        }

        PortInfo port;
        port.name = name;
        port.description = toUtf8(friendlyName);
        port.hardwareId = toUtf8(readProperty(devices, info, SPDRP_HARDWAREID));

        stripPortSuffix(port.description, port.name);
        if (port.description.empty()) {
            port.description = "устройство без имени";
        }

        ports.push_back(std::move(port));
    }

    SetupDiDestroyDeviceInfoList(devices);

    std::sort(ports.begin(), ports.end(), [](const PortInfo& left, const PortInfo& right) {
        return portNumber(left.name) < portNumber(right.name);
    });

    return ports;
}

}
