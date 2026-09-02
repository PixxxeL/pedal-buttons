#include "EditorView.h"

#include <imgui.h>

#include <cstring>

#include "../core/KeySender.h"
#include "../core/Logger.h"


namespace ui {

namespace {

constexpr std::chrono::seconds testDelay{2};

const ImVec4 colorMuted(0.451f, 0.478f, 0.522f, 1.00f);
const ImVec4 colorDanger(0.878f, 0.353f, 0.353f, 1.00f);

const char* eventTitle(const std::string& event) {
    if (event == "LEFT_CLICK") {
        return "Левая, клик";
    }
    if (event == "LEFT_HOLD") {
        return "Левая, удержание";
    }
    if (event == "RIGHT_CLICK") {
        return "Правая, клик";
    }
    if (event == "RIGHT_HOLD") {
        return "Правая, удержание";
    }
    return event.c_str();
}

template <std::size_t Size>
void copyInto(std::array<char, Size>& buffer, const std::string& text) {
    const std::size_t length = (std::min)(text.size(), buffer.size() - 1);
    std::memcpy(buffer.data(), text.data(), length);
    buffer[length] = '\0';
}

}

EditorView::EditorView(core::Config& config) : config(config) {
    syncBuffers(true);
}

void EditorView::syncBuffers(bool force) {
    const bool changed = force ||
        syncedProfile != config.activeProfile() ||
        syncedGeneration != config.generation();

    if (!changed) {
        return;
    }

    syncedProfile = config.activeProfile();
    syncedGeneration = config.generation();

    const auto& events = core::Config::eventNames();
    for (std::size_t i = 0; i < events.size() && i < buffers.size(); i++) {
        copyInto(buffers[i], core::formatKeySequence(config.binding(events[i])));
    }

    copyInto(matchBuffer, config.windowMatch());
    copyInto(exeBuffer, config.executablePath());
}

void EditorView::update() {
    if (capturingRow >= 0) {
        std::string chord;
        if (capture.take(chord)) {
            const auto& events = core::Config::eventNames();
            const std::size_t row = static_cast<std::size_t>(capturingRow);
            if (row < events.size() && row < buffers.size()) {
                copyInto(buffers[row], chord);
                config.setBinding(config.activeProfile(), events[row],
                    core::parseKeySequence(chord));
                syncedGeneration = config.generation();
            }
            capturingRow = -1;
        }
        else if (!capture.isActive()) {
            capturingRow = -1;
        }
    }

    if (pendingTest.empty()) {
        return;
    }
    if (std::chrono::steady_clock::now() < testFireAt) {
        return;
    }

    const core::KeySequence keys = config.binding(pendingTest);
    if (!core::isEmpty(keys)) {
        LOG_INFO << "Проверка привязки " << pendingTest << ": "
            << core::formatKeySequence(keys);
        core::KeySender::send(keys);
    }
    pendingTest.clear();
}

void EditorView::drawProfiles() {
    const auto names = config.profileNames();
    const std::string current = config.activeProfile();

    ImGui::TextUnformatted("Профиль");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
    if (ImGui::BeginCombo("##profile", current.c_str())) {
        for (const auto& name : names) {
            const bool selected = name == current;
            if (ImGui::Selectable(name.c_str(), selected)) {
                config.setActiveProfile(name);
                syncBuffers(true);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Добавить")) {
        newProfileName[0] = '\0';
        openNewProfile = true;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(names.size() <= 1);
    if (ImGui::Button("Удалить")) {
        ImGui::OpenPopup("Удалить профиль");
    }
    ImGui::EndDisabled();

    if (ImGui::BeginPopupModal("Удалить профиль", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Удалить профиль \"%s\"?", current.c_str());
        ImGui::TextColored(colorMuted, "Изменение попадёт в файл только после сохранения.");
        ImGui::Spacing();

        if (ImGui::Button("Удалить")) {
            config.removeProfile(current);
            syncBuffers(true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Отмена")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void EditorView::drawNewProfilePopup() {
    if (openNewProfile) {
        ImGui::OpenPopup("Новый профиль");
        openNewProfile = false;
    }

    if (!ImGui::BeginPopupModal("Новый профиль", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::TextUnformatted("Имя профиля");
    ImGui::SetNextItemWidth(280.0f);
    const bool submitted = ImGui::InputText("##name", newProfileName.data(), newProfileName.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);

    const std::string name(newProfileName.data());
    const bool valid = !name.empty();

    ImGui::Spacing();
    ImGui::BeginDisabled(!valid);
    const bool confirmed = ImGui::Button("Создать") || (submitted && valid);
    ImGui::EndDisabled();

    if (confirmed) {
        config.addProfile(name);
        config.setActiveProfile(name);
        syncBuffers(true);
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();
    if (ImGui::Button("Отмена")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void EditorView::drawTarget() {
    const std::string profile = config.activeProfile();

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Окно");
    ImGui::SameLine(ImGui::CalcTextSize("Приложение").x + ImGui::GetStyle().ItemSpacing.x * 2.0f);

    const float pickerWidth = ImGui::CalcTextSize("Выбрать из запущенных").x
        + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - pickerWidth
        - ImGui::GetStyle().ItemSpacing.x);

    if (ImGui::InputText("##match", matchBuffer.data(), matchBuffer.size())) {
        config.setWindowMatch(profile, std::string(matchBuffer.data()));
        syncedGeneration = config.generation();
    }

    ImGui::SameLine();
    if (ImGui::Button("Выбрать из запущенных")) {
        windows = core::listWindows();
        openWindowPicker = true;
    }

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Приложение");
    ImGui::SameLine(ImGui::CalcTextSize("Приложение").x + ImGui::GetStyle().ItemSpacing.x * 2.0f);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - pickerWidth
        - ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::InputText("##exe", exeBuffer.data(), exeBuffer.size())) {
        config.setExecutablePath(profile, std::string(exeBuffer.data()));
        syncedGeneration = config.generation();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(std::strlen(matchBuffer.data()) == 0);
    if (ImGui::Button("Проверить")) {
        const core::ActivationResult result = core::activateTarget(
            std::string(matchBuffer.data()), std::string(exeBuffer.data()));
        LOG_INFO << "Проверка окна: " << core::describeActivation(result);
    }
    ImGui::EndDisabled();

    ImGui::Spacing();
    if (std::strlen(matchBuffer.data()) == 0) {
        ImGui::TextColored(colorMuted,
            "Окно не задано — клавиши уходят в то приложение, которое сейчас в фокусе.");
    }
    else {
        ImGui::TextColored(colorMuted,
            "Перед отправкой клавиш окно выводится на передний план. "
            "Если оно не найдено, запускается указанное приложение.");
    }
}

void EditorView::drawWindowPickerPopup() {
    if (openWindowPicker) {
        ImGui::OpenPopup("Запущенные окна");
        openWindowPicker = false;
    }

    ImGui::SetNextWindowSize(ImVec2(640.0f, 420.0f), ImGuiCond_Appearing);
    if (!ImGui::BeginPopupModal("Запущенные окна", nullptr)) {
        return;
    }

    ImGui::TextColored(colorMuted, "Выбери окно приложения, которым управляет педаль.");
    ImGui::Spacing();

    ImGui::BeginChild("list", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing()),
        ImGuiChildFlags_Borders);

    for (const auto& window : windows) {
        ImGui::PushID(reinterpret_cast<const void*>(window.handle));

        const std::string label = (window.process.empty() ? "?" : window.process)
            + "  —  " + window.title;
        if (ImGui::Selectable(label.c_str())) {
            const std::string profile = config.activeProfile();
            copyInto(matchBuffer, core::makeMatchRule(window));
            config.setWindowMatch(profile, std::string(matchBuffer.data()));

            if (!window.path.empty()) {
                copyInto(exeBuffer, window.path);
                config.setExecutablePath(profile, window.path);
            }

            syncedGeneration = config.generation();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::IsItemHovered() && !window.className.empty()) {
            ImGui::SetTooltip("класс: %s", window.className.c_str());
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    if (ImGui::Button("Отмена")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void EditorView::drawBindings() {
    const auto& events = core::Config::eventNames();
    const std::string profile = config.activeProfile();
    const float labelWidth = ImGui::CalcTextSize("Правая, удержание").x
        + ImGui::GetStyle().ItemSpacing.x * 2.0f;

    for (std::size_t i = 0; i < events.size() && i < buffers.size(); i++) {
        ImGui::PushID(static_cast<int>(i));

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(eventTitle(events[i]));
        ImGui::SameLine(labelWidth);

        const bool isPending = pendingTest == events[i];
        const bool isCapturing = capturingRow == static_cast<int>(i);

        const float testWidth = ImGui::CalcTextSize("Тест через 2 с").x
            + ImGui::GetStyle().FramePadding.x * 2.0f;
        const float captureWidth = ImGui::CalcTextSize("Нажмите клавиши").x
            + ImGui::GetStyle().FramePadding.x * 2.0f;

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - testWidth - captureWidth
            - ImGui::GetStyle().ItemSpacing.x * 2.0f);

        ImGui::BeginDisabled(isCapturing);
        if (ImGui::InputText("##keys", buffers[i].data(), buffers[i].size())) {
            config.setBinding(profile, events[i],
                core::parseKeySequence(std::string(buffers[i].data())));
            syncedGeneration = config.generation();
        }
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

        ImGui::SameLine();

        if (isPending) {
            const auto left = std::chrono::duration_cast<std::chrono::seconds>(
                testFireAt - std::chrono::steady_clock::now()).count() + 1;
            char label[64];
            std::snprintf(label, sizeof(label), "Тест через %lld с", static_cast<long long>(left));
            if (ImGui::Button(label)) {
                pendingTest.clear();
            }
        }
        else {
            ImGui::BeginDisabled(std::strlen(buffers[i].data()) == 0);
            if (ImGui::Button("Тест")) {
                pendingTest = events[i];
                testFireAt = std::chrono::steady_clock::now() + testDelay;
            }
            ImGui::EndDisabled();
        }

        ImGui::PopID();
    }
}

void EditorView::draw() {
    syncBuffers(false);

    if (!config.isLoaded()) {
        ImGui::TextColored(colorDanger, "Конфигурация не загружена");
        return;
    }

    drawProfiles();
    drawNewProfilePopup();

    ImGui::SeparatorText("Целевое приложение");
    drawTarget();
    drawWindowPickerPopup();

    ImGui::SeparatorText("Привязки");
    drawBindings();

    ImGui::Spacing();
    ImGui::TextColored(colorMuted,
        "Плюс — клавиши одновременно (ctrl+r), запятая — по очереди (ctrl+s,enter).");
    ImGui::TextColored(colorMuted,
        "Тест срабатывает через 2 секунды — успеешь переключиться в нужное окно.");
    ImGui::TextColored(colorMuted,
        "Записать — нажми нужное сочетание, оно подставится само. Esc отменяет.");
}

}
