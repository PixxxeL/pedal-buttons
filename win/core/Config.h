#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "IniParser.h"


namespace core {

class Config {
public:
    static const std::vector<std::string>& eventNames();

    bool load(const std::string& filePath);
    const std::string& getAppName() const;
    std::vector<std::string> getKeys(const std::string& event) const;

private:
    IniParser ini;
    std::string appName;
    std::unordered_map<std::string, std::vector<std::string>> bindings;
};

}
