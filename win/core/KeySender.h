#pragma once

#include <string>
#include <unordered_map>

#include "KeyBinding.h"


namespace core {

class KeySender {
public:
    static void send(const KeySequence& sequence);
    static void sendChord(const KeyChord& chord);
    static std::string nameForVirtualKey(int virtualKey);
    static bool isModifierName(const std::string& key);

private:
    static const std::unordered_map<std::string, int>& getKeyMap();
};

}
