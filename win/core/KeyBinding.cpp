#include "KeyBinding.h"

#include <sstream>

#include "IniDocument.h"


namespace core {

namespace {

std::vector<std::string> splitBy(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        const std::string trimmed = IniDocument::trim(item);
        if (!trimmed.empty()) {
            parts.push_back(trimmed);
        }
    }
    return parts;
}

}

KeySequence parseKeySequence(const std::string& text) {
    KeySequence sequence;

    for (const auto& step : splitBy(text, ',')) {
        KeyChord chord = splitBy(step, '+');
        if (!chord.empty()) {
            sequence.push_back(std::move(chord));
        }
    }

    return sequence;
}

std::string formatKeySequence(const KeySequence& sequence) {
    std::string result;

    for (const auto& chord : sequence) {
        if (!result.empty()) {
            result += ", ";
        }
        for (std::size_t i = 0; i < chord.size(); i++) {
            if (i > 0) {
                result += "+";
            }
            result += chord[i];
        }
    }

    return result;
}

bool isEmpty(const KeySequence& sequence) {
    for (const auto& chord : sequence) {
        if (!chord.empty()) {
            return false;
        }
    }
    return true;
}

}
