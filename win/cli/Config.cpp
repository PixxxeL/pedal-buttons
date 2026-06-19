#include "Config.h"
#include <boost/log/trivial.hpp>


bool Config::load(const std::string& filePath) {
    if (!ini.load(filePath)) {
        BOOST_LOG_TRIVIAL(error) << "Не удалось загрузить конфиг: " << filePath;
        return false;
    }

    appName = ini.getString("app", "app");
    if (appName.empty()) {
        BOOST_LOG_TRIVIAL(error) << "Секция [app] не найдена или пуста";
        return false;
    }

    if (!ini.hasSection(appName)) {
        BOOST_LOG_TRIVIAL(error) << "Секция [" << appName << "] не найдена в конфиге";
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
