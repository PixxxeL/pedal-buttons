#pragma once

#include <array>
#include <string>
#include <vector>

#include "../core/AppActivator.h"
#include "../core/BoardDetector.h"
#include "../core/Config.h"
#include "../core/PedalService.h"
#include "../core/PortEnumerator.h"
#include "KeyCapture.h"


namespace ui {

class WizardView {
public:
    WizardView(core::PedalService& service, core::Config& config, core::BoardDetector& detector);

    void begin();
    void update();
    void draw();

    bool isActive() const;

private:
    void drawBoardStep();
    void drawTargetStep();
    void drawBindingsStep();
    void drawNavigation();
    void finish();

    core::PedalService& service;
    core::Config& config;
    core::BoardDetector& detector;

    bool active = false;
    int step = 0;
    bool searchStarted = false;

    std::vector<core::PortInfo> ports;
    std::string selectedPort;

    std::vector<core::WindowInfo> windows;
    std::string selectedWindow;
    std::string selectedExecutable;

    std::array<std::array<char, 128>, 4> bindings{};
    KeyCapture capture;
    int capturingRow = -1;
};

}
