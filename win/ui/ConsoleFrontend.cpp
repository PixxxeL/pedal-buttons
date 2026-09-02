#include "ConsoleFrontend.h"

#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

#include "../core/PortEnumerator.h"


namespace ui {

namespace {

std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return L"";
    }

    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        result.data(), length);
    return result;
}

}

void printPortsList(int maxPort) {
    std::cout << "Доступные [x] COM-порты:" << std::endl;

    const auto activePorts = core::listPortsFromRegistry();

    for (int i = 1; i <= maxPort; i++) {
        const std::string portName = "COM" + std::to_string(i);
        if (activePorts.count(portName)) {
            std::cout << "[x] " << portName << std::endl;
        }
        else {
            std::cout << "[ ] " << portName << std::endl;
        }
    }
}

void showFatalMessage(const std::string& text) {
    MessageBoxW(NULL, toWide(text).c_str(), L"Pedal Buttons",
        MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

}
