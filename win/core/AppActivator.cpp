#include "AppActivator.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <thread>

#include "IniDocument.h"
#include "Logger.h"


namespace fs = std::filesystem;

namespace core {

namespace {

constexpr std::chrono::milliseconds foregroundTimeout{400};
constexpr std::chrono::milliseconds foregroundPollStep{10};
constexpr std::chrono::milliseconds settleDelay{40};

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

std::string lower(const std::string& text) {
    return IniDocument::toLower(text);
}

bool contains(const std::string& haystack, const std::string& needle) {
    return lower(haystack).find(lower(needle)) != std::string::npos;
}

std::string processInfo(HWND window, std::string& fullPath) {
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId == 0) {
        return "";
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (process == nullptr) {
        return "";
    }

    wchar_t buffer[MAX_PATH * 2] = { 0 };
    DWORD size = static_cast<DWORD>(std::size(buffer));
    std::string name;

    if (QueryFullProcessImageNameW(process, 0, buffer, &size) != FALSE) {
        const fs::path path(std::wstring(buffer, size));
        fullPath = toUtf8(path.wstring());
        name = toUtf8(path.filename().wstring());
    }

    CloseHandle(process);
    return name;
}

BOOL CALLBACK collectWindow(HWND window, LPARAM parameter) {
    auto* windows = reinterpret_cast<std::vector<WindowInfo>*>(parameter);

    if (IsWindowVisible(window) == FALSE || GetWindow(window, GW_OWNER) != nullptr) {
        return TRUE;
    }
    if ((GetWindowLongPtrW(window, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) != 0) {
        return TRUE;
    }

    wchar_t title[512] = { 0 };
    const int titleLength = GetWindowTextW(window, title, static_cast<int>(std::size(title)));
    if (titleLength <= 0) {
        return TRUE;
    }

    wchar_t className[256] = { 0 };
    GetClassNameW(window, className, static_cast<int>(std::size(className)));

    WindowInfo info;
    info.handle = reinterpret_cast<std::uintptr_t>(window);
    info.title = toUtf8(std::wstring(title, titleLength));
    info.className = toUtf8(std::wstring(className));
    info.process = processInfo(window, info.path);

    windows->push_back(std::move(info));
    return TRUE;
}

bool matches(const WindowInfo& window, const std::string& rule) {
    const std::string trimmed = IniDocument::trim(rule);
    if (trimmed.empty()) {
        return false;
    }

    const auto separator = trimmed.find(':');
    std::string kind = "process";
    std::string value = trimmed;

    if (separator != std::string::npos) {
        const std::string prefix = lower(trimmed.substr(0, separator));
        if (prefix == "process" || prefix == "class" || prefix == "title") {
            kind = prefix;
            value = IniDocument::trim(trimmed.substr(separator + 1));
        }
    }

    if (value.empty()) {
        return false;
    }

    if (kind == "class") {
        return lower(window.className) == lower(value);
    }
    if (kind == "title") {
        return contains(window.title, value);
    }
    return lower(window.process) == lower(value);
}

bool waitForForeground(HWND target) {
    const auto deadline = std::chrono::steady_clock::now() + foregroundTimeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (GetForegroundWindow() == target) {
            return true;
        }
        std::this_thread::sleep_for(foregroundPollStep);
    }
    return GetForegroundWindow() == target;
}

}

std::vector<WindowInfo> listWindows() {
    std::vector<WindowInfo> windows;
    EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&windows));

    std::sort(windows.begin(), windows.end(), [](const WindowInfo& a, const WindowInfo& b) {
        if (a.process != b.process) {
            return lower(a.process) < lower(b.process);
        }
        return lower(a.title) < lower(b.title);
    });

    return windows;
}

std::uintptr_t findWindow(const std::string& rule) {
    if (IniDocument::trim(rule).empty()) {
        return 0;
    }

    for (const auto& window : listWindows()) {
        if (matches(window, rule)) {
            return window.handle;
        }
    }
    return 0;
}

bool activateWindow(std::uintptr_t handle) {
    HWND target = reinterpret_cast<HWND>(handle);
    if (target == nullptr || IsWindow(target) == FALSE) {
        return false;
    }

    if (IsIconic(target) != FALSE) {
        ShowWindow(target, SW_RESTORE);
    }

    if (GetForegroundWindow() == target) {
        return true;
    }

    const HWND foreground = GetForegroundWindow();
    const DWORD thisThread = GetCurrentThreadId();
    const DWORD targetThread = GetWindowThreadProcessId(target, nullptr);
    const DWORD foregroundThread = foreground != nullptr
        ? GetWindowThreadProcessId(foreground, nullptr)
        : 0;

    const bool attachForeground = foregroundThread != 0 && foregroundThread != thisThread;
    const bool attachTarget = targetThread != 0 && targetThread != thisThread
        && targetThread != foregroundThread;

    if (attachForeground) {
        AttachThreadInput(thisThread, foregroundThread, TRUE);
    }
    if (attachTarget) {
        AttachThreadInput(thisThread, targetThread, TRUE);
    }

    BringWindowToTop(target);
    SetForegroundWindow(target);
    SetFocus(target);

    if (attachTarget) {
        AttachThreadInput(thisThread, targetThread, FALSE);
    }
    if (attachForeground) {
        AttachThreadInput(thisThread, foregroundThread, FALSE);
    }

    if (!waitForForeground(target)) {
        return false;
    }

    std::this_thread::sleep_for(settleDelay);
    return true;
}

bool launchApplication(const std::string& path) {
    const std::string trimmed = IniDocument::trim(path);
    if (trimmed.empty()) {
        return false;
    }

    std::error_code error;
    if (!fs::exists(trimmed, error)) {
        LOG_ERROR << "Приложение не найдено по пути: " << trimmed;
        return false;
    }

    const std::wstring wide = toWide(trimmed);
    const std::wstring directory = fs::path(wide).parent_path().wstring();

    const HINSTANCE result = ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr,
        directory.empty() ? nullptr : directory.c_str(), SW_SHOWNORMAL);

    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        LOG_ERROR << "Не удалось запустить приложение: " << trimmed;
        return false;
    }
    return true;
}

ActivationResult activateTarget(const std::string& rule, const std::string& executablePath) {
    if (IniDocument::trim(rule).empty()) {
        return ActivationResult::NotConfigured;
    }

    const std::uintptr_t handle = findWindow(rule);
    if (handle != 0) {
        if (GetForegroundWindow() == reinterpret_cast<HWND>(handle)) {
            return ActivationResult::AlreadyActive;
        }
        return activateWindow(handle) ? ActivationResult::Activated : ActivationResult::Failed;
    }

    if (!IniDocument::trim(executablePath).empty()) {
        return launchApplication(executablePath)
            ? ActivationResult::Launched
            : ActivationResult::Failed;
    }

    return ActivationResult::NotFound;
}

std::string describeActivation(ActivationResult result) {
    switch (result) {
        case ActivationResult::NotConfigured: return "целевое приложение не задано";
        case ActivationResult::Activated: return "окно переведено на передний план";
        case ActivationResult::AlreadyActive: return "окно уже активно";
        case ActivationResult::Launched: return "приложение запущено";
        case ActivationResult::NotFound: return "окно не найдено";
        case ActivationResult::Failed: return "не удалось активировать окно";
    }
    return "";
}

std::string makeMatchRule(const WindowInfo& window) {
    if (!window.process.empty()) {
        return "process:" + window.process;
    }
    if (!window.className.empty()) {
        return "class:" + window.className;
    }
    return "title:" + window.title;
}

}
