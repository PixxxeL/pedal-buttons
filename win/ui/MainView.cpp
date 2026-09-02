#include "MainView.h"

#include <imgui.h>

#include <algorithm>
#include <utility>



namespace ui {

namespace {

constexpr std::chrono::milliseconds flashDuration{300};
constexpr std::chrono::milliseconds portScanInterval{2000};

const ImVec4 colorClick(0.298f, 0.820f, 0.549f, 1.00f);
const ImVec4 colorHold(0.980f, 0.682f, 0.220f, 1.00f);
const ImVec4 colorOnline(0.298f, 0.820f, 0.549f, 1.00f);
const ImVec4 colorPending(0.980f, 0.682f, 0.220f, 1.00f);
const ImVec4 colorOffline(0.878f, 0.353f, 0.353f, 1.00f);
const ImVec4 colorMuted(0.451f, 0.478f, 0.522f, 1.00f);

ImVec4 levelColor(core::LogLevel level) {
    switch (level) {
        case core::LogLevel::Trace: return ImVec4(0.400f, 0.427f, 0.471f, 1.00f);
        case core::LogLevel::Debug: return ImVec4(0.545f, 0.612f, 0.706f, 1.00f);
        case core::LogLevel::Info: return ImVec4(0.859f, 0.878f, 0.902f, 1.00f);
        case core::LogLevel::Warning: return ImVec4(0.980f, 0.682f, 0.220f, 1.00f);
        case core::LogLevel::Error: return ImVec4(0.929f, 0.420f, 0.400f, 1.00f);
        case core::LogLevel::Fatal: return ImVec4(1.000f, 0.353f, 0.549f, 1.00f);
    }
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
}

void dot(const ImVec4& color, float radius, float gap) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 position = ImGui::GetCursorScreenPos();
    const float lineHeight = ImGui::GetTextLineHeight();
    const ImVec2 center(position.x + radius, position.y + lineHeight * 0.5f);

    drawList->AddCircleFilled(center, radius, ImGui::GetColorU32(color), 24);
    ImGui::Dummy(ImVec2(radius * 2.0f, lineHeight));
    ImGui::SameLine(0.0f, gap);
}

std::string describeAge(std::chrono::system_clock::time_point time) {
    const auto age = std::chrono::system_clock::now() - time;
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(age).count();

    if (seconds < 1) {
        return "только что";
    }
    if (seconds < 60) {
        return std::to_string(seconds) + " с назад";
    }
    if (seconds < 3600) {
        return std::to_string(seconds / 60) + " мин назад";
    }
    return std::to_string(seconds / 3600) + " ч назад";
}

bool endsWith(const std::string& text, const char* suffix) {
    const std::string tail(suffix);
    return text.size() >= tail.size() &&
        text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

std::string portLabel(const core::PortInfo& port) {
    return "[" + port.name + "] " + port.description;
}

ImVec4 mix(const ImVec4& from, const ImVec4& to, float amount) {
    return ImVec4(
        from.x + (to.x - from.x) * amount,
        from.y + (to.y - from.y) * amount,
        from.z + (to.z - from.z) * amount,
        from.w + (to.w - from.w) * amount);
}

float flashAmount(std::chrono::steady_clock::time_point time) {
    const auto elapsed = std::chrono::steady_clock::now() - time;
    if (elapsed < std::chrono::steady_clock::duration::zero() || elapsed >= flashDuration) {
        return 0.0f;
    }
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    return 1.0f - static_cast<float>(elapsedMs) / static_cast<float>(flashDuration.count());
}

bool toggleChip(const char* label, bool& value, const ImVec4& activeColor) {
    const ImVec4 background = value
        ? ImVec4(activeColor.x, activeColor.y, activeColor.z, 0.18f)
        : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
        ImVec2(10.0f, ImGui::GetStyle().FramePadding.y));
    ImGui::PushStyleColor(ImGuiCol_Button, background);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
        ImVec4(activeColor.x, activeColor.y, activeColor.z, 0.28f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,
        ImVec4(activeColor.x, activeColor.y, activeColor.z, 0.38f));
    ImGui::PushStyleColor(ImGuiCol_Text, value ? activeColor : colorMuted);

    const bool clicked = ImGui::Button(label);
    if (clicked) {
        value = !value;
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    return clicked;
}

}

MainView::MainView(core::PedalService& service, core::Config& config,
        std::shared_ptr<core::MemorySink> logSink, std::string initialPort)
    : service(service), config(config), logSink(std::move(logSink)), editor(config), settings(config),
      selectedPort(std::move(initialPort)) {
    refreshPorts(true);
}

void MainView::applyIndicator(const core::PedalEvent& event) {
    PedalIndicator indicator;
    indicator.flashTime = std::chrono::steady_clock::now();
    indicator.kind = endsWith(event.name, "_HOLD") ? FlashKind::Hold : FlashKind::Click;
    indicator.event = event;
    indicator.hasEvent = true;

    if (event.name.rfind("LEFT", 0) == 0) {
        left = indicator;
    }
    else if (event.name.rfind("RIGHT", 0) == 0) {
        right = indicator;
    }
}

void MainView::pumpEvents() {
    core::PedalEvent event;
    while (service.events().pop(event)) {
        if (event.type == core::PedalEventType::Pedal ||
            event.type == core::PedalEventType::Unknown) {
            applyIndicator(event);
        }
    }
}

bool MainView::leftFlashing() const {
    return left.hasEvent && flashAmount(left.flashTime) > 0.0f;
}

bool MainView::rightFlashing() const {
    return right.hasEvent && flashAmount(right.flashTime) > 0.0f;
}

void MainView::refreshLog() {
    const std::uint64_t current = logSink->version();
    if (current == logVersion) {
        return;
    }
    logVersion = current;
    logRecords = logSink->snapshot();
}

void MainView::refreshPorts(bool force) {
    const auto now = std::chrono::steady_clock::now();
    if (!force && now - lastPortScan < portScanInterval) {
        return;
    }
    lastPortScan = now;
    ports = core::listPorts();

    if (selectedPort.empty() && !ports.empty()) {
        selectedPort = ports.front().name;
    }
}

void MainView::drawConnection() {
    const bool connected = service.isConnected();
    const bool running = service.isRunning();
    const float gap = ImGui::GetStyle().ItemInnerSpacing.x;

    if (connected) {
        dot(colorOnline, ImGui::GetTextLineHeight() * 0.34f, gap);
        ImGui::Text("Подключено к %s", service.portName().c_str());
    }
    else if (running) {
        dot(colorPending, ImGui::GetTextLineHeight() * 0.34f, gap);
        ImGui::Text("Подключение к %s", service.portName().c_str());
    }
    else {
        dot(colorOffline, ImGui::GetTextLineHeight() * 0.34f, gap);
        ImGui::TextUnformatted("Не подключено");
    }

    refreshPorts(false);

    std::string preview;
    for (const auto& port : ports) {
        if (port.name == selectedPort) {
            preview = portLabel(port);
            break;
        }
    }
    if (preview.empty()) {
        preview = selectedPort.empty() ? "порты не найдены" : "[" + selectedPort + "]";
    }

    const float buttonWidth = ImGui::CalcTextSize("Обновить список").x
        + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth
        - ImGui::GetStyle().ItemSpacing.x);

    if (ImGui::BeginCombo("##port", preview.c_str())) {
        for (const auto& port : ports) {
            const bool selected = port.name == selectedPort;
            if (ImGui::Selectable(portLabel(port).c_str(), selected)) {
                selectedPort = port.name;
                config.setPort(selectedPort);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Обновить список")) {
        refreshPorts(true);
    }

    if (running) {
        if (ImGui::Button("Отключить")) {
            service.stop();
            service.join();
        }
        ImGui::SameLine();
        if (ImGui::Button("Переподключить")) {
            service.restart(selectedPort);
        }
    }
    else {
        ImGui::BeginDisabled(selectedPort.empty());
        if (ImGui::Button("Подключить")) {
            service.start(selectedPort);
        }
        ImGui::EndDisabled();
    }

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x * 2.0f);
    bool autoReconnect = service.autoReconnectEnabled();
    if (toggleChip("Переподключаться автоматически", autoReconnect, colorOnline)) {
        service.setAutoReconnect(autoReconnect);
        config.setAutoReconnect(autoReconnect);
    }

    ImGui::SameLine();
    bool paused = service.isPaused();
    if (toggleChip("Пауза", paused, colorHold)) {
        service.setPaused(paused);
    }
}

void MainView::drawPedalCard(const char* id, const char* title,
        const PedalIndicator& indicator, float width) {
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImVec4 accent = indicator.kind == FlashKind::Hold ? colorHold : colorClick;
    const float amount = indicator.hasEvent ? flashAmount(indicator.flashTime) : 0.0f;

    const ImVec4 tint(accent.x, accent.y, accent.z, 0.20f);
    const ImVec4 background = mix(style.Colors[ImGuiCol_ChildBg], tint, amount);
    const ImVec4 border = mix(style.Colors[ImGuiCol_Border], accent, amount);

    const float lineHeight = ImGui::GetTextLineHeightWithSpacing();
    const ImVec2 size(width, lineHeight * 3.0f + style.WindowPadding.y * 2.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, background);
    ImGui::PushStyleColor(ImGuiCol_Border, border);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f + amount);

    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

    ImGui::TextUnformatted(title);

    if (!indicator.hasEvent) {
        ImGui::TextColored(colorMuted, "нажатий не было");
    }
    else {
        const char* kind = indicator.kind == FlashKind::Hold ? "удержание" : "клик";
        if (indicator.event.type == core::PedalEventType::Unknown) {
            ImGui::TextColored(colorMuted, "%s, нет привязки", kind);
        }
        else {
            ImGui::TextColored(accent, "%s -> %s", kind, indicator.event.detail.c_str());
        }
        ImGui::TextColored(colorMuted, "%s", describeAge(indicator.event.time).c_str());
    }

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void MainView::drawIndicators() {
    const float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    drawPedalCard("##left", "Левая педаль", left, width);
    ImGui::SameLine();
    drawPedalCard("##right", "Правая педаль", right, width);
}

void MainView::drawLog() {
    refreshLog();

    static const char* names[] = { "trace", "debug", "info", "warning", "error", "fatal" };
    for (int i = 0; i < 6; i++) {
        if (i > 0) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        }
        toggleChip(names[i], levelFilter[i], levelColor(static_cast<core::LogLevel>(i)));
    }

    const float buttons = ImGui::CalcTextSize("Копировать").x + ImGui::CalcTextSize("Очистить").x
        + ImGui::GetStyle().FramePadding.x * 4.0f + ImGui::GetStyle().ItemSpacing.x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttons - ImGui::GetStyle().WindowPadding.x);

    if (ImGui::Button("Копировать")) {
        std::string text;
        for (auto it = logRecords.rbegin(); it != logRecords.rend(); ++it) {
            if (levelFilter[static_cast<int>(it->level)]) {
                text += core::formatRecord(*it) + "\n";
            }
        }
        ImGui::SetClipboardText(text.c_str());
    }

    ImGui::SameLine();
    if (ImGui::Button("Очистить")) {
        logSink->clear();
    }

    ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_Borders,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (auto it = logRecords.rbegin(); it != logRecords.rend(); ++it) {
        if (!levelFilter[static_cast<int>(it->level)]) {
            continue;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, levelColor(it->level));
        ImGui::TextUnformatted(core::formatRecord(*it).c_str());
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
}

bool MainView::beginTab(int index, const char* label) {
    const bool selected = index == selectedTab;

    pushBoldFont();
    ImGui::PushStyleColor(ImGuiCol_Text,
        selected ? ImGui::GetStyle().Colors[ImGuiCol_Text] : colorMuted);

    const bool open = ImGui::BeginTabItem(label);

    ImGui::PopStyleColor();
    popBoldFont();

    if (open) {
        selectedTab = index;
    }
    return open;
}

void MainView::drawStatusTab() {
    ImGui::SeparatorText("Подключение");
    drawConnection();

    ImGui::SeparatorText("Педали");
    drawIndicators();

    ImGui::SeparatorText("Журнал");
    drawLog();
}

void MainView::drawFooter() {
    if (!config.isDirty()) {
        return;
    }

    ImGui::Separator();
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(colorPending, "Есть несохранённые изменения");

    ImGui::SameLine();
    if (ImGui::Button("Сохранить")) {
        config.save();
    }

    ImGui::SameLine();
    if (ImGui::Button("Отменить")) {
        config.reload();
        selectedPort = config.port().empty() ? selectedPort : config.port();
        service.setAutoReconnect(config.autoReconnect());
    }
}

void MainView::draw() {
    editor.update();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    const float footerHeight = config.isDirty()
        ? ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y * 2.0f
        : 0.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::BeginChild("content", ImVec2(0.0f, -footerHeight));
    ImGui::PopStyleColor();

    if (ImGui::BeginTabBar("tabs")) {
        if (beginTab(0, "Состояние")) {
            drawStatusTab();
            ImGui::EndTabItem();
        }
        if (beginTab(1, "Привязки")) {
            editor.draw();
            ImGui::EndTabItem();
        }
        if (beginTab(2, "Приложение")) {
            settings.draw();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::EndChild();

    drawFooter();

    ImGui::End();
}

}
