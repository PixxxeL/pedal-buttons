#include "ConsoleFrontend.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "../core/PortEnumerator.h"


namespace ui {

namespace {

typedef int (WINAPI *MessageBoxTimeoutProc)(HWND, LPCSTR, LPCSTR, UINT, WORD, DWORD);

constexpr DWORD messageTimeoutMs = 3000;

std::atomic<bool> messageOpen{false};
std::atomic<bool> messageCooldown{false};

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
    if (messageOpen.load() || messageCooldown.load()) {
        return;
    }

    HMODULE user32 = LoadLibraryA("user32.dll");
    if (!user32) {
        return;
    }

    auto messageBoxTimeout = reinterpret_cast<MessageBoxTimeoutProc>(
        GetProcAddress(user32, "MessageBoxTimeoutA"));
    if (!messageBoxTimeout) {
        FreeLibrary(user32);
        return;
    }

    messageOpen.store(true);
    messageCooldown.store(true);

    std::thread([] {
        std::this_thread::sleep_for(std::chrono::milliseconds(messageTimeoutMs));
        messageCooldown.store(false);
    }).detach();

    messageBoxTimeout(NULL, text.c_str(), "Pedal Buttons", MB_OK | MB_ICONERROR, 0, 0);

    messageOpen.store(false);
    FreeLibrary(user32);
}

}
