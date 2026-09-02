#include "KeySender.h"

#include <windows.h>

#include <algorithm>
#include <cctype>


namespace core {

namespace {

INPUT makeKeyInput(WORD virtualKey, DWORD flags) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = virtualKey;
    input.ki.dwFlags = flags;
    return input;
}

bool isModifier(const std::string& key) {
    return key == "ctrl" || key == "control" ||
        key == "alt" || key == "menu" ||
        key == "shift";
}

std::string toLower(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

}

const std::unordered_map<std::string, int>& KeySender::getKeyMap() {
    static const std::unordered_map<std::string, int> keyMap = {
        {"ctrl", VK_CONTROL}, {"control", VK_CONTROL},
        {"alt", VK_MENU}, {"menu", VK_MENU},
        {"shift", VK_SHIFT},
        {"space", VK_SPACE},
        {"enter", VK_RETURN}, {"return", VK_RETURN},
        {"tab", VK_TAB},
        {"esc", VK_ESCAPE}, {"escape", VK_ESCAPE},
        {"backspace", VK_BACK}, {"back", VK_BACK},
        {"delete", VK_DELETE}, {"del", VK_DELETE},
        {"left", VK_LEFT}, {"right", VK_RIGHT},
        {"up", VK_UP}, {"down", VK_DOWN},
        {"home", VK_HOME}, {"end", VK_END},
        {"pageup", VK_PRIOR}, {"pagedown", VK_NEXT},
        {"f1", VK_F1}, {"f2", VK_F2}, {"f3", VK_F3}, {"f4", VK_F4},
        {"f5", VK_F5}, {"f6", VK_F6}, {"f7", VK_F7}, {"f8", VK_F8},
        {"f9", VK_F9}, {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12},
        {"insert", VK_INSERT}, {"print", VK_SNAPSHOT},
        {"capslock", VK_CAPITAL}, {"numlock", VK_NUMLOCK}, {"scrolllock", VK_SCROLL},
    };
    return keyMap;
}

std::string KeySender::describe(const std::vector<std::string>& keys) {
    std::string result;
    for (std::size_t i = 0; i < keys.size(); i++) {
        if (i > 0) {
            result += "+";
        }
        result += keys[i];
    }
    return result;
}

void KeySender::send(const std::vector<std::string>& keys) {
    if (keys.empty()) {
        return;
    }

    const auto& keyMap = getKeyMap();

    std::vector<WORD> modifiers;
    std::vector<WORD> regulars;

    for (const auto& key : keys) {
        const std::string lowered = toLower(key);
        const auto it = keyMap.find(lowered);
        if (it != keyMap.end()) {
            if (isModifier(lowered)) {
                modifiers.push_back(static_cast<WORD>(it->second));
            }
            else {
                regulars.push_back(static_cast<WORD>(it->second));
            }
        }
        else if (lowered.size() == 1) {
            const WORD virtualKey = VkKeyScanA(lowered[0]) & 0xFF;
            regulars.push_back(virtualKey);
        }
    }

    std::vector<INPUT> inputs;
    for (WORD virtualKey : modifiers) {
        inputs.push_back(makeKeyInput(virtualKey, 0));
    }
    for (WORD virtualKey : regulars) {
        inputs.push_back(makeKeyInput(virtualKey, 0));
    }
    for (WORD virtualKey : regulars) {
        inputs.push_back(makeKeyInput(virtualKey, KEYEVENTF_KEYUP));
    }
    for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it) {
        inputs.push_back(makeKeyInput(*it, KEYEVENTF_KEYUP));
    }

    if (inputs.empty()) {
        return;
    }

    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

}
