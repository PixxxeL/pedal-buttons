#pragma once

#include "../core/Config.h"


namespace ui {

class SettingsView {
public:
    explicit SettingsView(core::Config& config);

    void draw();

private:
    core::Config& config;
};

}
