#pragma once

#include <memory>
#include <string>


namespace ui {

enum class TrayState {
    Disconnected,
    Connected,
    LeftPressed,
    RightPressed,
    Paused
};

enum class TrayCommand {
    None,
    ToggleWindow,
    ShowWindow,
    TogglePause,
    Exit
};

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    bool create();
    void destroy();

    void setIconVisible(bool visible);
    void setState(TrayState state, const std::string& tooltip);
    void setWindowVisible(bool visible);
    void setPaused(bool paused);

    bool poll(TrayCommand& command);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
