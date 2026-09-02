#include "ConsoleFrontend.h"

#include <windows.h>

#include <iostream>
#include <mutex>
#include <string>

#include "../core/PortEnumerator.h"


namespace ui {

namespace {

std::mutex interruptMutex;
std::function<void()> interruptCallback;

BOOL WINAPI consoleCtrlHandler(DWORD type) {
    if (type != CTRL_C_EVENT && type != CTRL_BREAK_EVENT &&
        type != CTRL_CLOSE_EVENT && type != CTRL_SHUTDOWN_EVENT) {
        return FALSE;
    }

    std::function<void()> callback;
    {
        std::lock_guard<std::mutex> lock(interruptMutex);
        callback = interruptCallback;
    }

    if (!callback) {
        return FALSE;
    }

    callback();
    return TRUE;
}

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

bool attachParentConsole() {
    if (GetConsoleWindow() == NULL && !AttachConsole(ATTACH_PARENT_PROCESS)) {
        return false;
    }

    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout.clear();
    std::cerr.clear();
    return true;
}

void printPortsList() {
    const auto ports = core::listPorts();

    if (ports.empty()) {
        std::cout << "COM-порты не найдены." << std::endl;
        return;
    }

    std::cout << "Доступные COM-порты:" << std::endl;
    for (const auto& port : ports) {
        std::cout << "* [" << port.name << "] " << port.description << std::endl;
    }
}

void showFatalMessage(const std::string& text) {
    MessageBoxW(NULL, toWide(text).c_str(), L"Pedal Buttons",
        MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
}

void installInterruptHandler(std::function<void()> onInterrupt) {
    {
        std::lock_guard<std::mutex> lock(interruptMutex);
        interruptCallback = std::move(onInterrupt);
    }
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
}

}
