#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "IniDocument.h"
#include "KeyBinding.h"
#include "WindowGeometry.h"


namespace core {

class Config {
public:
    static const std::vector<std::string>& eventNames();
    static bool createDefault(const std::string& filePath);

    bool load(const std::string& filePath);
    bool reload();
    bool save();
    bool isLoaded() const;
    bool isDirty() const;
    std::uint64_t generation() const;
    const std::string& path() const;

    std::vector<std::string> profileNames() const;
    const std::string& activeProfile() const;
    void setActiveProfile(const std::string& name);
    void addProfile(const std::string& name);
    void removeProfile(const std::string& name);

    std::string windowMatch() const;
    std::string windowMatch(const std::string& profile) const;
    void setWindowMatch(const std::string& profile, const std::string& value);

    std::string executablePath() const;
    std::string executablePath(const std::string& profile) const;
    void setExecutablePath(const std::string& profile, const std::string& value);

    KeySequence binding(const std::string& event) const;
    KeySequence binding(const std::string& profile, const std::string& event) const;
    void setBinding(const std::string& profile, const std::string& event, const KeySequence& keys);

    std::string port() const;
    void setPort(const std::string& value);

    bool autoReconnect() const;
    void setAutoReconnect(bool value);

    WindowGeometry windowGeometry() const;
    void setWindowGeometry(const WindowGeometry& geometry);

    bool useTray() const;
    void setUseTray(bool value);
    bool minimizeToTray() const;
    void setMinimizeToTray(bool value);
    bool closeToTray() const;
    void setCloseToTray(bool value);
    bool hideTrayIconWithWindow() const;
    void setHideTrayIconWithWindow(bool value);
    bool startMinimized() const;
    void setStartMinimized(bool value);
    bool autostart() const;
    void setAutostart(bool value);

private:
    IniDocument document;
    std::string filePath;
    std::string profile;
    bool loaded = false;
    bool dirty = false;
    std::uint64_t revision = 0;
};

}
