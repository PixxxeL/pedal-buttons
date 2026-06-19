#include "IniParser.h"
#include <fstream>
#include <sstream>
#include <algorithm>


std::string IniParser::trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string IniParser::toLower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> IniParser::split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::istringstream stream(s);
    std::string item;
    while (std::getline(stream, item, delim)) {
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

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos || currentSection.empty()) {
            continue;
        }

        std::string key = toLower(trim(line.substr(0, eqPos)));
        std::string value = trim(line.substr(eqPos + 1));

        sections[currentSection][key] = value;
    }

    return true;
}

std::string IniParser::getString(const std::string& section, const std::string& key, const std::string& defaultValue) const {
    auto secIt = sections.find(toLower(section));
    if (secIt == sections.end()) return defaultValue;

    auto keyIt = secIt->second.find(toLower(key));
    if (keyIt == secIt->second.end()) return defaultValue;

    return keyIt->second;
}

std::vector<std::string> IniParser::getArray(const std::string& section, const std::string& key) const {
    std::string value = getString(section, key, "");
    if (value.empty()) return {};
    return split(value, ',');
}

bool IniParser::hasSection(const std::string& section) const {
    return sections.find(toLower(section)) != sections.end();
}
