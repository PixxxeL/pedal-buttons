#include "KeySender.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>
#include <vector>

#include "Logger.h"


namespace core {

namespace {

constexpr std::chrono::milliseconds chordGap{20};

struct Stroke {
    WORD scanCode = 0;
    WORD virtualKey = 0;
    bool extended = false;
    bool valid = false;
};

HKL referenceLayout() {
    static const HKL layout = [] {
        HKL loaded = LoadKeyboardLayoutW(L"00000409", KLF_NOTELLSHELL);
        if (loaded == nullptr) {
            LOG_WARNING << "Не удалось загрузить раскладку US, используется текущая";
            loaded = GetKeyboardLayout(0);
        }
        return loaded;
    }();
    return layout;
}

bool isAlwaysExtended(int virtualKey) {
    switch (virtualKey) {
        case VK_INSERT:
        case VK_DELETE:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_NUMLOCK:
        case VK_SNAPSHOT:
        case VK_DIVIDE:
        case VK_RCONTROL:
        case VK_RMENU:
            return true;
        default:
            return false;
    }
}

bool isModifierKey(const std::string& key) {
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

Stroke strokeForVirtualKey(int virtualKey) {
    Stroke stroke;
    stroke.virtualKey = static_cast<WORD>(virtualKey);

    const UINT mapped = MapVirtualKeyExW(static_cast<UINT>(virtualKey),
        MAPVK_VK_TO_VSC_EX, referenceLayout());
    if (mapped == 0) {
        return stroke;
    }

    const UINT prefix = (mapped >> 8) & 0xFF;
    stroke.scanCode = static_cast<WORD>(mapped & 0xFF);
    stroke.extended = prefix == 0xE0 || isAlwaysExtended(virtualKey);
    stroke.valid = true;
    return stroke;
}

bool resolveKey(const std::string& key, int& virtualKey, bool& needsShift) {
    const std::string lowered = toLower(key);
    needsShift = false;

    const auto& map = KeySender::virtualKeyMap();
    const auto it = map.find(lowered);
    if (it != map.end()) {
        virtualKey = it->second;
        return true;
    }

    if (key.size() != 1) {
        return false;
    }

    const SHORT scanned = VkKeyScanExA(key[0], referenceLayout());
    if (scanned == -1) {
        return false;
    }

    virtualKey = LOBYTE(scanned);
    needsShift = (HIBYTE(scanned) & 1) != 0;
    return true;
}

INPUT makeInput(const Stroke& stroke, bool release) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;

    if (stroke.valid) {
        input.ki.wVk = 0;
        input.ki.wScan = stroke.scanCode;
        input.ki.dwFlags = KEYEVENTF_SCANCODE;
        if (stroke.extended) {
            input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
        }
    }
    else {
        input.ki.wVk = stroke.virtualKey;
        input.ki.dwFlags = 0;
    }

    if (release) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    return input;
}

}

const std::unordered_map<std::string, int>& KeySender::virtualKeyMap() {
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

    const UINT character = MapVirtualKeyExW(static_cast<UINT>(virtualKey),
        MAPVK_VK_TO_CHAR, referenceLayout()) & 0x7FFF;
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

    std::vector<Stroke> modifiers;
    std::vector<Stroke> regulars;
    bool shiftRequired = false;
    bool shiftRequested = false;

    for (const auto& key : keys) {
        int virtualKey = 0;
        bool needsShift = false;

        if (!resolveKey(key, virtualKey, needsShift)) {
            LOG_WARNING << "Неизвестная клавиша в привязке: " << key;
            continue;
        }

        const Stroke stroke = strokeForVirtualKey(virtualKey);
        if (isModifierKey(toLower(key))) {
            modifiers.push_back(stroke);
            if (virtualKey == VK_SHIFT) {
                shiftRequested = true;
            }
        }
        else {
            regulars.push_back(stroke);
            shiftRequired = shiftRequired || needsShift;
        }
    }

    if (shiftRequired && !shiftRequested) {
        modifiers.push_back(strokeForVirtualKey(VK_SHIFT));
    }

    if (modifiers.empty() && regulars.empty()) {
        return;
    }

    std::vector<INPUT> inputs;
    inputs.reserve((modifiers.size() + regulars.size()) * 2);

    for (const auto& stroke : modifiers) {
        inputs.push_back(makeInput(stroke, false));
    }
    for (const auto& stroke : regulars) {
        inputs.push_back(makeInput(stroke, false));
    }
    for (auto it = regulars.rbegin(); it != regulars.rend(); ++it) {
        inputs.push_back(makeInput(*it, true));
    }
    for (auto it = modifiers.rbegin(); it != modifiers.rend(); ++it) {
        inputs.push_back(makeInput(*it, true));
    }

    const UINT sent = SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
    if (sent != inputs.size()) {
        LOG_WARNING << "Отправлено " << sent << " из " << inputs.size()
            << " событий клавиатуры, код " << GetLastError();
    }
}

}
