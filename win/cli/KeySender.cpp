#include "KeySender.h"
#include <windows.h>
#include <algorithm>


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

static INPUT makeKeyInput(WORD vk, DWORD flags) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.dwFlags = flags;
    return input;
}

static bool isModifier(const std::string& key) {
    return key == "ctrl" || key == "control" ||
           key == "alt" || key == "menu" ||
           key == "shift";
}

void KeySender::send(const std::vector<std::string>& keys) {
    if (keys.empty()) {
        return;
    }

    const auto& keyMap = getKeyMap();
    std::vector<INPUT> inputs;

    std::vector<std::string> lowerKeys;
    lowerKeys.reserve(keys.size());
    for (const auto& k : keys) {
        std::string lk = k;
        std::transform(lk.begin(), lk.end(), lk.begin(),
            [](unsigned char c) { return std::tolower(c); });
        lowerKeys.push_back(lk);
    }

    std::vector<WORD> modifiers;
    std::vector<WORD> regulars;

    for (const auto& k : lowerKeys) {
        auto it = keyMap.find(k);
        if (it != keyMap.end()) {
            if (isModifier(k)) {
                modifiers.push_back(it->second);
            } else {
                regulars.push_back(it->second);
            }
        } else if (k.size() == 1) {
            VkKeyScanA(k[0]);
            WORD vk = VkKeyScanA(k[0]) & 0xFF;
            regulars.push_back(vk);
        }
    }

    for (WORD vk : modifiers) {
        inputs.push_back(makeKeyInput(vk, 0));
    }
    for (WORD vk : regulars) {
        inputs.push_back(makeKeyInput(vk, 0));
    }
    for (WORD vk : regulars) {
        inputs.push_back(makeKeyInput(vk, KEYEVENTF_KEYUP));
    }
    for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it) {
        inputs.push_back(makeKeyInput(*it, KEYEVENTF_KEYUP));
    }

    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}
