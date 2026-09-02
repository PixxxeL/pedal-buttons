#include "SettingsView.h"

#include <windows.h>
#include <shellapi.h>

#include <imgui.h>

#include <string>

#include "../core/Paths.h"
#include "Autostart.h"


namespace ui {

namespace {

const ImVec4 colorMuted(0.451f, 0.478f, 0.522f, 1.00f);

void openDataDirectory() {
    const std::string& path = core::dataDirectory();
    const int length = MultiByteToWideChar(CP_UTF8, 0, path.data(),
        static_cast<int>(path.size()), nullptr, 0);
    if (length <= 0) {
        return;
    }

    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.data(), static_cast<int>(path.size()),
        wide.data(), length);

    ShellExecuteW(nullptr, L"open", wide.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

bool checkbox(const char* label, bool value) {
    bool current = value;
    ImGui::Checkbox(label, &current);
    return current;
}

}

SettingsView::SettingsView(core::Config& config) : config(config) {}

void SettingsView::draw() {
    ImGui::SeparatorText("Трей");

    const bool useTray = checkbox("Держать иконку в трее", config.useTray());
    if (useTray != config.useTray()) {
        config.setUseTray(useTray);
    }

    ImGui::BeginDisabled(!useTray);

    const bool minimize = checkbox("Сворачивать в трей вместо панели задач", config.minimizeToTray());
    if (minimize != config.minimizeToTray()) {
        config.setMinimizeToTray(minimize);
    }

    const bool close = checkbox("Закрытие окна прячет в трей, а не завершает", config.closeToTray());
    if (close != config.closeToTray()) {
        config.setCloseToTray(close);
    }

    const bool hideIcon = checkbox("Убирать иконку из трея, пока окно открыто",
        config.hideTrayIconWithWindow());
    if (hideIcon != config.hideTrayIconWithWindow()) {
        config.setHideTrayIconWithWindow(hideIcon);
    }

    ImGui::EndDisabled();

    ImGui::SeparatorText("Запуск");

    const bool startMinimized = checkbox("Запускаться свёрнутым", config.startMinimized());
    if (startMinimized != config.startMinimized()) {
        config.setStartMinimized(startMinimized);
    }

    const bool autostart = checkbox("Запускать вместе с Windows", config.autostart());
    if (autostart != config.autostart()) {
        config.setAutostart(autostart);
        setAutostartEnabled(autostart);
    }

    if (config.autostart() != isAutostartEnabled()) {
        ImGui::TextColored(colorMuted,
            "Запись в реестре не совпадает с настройкой — возможно, exe переместили.");
        ImGui::SameLine();
        if (ImGui::Button("Исправить")) {
            setAutostartEnabled(config.autostart());
        }
    }

    ImGui::SeparatorText("Файлы");

    ImGui::TextColored(colorMuted, "Конфигурация: %s", config.path().c_str());
    ImGui::TextColored(colorMuted, "Папка данных: %s", core::dataDirectory().c_str());

    if (ImGui::Button("Открыть папку данных")) {
        openDataDirectory();
    }
}

}
