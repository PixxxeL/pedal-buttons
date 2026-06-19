#pragma once

#include <string>
#include <unordered_map>
#include <vector>


class IniParser {
private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
    std::string currentSection;

    static std::string trim(const std::string& s);
    static std::string toLower(const std::string& s);
    static std::vector<std::string> split(const std::string& s, char delim);

public:
    bool load(const std::string& filePath);

    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    std::vector<std::string> getArray(const std::string& section, const std::string& key) const;
    bool hasSection(const std::string& section) const;
};
