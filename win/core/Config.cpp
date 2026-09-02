#include "Config.h"

#include "Logger.h"


namespace core {

const std::vector<std::string>& Config::eventNames() {
    static const std::vector<std::string> names = {
        "LEFT_CLICK", "LEFT_HOLD", "RIGHT_CLICK", "RIGHT_HOLD"
    };
    return names;
}

bool Config::load(const std::string& filePath) {
    if (!ini.load(filePath)) {
        LOG_ERROR << "Не удалось загрузить конфиг: " << filePath;
        return false;
    }

    appName = ini.getString("app", "app");
    if (appName.empty()) {
        LOG_ERROR << "Секция [app] не найдена или пуста";
        return false;
    }

    if (!ini.hasSection(appName)) {
        LOG_ERROR << "Секция [" << appName << "] не найдена в конфиге";
        return false;
    }

    bindings.clear();
    for (const auto& event : eventNames()) {
        bindings[event] = ini.getArray(appName, event);
    }

    return true;
}

const std::string& Config::getAppName() const {
    return appName;
}

std::vector<std::string> Config::getKeys(const std::string& event) const {
    const auto it = bindings.find(event);
    if (it == bindings.end()) {
        return {};
    }
    return it->second;
}

}
