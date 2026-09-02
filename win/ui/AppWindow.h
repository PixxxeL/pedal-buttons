#pragma once

#include <memory>
#include <string>

#include "../core/WindowGeometry.h"


namespace ui {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

    bool create(const std::string& title, const core::WindowGeometry& saved);
    void destroy();

    core::WindowGeometry geometry() const;

    bool shouldClose() const;
    void requestClose();
    void clearCloseRequest();

    bool isMinimized() const;
    void restore();
    void focus();

    void setVisible(bool visible);
    bool isVisible() const;

    void waitEvents();
    void beginFrame();
    void endFrame();

    static void wakeUp();

    float scale() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
