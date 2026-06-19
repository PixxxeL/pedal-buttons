#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "IniParser.h"


class Config {
private:
    IniParser ini;
    std::string appName;
    std::unordered_map<std::string, std::vector<std::string>> bindings;

public:
    bool load(const std::string& filePath);
    const std::string& getAppName() const;
    std::vector<std::string> getKeys(const std::string& event) const;
};
