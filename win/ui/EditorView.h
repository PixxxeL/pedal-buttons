#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string>

#include "../core/Config.h"
#include "KeyCapture.h"


namespace ui {

class EditorView {
public:
    explicit EditorView(core::Config& config);

    void draw();
    void update();

private:
    void syncBuffers(bool force);
    void drawProfiles();
    void drawBindings();
    void drawNewProfilePopup();

    core::Config& config;

    std::array<std::array<char, 128>, 4> buffers{};
    std::string syncedProfile;
    std::uint64_t syncedGeneration = 0;

    std::array<char, 64> newProfileName{};
    bool openNewProfile = false;

    std::string pendingTest;
    std::chrono::steady_clock::time_point testFireAt;

    KeyCapture capture;
    int capturingRow = -1;
};

}
