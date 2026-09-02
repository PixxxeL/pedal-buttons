#include "IniParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>


namespace core {

std::string IniParser::trim(const std::string& text) {
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

std::string IniParser::toLower(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

std::vector<std::string> IniParser::split(const std::string& text, char delimiter) {
    std::vector<std::string> result;
    std::istringstream stream(text);
    std::string item;
    while (std::getline(stream, item, delimiter)) {
        result.push_back(trim(item));
    }
    return result;
}

bool IniParser::load(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    sections.clear();
    std::string currentSection;
    std::string line;

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            currentSection = toLower(trim(line.substr(1, line.size() - 2)));
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos || currentSection.empty()) {
            continue;
        }

        const std::string key = toLower(trim(line.substr(0, separator)));
        const std::string value = trim(line.substr(separator + 1));

        sections[currentSection][key] = value;
    }

    return true;
}

std::string IniParser::getString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    const auto sectionIt = sections.find(toLower(section));
    if (sectionIt == sections.end()) {
        return defaultValue;
    }

    const auto keyIt = sectionIt->second.find(toLower(key));
    if (keyIt == sectionIt->second.end()) {
        return defaultValue;
    }

    return keyIt->second;
}

std::vector<std::string> IniParser::getArray(const std::string& section, const std::string& key) const {
    const std::string value = getString(section, key, "");
    if (value.empty()) {
        return {};
    }
    return split(value, ',');
}

bool IniParser::hasSection(const std::string& section) const {
    return sections.find(toLower(section)) != sections.end();
}

}
