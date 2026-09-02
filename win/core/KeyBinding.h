#pragma once

#include <string>
#include <vector>


namespace core {

using KeyChord = std::vector<std::string>;
using KeySequence = std::vector<KeyChord>;

KeySequence parseKeySequence(const std::string& text);
std::string formatKeySequence(const KeySequence& sequence);
bool isEmpty(const KeySequence& sequence);

}
