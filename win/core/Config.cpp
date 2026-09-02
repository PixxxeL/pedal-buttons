#include "Config.h"

#include <algorithm>
#include <stdexcept>

#include "Logger.h"


namespace core {

namespace {

const char* settingsSection = "app";

bool isSettingsSection(const std::string& name) {
    return IniDocument::toLower(name) == settingsSection;
}

int toInt(const std::string& text, int defaultValue) {
    try {
        const std::string value = IniDocument::trim(text);
        if (value.empty()) {
            return defaultValue;
        }
        std::size_t consumed = 0;
        const int result = std::stoi(value, &consumed);
        return consumed == value.size() ? result : defaultValue;
    }
    catch (const std::exception&) {
        return defaultValue;
    }
}

bool toBool(const std::string& text, bool defaultValue) {
    const std::string value = IniDocument::toLower(IniDocument::trim(text));
    if (value == "1" || value == "true" || value == "yes" || value == "on") {
        return true;
    }
    if (value == "0" || value == "false" || value == "no" || value == "off") {
        return false;
    }
    return defaultValue;
}

}

const std::vector<std::string>& Config::eventNames() {
    static const std::vector<std::string> names = {
        "LEFT_CLICK", "LEFT_HOLD", "RIGHT_CLICK", "RIGHT_HOLD"
    };
    return names;
}

bool Config::createDefault(const std::string& filePath) {
    IniDocument document;
    document.set(settingsSection, "app", "default");
    document.addSection("default");

    for (const auto& event : eventNames()) {
        document.set("default", event, "");
    }

    if (!document.save(filePath)) {
        LOG_ERROR << "Не удалось создать конфиг: " << filePath;
        return false;
    }

    LOG_INFO << "Создан конфиг по умолчанию: " << filePath;
    return true;
}

bool Config::load(const std::string& path) {
    IniDocument loadedDocument;
    if (!loadedDocument.load(path)) {
        LOG_ERROR << "Не удалось загрузить конфиг: " << path;
        return false;
    }

    const std::string name = IniDocument::trim(loadedDocument.get(settingsSection, "app"));
    if (name.empty()) {
        LOG_ERROR << "Секция [app] не найдена или в ней не указан профиль";
        return false;
    }

    if (!loadedDocument.hasSection(name)) {
        LOG_ERROR << "Профиль [" << name << "] не найден в конфиге";
        return false;
    }

    document = std::move(loadedDocument);
    filePath = path;
    profile = name;
    loaded = true;
    dirty = false;
    revision++;

    for (const auto& event : eventNames()) {
        const std::string raw = document.get(profile, event);
        if (raw.find(',') != std::string::npos && raw.find('+') == std::string::npos) {
            LOG_WARNING << "Привязка " << event << " = " << raw
                << " содержит запятую. Теперь запятая означает последовательность,"
                << " а одновременное нажатие записывается через плюс: "
                << "например ctrl+r";
        }
    }

    return true;
}

bool Config::reload() {
    return loaded && load(filePath);
}

bool Config::save() {
    if (!loaded) {
        return false;
    }

    if (!document.save(filePath)) {
        LOG_ERROR << "Не удалось сохранить конфиг: " << filePath;
        return false;
    }

    dirty = false;
    LOG_INFO << "Конфиг сохранён: " << filePath;
    return true;
}

bool Config::isLoaded() const {
    return loaded;
}

bool Config::isDirty() const {
    return dirty;
}

std::uint64_t Config::generation() const {
    return revision;
}

const std::string& Config::path() const {
    return filePath;
}

std::vector<std::string> Config::profileNames() const {
    std::vector<std::string> names;
    for (const auto& section : document.sections()) {
        if (!isSettingsSection(section)) {
            names.push_back(section);
        }
    }
    return names;
}

const std::string& Config::activeProfile() const {
    return profile;
}

void Config::setActiveProfile(const std::string& name) {
    if (name.empty() || !document.hasSection(name)) {
        return;
    }
    profile = name;
    document.set(settingsSection, "app", name);
    dirty = true;
}

void Config::addProfile(const std::string& name) {
    const std::string trimmed = IniDocument::trim(name);
    if (trimmed.empty() || isSettingsSection(trimmed) || document.hasSection(trimmed)) {
        return;
    }

    document.addSection(trimmed);
    for (const auto& event : eventNames()) {
        document.set(trimmed, event, "");
    }
    dirty = true;
}

void Config::removeProfile(const std::string& name) {
    if (isSettingsSection(name) || !document.hasSection(name)) {
        return;
    }

    const auto names = profileNames();
    if (names.size() <= 1) {
        return;
    }

    document.removeSection(name);
    dirty = true;

    if (IniDocument::toLower(profile) == IniDocument::toLower(name)) {
        for (const auto& candidate : profileNames()) {
            setActiveProfile(candidate);
            break;
        }
    }
}

std::string Config::windowMatch() const {
    return windowMatch(profile);
}

std::string Config::windowMatch(const std::string& profileName) const {
    return IniDocument::trim(document.get(profileName, "match"));
}

void Config::setWindowMatch(const std::string& profileName, const std::string& value) {
    if (windowMatch(profileName) == IniDocument::trim(value)) {
        return;
    }
    document.set(profileName, "match", IniDocument::trim(value));
    dirty = true;
}

std::string Config::executablePath() const {
    return executablePath(profile);
}

std::string Config::executablePath(const std::string& profileName) const {
    return IniDocument::trim(document.get(profileName, "exe"));
}

void Config::setExecutablePath(const std::string& profileName, const std::string& value) {
    if (executablePath(profileName) == IniDocument::trim(value)) {
        return;
    }
    document.set(profileName, "exe", IniDocument::trim(value));
    dirty = true;
}

KeySequence Config::binding(const std::string& event) const {
    return binding(profile, event);
}

KeySequence Config::binding(const std::string& profileName, const std::string& event) const {
    return parseKeySequence(document.get(profileName, event));
}

void Config::setBinding(const std::string& profileName, const std::string& event,
        const KeySequence& keys) {
    const std::string text = formatKeySequence(keys);
    if (document.get(profileName, event) == text) {
        return;
    }
    document.set(profileName, event, text);
    dirty = true;
}

std::string Config::port() const {
    return IniDocument::trim(document.get(settingsSection, "port"));
}

void Config::setPort(const std::string& value) {
    if (port() == value) {
        return;
    }
    document.set(settingsSection, "port", value);
    dirty = true;
}

bool Config::autoReconnect() const {
    return toBool(document.get(settingsSection, "autoReconnect"), true);
}

WindowGeometry Config::windowGeometry() const {
    WindowGeometry geometry;
    geometry.x = toInt(document.get(settingsSection, "windowX"), WindowGeometry::unsetPosition);
    geometry.y = toInt(document.get(settingsSection, "windowY"), WindowGeometry::unsetPosition);
    geometry.width = toInt(document.get(settingsSection, "windowWidth"), 0);
    geometry.height = toInt(document.get(settingsSection, "windowHeight"), 0);
    geometry.maximized = toBool(document.get(settingsSection, "windowMaximized"), false);
    return geometry;
}

void Config::setWindowGeometry(const WindowGeometry& geometry) {
    document.set(settingsSection, "windowX", std::to_string(geometry.x));
    document.set(settingsSection, "windowY", std::to_string(geometry.y));
    document.set(settingsSection, "windowWidth", std::to_string(geometry.width));
    document.set(settingsSection, "windowHeight", std::to_string(geometry.height));
    document.set(settingsSection, "windowMaximized", geometry.maximized ? "true" : "false");
}

void Config::setAutoReconnect(bool value) {
    if (document.hasKey(settingsSection, "autoReconnect") && autoReconnect() == value) {
        return;
    }
    document.set(settingsSection, "autoReconnect", value ? "true" : "false");
    dirty = true;
}


bool Config::useTray() const {
    return toBool(document.get(settingsSection, "tray"), true);
}

void Config::setUseTray(bool value) {
    if (document.hasKey(settingsSection, "tray") && useTray() == value) {
        return;
    }
    document.set(settingsSection, "tray", value ? "true" : "false");
    dirty = true;
}

bool Config::minimizeToTray() const {
    return toBool(document.get(settingsSection, "minimizeToTray"), true);
}

void Config::setMinimizeToTray(bool value) {
    if (document.hasKey(settingsSection, "minimizeToTray") && minimizeToTray() == value) {
        return;
    }
    document.set(settingsSection, "minimizeToTray", value ? "true" : "false");
    dirty = true;
}

bool Config::closeToTray() const {
    return toBool(document.get(settingsSection, "closeToTray"), true);
}

void Config::setCloseToTray(bool value) {
    if (document.hasKey(settingsSection, "closeToTray") && closeToTray() == value) {
        return;
    }
    document.set(settingsSection, "closeToTray", value ? "true" : "false");
    dirty = true;
}

bool Config::hideTrayIconWithWindow() const {
    return toBool(document.get(settingsSection, "hideTrayIconWithWindow"), false);
}

void Config::setHideTrayIconWithWindow(bool value) {
    if (document.hasKey(settingsSection, "hideTrayIconWithWindow") && hideTrayIconWithWindow() == value) {
        return;
    }
    document.set(settingsSection, "hideTrayIconWithWindow", value ? "true" : "false");
    dirty = true;
}

bool Config::startMinimized() const {
    return toBool(document.get(settingsSection, "startMinimized"), false);
}

void Config::setStartMinimized(bool value) {
    if (document.hasKey(settingsSection, "startMinimized") && startMinimized() == value) {
        return;
    }
    document.set(settingsSection, "startMinimized", value ? "true" : "false");
    dirty = true;
}

bool Config::autostart() const {
    return toBool(document.get(settingsSection, "autostart"), false);
}

void Config::setAutostart(bool value) {
    if (document.hasKey(settingsSection, "autostart") && autostart() == value) {
        return;
    }
    document.set(settingsSection, "autostart", value ? "true" : "false");
    dirty = true;
}

}
