#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../core/Config.h"
#include "../core/Logger.h"
#include "../core/PedalEvent.h"
#include "../core/PedalService.h"
#include "../core/PortEnumerator.h"
#include "EditorView.h"
#include "SettingsView.h"
#include "Theme.h"


namespace ui {

enum class FlashKind {
    None,
    Click,
    Hold
};

struct PedalIndicator {
    FlashKind kind = FlashKind::None;
    std::chrono::steady_clock::time_point flashTime;
    core::PedalEvent event;
    bool hasEvent = false;
};

class MainView {
public:
    MainView(core::PedalService& service, core::Config& config,
        std::shared_ptr<core::MemorySink> logSink, std::string initialPort);

    void pumpEvents();
    void draw();

    bool leftFlashing() const;
    bool rightFlashing() const;

private:
    bool beginTab(int index, const char* label);
    void drawStatusTab();
    void drawFooter();
    void drawConnection();
    void drawIndicators();
    void drawPedalCard(const char* id, const char* title, const PedalIndicator& indicator, float width);
    void drawLog();

    void refreshLog();
    void refreshPorts(bool force);
    void applyIndicator(const core::PedalEvent& event);

    core::PedalService& service;
    core::Config& config;
    std::shared_ptr<core::MemorySink> logSink;
    EditorView editor;
    SettingsView settings;
    int selectedTab = 0;

    std::uint64_t logVersion = 0;
    std::vector<core::LogRecord> logRecords;
    bool levelFilter[6] = { false, true, true, true, true, true };

    std::vector<core::PortInfo> ports;
    std::chrono::steady_clock::time_point lastPortScan;
    std::string selectedPort;

    PedalIndicator left;
    PedalIndicator right;
};

}
