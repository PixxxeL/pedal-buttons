#include "KeyCapture.h"

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <set>
#include <vector>

#include "../core/KeySender.h"
#include "../core/Logger.h"


namespace ui {

namespace {

constexpr std::chrono::seconds captureTimeout{10};

HHOOK hook = nullptr;
std::set<int> pressedKeys;
std::vector<std::string> capturedChord;
bool captureReady = false;
bool captureCancelled = false;
std::chrono::steady_clock::time_point captureStarted;

void rememberChord() {
    std::vector<std::string> modifiers;
    std::vector<std::string> regulars;

    for (int virtualKey : pressedKeys) {
        const std::string name = core::KeySender::nameForVirtualKey(virtualKey);
        if (name.empty()) {
            continue;
        }
        if (core::KeySender::isModifierName(name)) {
            if (std::find(modifiers.begin(), modifiers.end(), name) == modifiers.end()) {
                modifiers.push_back(name);
            }
        }
        else if (std::find(regulars.begin(), regulars.end(), name) == regulars.end()) {
            regulars.push_back(name);
        }
    }

    if (regulars.empty() && modifiers.empty()) {
        return;
    }

    static const std::vector<std::string> order = { "ctrl", "alt", "shift" };
    std::vector<std::string> chord;
    for (const auto& name : order) {
        if (std::find(modifiers.begin(), modifiers.end(), name) != modifiers.end()) {
            chord.push_back(name);
        }
    }
    chord.insert(chord.end(), regulars.begin(), regulars.end());

    capturedChord = std::move(chord);
}

LRESULT CALLBACK keyboardHook(int code, WPARAM message, LPARAM data) {
    if (code != HC_ACTION) {
        return CallNextHookEx(hook, code, message, data);
    }

    const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
    const int virtualKey = static_cast<int>(event->vkCode);
    const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool up = message == WM_KEYUP || message == WM_SYSKEYUP;

    if (down) {
        if (virtualKey == VK_ESCAPE && pressedKeys.empty()) {
            captureCancelled = true;
            return 1;
        }
        pressedKeys.insert(virtualKey);
        rememberChord();
    }
    else if (up) {
        pressedKeys.erase(virtualKey);
        if (pressedKeys.empty() && !capturedChord.empty()) {
            captureReady = true;
        }
    }

    return 1;
}

}

KeyCapture::~KeyCapture() {
    cancel();
}

bool KeyCapture::start() {
    if (hook != nullptr) {
        return false;
    }

    pressedKeys.clear();
    capturedChord.clear();
    captureReady = false;
    captureCancelled = false;
    captureStarted = std::chrono::steady_clock::now();

    hook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardHook, GetModuleHandleW(nullptr), 0);
    if (hook == nullptr) {
        LOG_ERROR << "Не удалось установить перехват клавиатуры";
        return false;
    }

    return true;
}

void KeyCapture::cancel() {
    if (hook != nullptr) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
    }
    pressedKeys.clear();
    capturedChord.clear();
    captureReady = false;
    captureCancelled = false;
}

bool KeyCapture::isActive() const {
    return hook != nullptr;
}

bool KeyCapture::take(std::string& chord) {
    if (hook == nullptr) {
        return false;
    }

    if (captureCancelled) {
        cancel();
        return false;
    }

    if (std::chrono::steady_clock::now() - captureStarted > captureTimeout) {
        LOG_WARNING << "Захват клавиш прерван по таймауту";
        cancel();
        return false;
    }

    if (!captureReady) {
        return false;
    }

    chord.clear();
    for (std::size_t i = 0; i < capturedChord.size(); i++) {
        if (i > 0) {
            chord += "+";
        }
        chord += capturedChord[i];
    }

    cancel();
    return !chord.empty();
}

}
