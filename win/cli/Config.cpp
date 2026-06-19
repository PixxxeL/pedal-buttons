#include "Config.h"
#include <iostream>


bool Config::load(const std::string& filePath) {
    if (!ini.load(filePath)) {
        std::cerr << "Не удалось загрузить конфиг: " << filePath << std::endl;
        return false;
    }

    appName = ini.getString("app", "app");
    if (appName.empty()) {
        std::cerr << "Секция [app] не найдена или пуста" << std::endl;
        return false;
    }

    if (!ini.hasSection(appName)) {
        std::cerr << "Секция [" << appName << "] не найдена в конфиге" << std::endl;
        return false;
    }

    static const char* events[] = {
        "LEFT_CLICK", "LEFT_HOLD", "RIGHT_CLICK", "RIGHT_HOLD"
    };
    for (const auto& event : events) {
        bindings[event] = ini.getArray(appName, event);
    }

    return true;
}

const std::string& Config::getAppName() const {
    return appName;
}

std::vector<std::string> Config::getKeys(const std::string& event) const {
    auto it = bindings.find(event);
    if (it == bindings.end()) return {};
    return it->second;
}
