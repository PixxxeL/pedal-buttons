#include "KeySender.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>


namespace core {

namespace {

constexpr std::chrono::milliseconds chordGap{20};

INPUT makeKeyInput(WORD virtualKey, DWORD flags) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = virtualKey;
    input.ki.dwFlags = flags;
    return input;
}

bool isModifierKey(const std::string& key) {
    return key == "ctrl" || key == "control" ||
        key == "alt" || key == "menu" ||
        key == "shift";
}

const std::unordered_map<int, std::string>& canonicalNames() {
    static const std::unordered_map<int, std::string> names = {
        {VK_CONTROL, "ctrl"}, {VK_LCONTROL, "ctrl"}, {VK_RCONTROL, "ctrl"},
        {VK_MENU, "alt"}, {VK_LMENU, "alt"}, {VK_RMENU, "alt"},
        {VK_SHIFT, "shift"}, {VK_LSHIFT, "shift"}, {VK_RSHIFT, "shift"},
        {VK_SPACE, "space"}, {VK_RETURN, "enter"}, {VK_TAB, "tab"},
        {VK_ESCAPE, "esc"}, {VK_BACK, "backspace"}, {VK_DELETE, "delete"},
        {VK_LEFT, "left"}, {VK_RIGHT, "right"}, {VK_UP, "up"}, {VK_DOWN, "down"},
        {VK_HOME, "home"}, {VK_END, "end"},
        {VK_PRIOR, "pageup"}, {VK_NEXT, "pagedown"},
        {VK_F1, "f1"}, {VK_F2, "f2"}, {VK_F3, "f3"}, {VK_F4, "f4"},
        {VK_F5, "f5"}, {VK_F6, "f6"}, {VK_F7, "f7"}, {VK_F8, "f8"},
        {VK_F9, "f9"}, {VK_F10, "f10"}, {VK_F11, "f11"}, {VK_F12, "f12"},
        {VK_INSERT, "insert"}, {VK_SNAPSHOT, "print"},
        {VK_CAPITAL, "capslock"}, {VK_NUMLOCK, "numlock"}, {VK_SCROLL, "scrolllock"},
    };
    return names;
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

std::string KeySender::nameForVirtualKey(int virtualKey) {
    const auto& names = canonicalNames();
    const auto it = names.find(virtualKey);
    if (it != names.end()) {
        return it->second;
    }

    if ((virtualKey >= '0' && virtualKey <= '9') || (virtualKey >= 'A' && virtualKey <= 'Z')) {
        return std::string(1, static_cast<char>(std::tolower(virtualKey)));
    }

    const UINT character = MapVirtualKeyA(static_cast<UINT>(virtualKey), MAPVK_VK_TO_CHAR) & 0x7FFF;
    if (character >= 0x20 && character < 0x7F) {
        return std::string(1, static_cast<char>(std::tolower(static_cast<int>(character))));
    }

    return "";
}

bool KeySender::isModifierName(const std::string& key) {
    return isModifierKey(toLower(key));
}

void KeySender::send(const KeySequence& sequence) {
    for (std::size_t i = 0; i < sequence.size(); i++) {
        if (i > 0) {
            std::this_thread::sleep_for(chordGap);
        }
        sendChord(sequence[i]);
    }
}

void KeySender::sendChord(const KeyChord& keys) {
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
            if (isModifierKey(lowered)) {
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
