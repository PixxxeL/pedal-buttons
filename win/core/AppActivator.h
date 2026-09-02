#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace core {

struct WindowInfo {
    std::uintptr_t handle = 0;
    std::string process;
    std::string path;
    std::string className;
    std::string title;
};

enum class ActivationResult {
    NotConfigured,
    Activated,
    AlreadyActive,
    Launched,
    NotFound,
    Failed
};

std::vector<WindowInfo> listWindows();
WindowInfo foregroundWindow();
bool windowMatches(const WindowInfo& window, const std::string& rule);

std::uintptr_t findWindow(const std::string& rule);
bool activateWindow(std::uintptr_t handle);
bool launchApplication(const std::string& path);

ActivationResult activateTarget(const std::string& rule, const std::string& executablePath);

std::string describeActivation(ActivationResult result);
std::string makeMatchRule(const WindowInfo& window);

}
