#pragma once

#include <memory>
#include <string>


namespace ui {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    AppWindow(const AppWindow&) = delete;
    AppWindow& operator=(const AppWindow&) = delete;

    bool create(const std::string& title, int width, int height);
    void destroy();

    bool shouldClose() const;
    void requestClose();

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
