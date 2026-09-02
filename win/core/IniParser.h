#pragma once

#include <string>
#include <unordered_map>
#include <vector>


namespace core {

class IniParser {
public:
    bool load(const std::string& filePath);

    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    std::vector<std::string> getArray(const std::string& section, const std::string& key) const;
    bool hasSection(const std::string& section) const;

private:
    static std::string trim(const std::string& text);
    static std::string toLower(const std::string& text);
    static std::vector<std::string> split(const std::string& text, char delimiter);

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
};

}
