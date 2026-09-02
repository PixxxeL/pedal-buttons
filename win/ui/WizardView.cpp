#include "WizardView.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>

#include "../core/KeyBinding.h"
#include "../core/Logger.h"
#include "Theme.h"


namespace ui {

namespace {

const ImVec4 colorMuted(0.451f, 0.478f, 0.522f, 1.00f);
const ImVec4 colorAccent(0.298f, 0.820f, 0.549f, 1.00f);
const ImVec4 colorPending(0.980f, 0.682f, 0.220f, 1.00f);

const char* stepTitles[] = {
    "Шаг 1 из 3. Плата",
    "Шаг 2 из 3. Приложение",
    "Шаг 3 из 3. Педали"
};

const char* eventTitle(std::size_t index) {
    static const char* titles[] = {
        "Левая, клик", "Левая, удержание", "Правая, клик", "Правая, удержание"
    };
    return index < 4 ? titles[index] : "";
}

template <std::size_t Size>
void copyInto(std::array<char, Size>& buffer, const std::string& text) {
    const std::size_t length = (std::min)(text.size(), buffer.size() - 1);
    std::memcpy(buffer.data(), text.data(), length);
    buffer[length] = '\0';
}

std::string portLabel(const core::PortInfo& port) {
    return "[" + port.name + "] " + port.description;
}

}

WizardView::WizardView(core::PedalService& service, core::Config& config,
        core::BoardDetector& detector)
    : service(service), config(config), detector(detector) {}

bool WizardView::isActive() const {
    return active;
}

void WizardView::begin() {
    active = true;
    step = 0;
    searchStarted = false;
    selectedPort.clear();
    selectedWindow.clear();
    selectedExecutable.clear();
    capturingRow = -1;

    for (auto& binding : bindings) {
        binding[0] = '\0';
    }

    ports = core::listPorts();
    windows = core::listWindows();
}

void WizardView::update() {
    if (!active) {
        return;
    }

    if (capturingRow >= 0) {
        std::string chord;
        if (capture.take(chord)) {
            const std::size_t row = static_cast<std::size_t>(capturingRow);
            if (row < bindings.size()) {
                copyInto(bindings[row], chord);
            }
            capturingRow = -1;
        }
        else if (!capture.isActive()) {
            capturingRow = -1;
        }
    }

    core::DetectionResult found;
    if (detector.take(found) && found.found) {
        selectedPort = found.port;
        ports = core::listPorts();
    }
}

void WizardView::drawBoardStep() {
    ImGui::TextColored(colorMuted,
        "Подключи плату по USB. Приложение найдёт её само по ответу прошивки.");
    ImGui::Spacing();

    if (!searchStarted && !detector.isRunning()) {
        searchStarted = true;
        service.stop();
        service.join();
        detector.start();
    }

    if (detector.isRunning()) {
        ImGui::TextColored(colorPending, "%s", detector.status().c_str());
    }
    else if (!selectedPort.empty()) {
        ImGui::TextColored(colorAccent, "Плата найдена: %s", selectedPort.c_str());
    }
    else {
        ImGui::TextColored(colorMuted, "%s", detector.status().c_str());
    }

    ImGui::Spacing();

    ImGui::BeginDisabled(detector.isRunning());
    if (ImGui::Button("Искать заново")) {
        service.stop();
        service.join();
        detector.start();
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::TextColored(colorMuted, "Либо выбери порт вручную:");

    std::string preview = selectedPort.empty() ? "не выбран" : selectedPort;
    for (const auto& port : ports) {
        if (port.name == selectedPort) {
            preview = portLabel(port);
            break;
        }
    }

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
    if (ImGui::BeginCombo("##wizardPort", preview.c_str())) {
        for (const auto& port : ports) {
            if (ImGui::Selectable(portLabel(port).c_str(), port.name == selectedPort)) {
                selectedPort = port.name;
            }
        }
        ImGui::EndCombo();
    }
}

void WizardView::drawTargetStep() {
    ImGui::TextColored(colorMuted,
        "Выбери приложение, которым будет управлять педаль. "
        "Его окно будет выводиться вперёд перед отправкой клавиш.");
    ImGui::Spacing();

    if (ImGui::Button("Обновить список")) {
        windows = core::listWindows();
    }

    ImGui::SameLine();
    if (ImGui::Button("Пропустить")) {
        selectedWindow.clear();
        selectedExecutable.clear();
    }

    ImGui::SameLine();
    if (selectedWindow.empty()) {
        ImGui::TextColored(colorMuted, "не выбрано — клавиши пойдут в активное окно");
    }
    else {
        ImGui::TextColored(colorAccent, "%s", selectedWindow.c_str());
    }

    ImGui::Spacing();
    ImGui::BeginChild("wizardWindows", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);

    for (const auto& window : windows) {
        ImGui::PushID(reinterpret_cast<const void*>(window.handle));

        const std::string rule = core::makeMatchRule(window);
        const std::string label = (window.process.empty() ? "?" : window.process)
            + "  " + window.title;

        if (ImGui::Selectable(label.c_str(), rule == selectedWindow)) {
            selectedWindow = rule;
            selectedExecutable = window.path;
        }

        ImGui::PopID();
    }

    ImGui::EndChild();
}

void WizardView::drawBindingsStep() {
    ImGui::TextColored(colorMuted,
        "Назначь клавиши на четыре действия. Нажми «Записать» и введи сочетание. "
        "Пустое поле означает, что действие ничего не делает.");
    ImGui::Spacing();

    const float labelWidth = ImGui::CalcTextSize("Правая, удержание").x
        + ImGui::GetStyle().ItemSpacing.x * 2.0f;
    const float buttonWidth = ImGui::CalcTextSize("Нажмите клавиши").x
        + ImGui::GetStyle().FramePadding.x * 2.0f;

    for (std::size_t i = 0; i < bindings.size(); i++) {
        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(eventTitle(i));
        ImGui::SameLine(labelWidth);

        const bool isCapturing = capturingRow == static_cast<int>(i);

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - buttonWidth
            - ImGui::GetStyle().ItemSpacing.x);
        ImGui::BeginDisabled(isCapturing);
        ImGui::InputText("##binding", bindings[i].data(), bindings[i].size());
        ImGui::EndDisabled();

        ImGui::SameLine();

        if (isCapturing) {
            if (ImGui::Button("Нажмите клавиши")) {
                capture.cancel();
                capturingRow = -1;
            }
        }
        else {
            ImGui::BeginDisabled(capturingRow >= 0);
            if (ImGui::Button("Записать")) {
                if (capture.start()) {
                    capturingRow = static_cast<int>(i);
                }
            }
            ImGui::EndDisabled();
        }

        ImGui::PopID();
    }
}

void WizardView::finish() {
    const std::string profile = config.activeProfile();

    if (!selectedPort.empty()) {
        config.setPort(selectedPort);
    }

    config.setWindowMatch(profile, selectedWindow);
    config.setExecutablePath(profile, selectedExecutable);

    const auto& events = core::Config::eventNames();
    for (std::size_t i = 0; i < events.size() && i < bindings.size(); i++) {
        config.setBinding(profile, events[i],
            core::parseKeySequence(std::string(bindings[i].data())));
    }

    config.save();
    LOG_INFO << "Первоначальная настройка завершена";

    if (!selectedPort.empty()) {
        service.start(selectedPort);
    }

    active = false;
}

void WizardView::drawNavigation() {
    ImGui::Separator();

    ImGui::BeginDisabled(step == 0);
    if (ImGui::Button("Назад")) {
        step--;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (step < 2) {
        ImGui::BeginDisabled(step == 0 && selectedPort.empty());
        if (ImGui::Button("Далее")) {
            step++;
        }
        ImGui::EndDisabled();
    }
    else if (ImGui::Button("Готово")) {
        finish();
    }

    ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x * 2.0f);
    if (ImGui::Button("Настроить позже")) {
        config.save();
        if (!selectedPort.empty()) {
            service.start(selectedPort);
        }
        active = false;
    }
}

void WizardView::draw() {
    if (!active) {
        return;
    }

    pushBoldFont();
    ImGui::TextUnformatted("Первоначальная настройка");
    popBoldFont();

    ImGui::TextColored(colorMuted, "%s", stepTitles[(std::min)(step, 2)]);
    ImGui::Separator();
    ImGui::Spacing();

    const float navigationHeight = ImGui::GetFrameHeightWithSpacing()
        + ImGui::GetStyle().ItemSpacing.y * 2.0f;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::BeginChild("wizardBody", ImVec2(0.0f, -navigationHeight));
    ImGui::PopStyleColor();

    if (step == 0) {
        drawBoardStep();
    }
    else if (step == 1) {
        drawTargetStep();
    }
    else {
        drawBindingsStep();
    }

    ImGui::EndChild();

    drawNavigation();
}

}
