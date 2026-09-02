#include "AppWindow.h"

#include <windows.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>
#include <string>

#include "../core/Logger.h"


namespace fs = std::filesystem;

namespace ui {

namespace {

constexpr double visibleWaitSeconds = 1.0 / 30.0;
constexpr double hiddenWaitSeconds = 0.2;
constexpr float baseFontSize = 16.0f;

void glfwErrorCallback(int code, const char* description) {
    LOG_ERROR << "GLFW: " << description << " (код " << code << ")";
}

std::string systemFontPath(const char* fileName) {
    char windowsDirectory[MAX_PATH] = { 0 };
    if (GetWindowsDirectoryA(windowsDirectory, MAX_PATH) == 0) {
        return "";
    }
    const fs::path path = fs::path(windowsDirectory) / "Fonts" / fileName;
    std::error_code error;
    return fs::exists(path, error) ? path.string() : "";
}

void loadFont(float scale) {
    ImGuiIO& io = ImGui::GetIO();

    static const ImWchar ranges[] = {
        0x0020, 0x00FF,
        0x2010, 0x205E,
        0x0400, 0x052F,
        0x2DE0, 0x2DFF,
        0xA640, 0xA69F,
        0
    };

    static const char* candidates[] = { "segoeui.ttf", "tahoma.ttf", "arial.ttf" };
    for (const char* candidate : candidates) {
        const std::string path = systemFontPath(candidate);
        if (path.empty()) {
            continue;
        }
        if (io.Fonts->AddFontFromFileTTF(path.c_str(), baseFontSize * scale,
                nullptr, ranges) != nullptr) {
            return;
        }
    }

    LOG_WARNING << "Не найден системный шрифт с кириллицей, текст может отображаться неверно";
    io.Fonts->AddFontDefault();
}

void applyStyle(float scale) {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding = ImVec2(18.0f, 12.0f);
    style.FramePadding = ImVec2(11.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.CellPadding = ImVec2(8.0f, 6.0f);

    style.WindowRounding = 0.0f;
    style.ChildRounding = 10.0f;
    style.FrameRounding = 7.0f;
    style.PopupRounding = 8.0f;
    style.GrabRounding = 7.0f;
    style.ScrollbarRounding = 10.0f;
    style.ScrollbarSize = 13.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.SeparatorTextBorderSize = 1.0f;
    style.SeparatorTextAlign = ImVec2(0.0f, 0.5f);
    style.SeparatorTextPadding = ImVec2(0.0f, 10.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.078f, 0.086f, 0.102f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.105f, 0.117f, 0.137f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.105f, 0.117f, 0.137f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.180f, 0.200f, 0.231f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.902f, 0.914f, 0.933f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.451f, 0.478f, 0.522f, 1.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.145f, 0.161f, 0.188f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.192f, 0.212f, 0.247f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.227f, 0.251f, 0.294f, 1.00f);

    colors[ImGuiCol_Button] = ImVec4(0.176f, 0.196f, 0.231f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.231f, 0.263f, 0.310f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.278f, 0.318f, 0.376f, 1.00f);

    colors[ImGuiCol_Header] = ImVec4(0.180f, 0.451f, 0.365f, 0.65f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.204f, 0.522f, 0.420f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.227f, 0.584f, 0.467f, 0.90f);

    colors[ImGuiCol_CheckMark] = ImVec4(0.298f, 0.820f, 0.549f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.180f, 0.200f, 0.231f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.086f, 0.094f, 0.110f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.220f, 0.243f, 0.282f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.278f, 0.310f, 0.361f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.325f, 0.365f, 0.427f, 1.00f);

    style.ScaleAllSizes(scale);
}

}

struct AppWindow::Impl {
    GLFWwindow* window = nullptr;
    bool imguiReady = false;
    bool visible = true;
    float scale = 1.0f;
};

AppWindow::AppWindow() : impl(std::make_unique<Impl>()) {}

AppWindow::~AppWindow() {
    destroy();
}

bool AppWindow::create(const std::string& title, int width, int height) {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        LOG_ERROR << "Не удалось инициализировать GLFW";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor()) {
        glfwGetMonitorContentScale(monitor, &scaleX, &scaleY);
    }
    impl->scale = scaleX > 0.0f ? scaleX : 1.0f;

    impl->window = glfwCreateWindow(
        static_cast<int>(width * impl->scale),
        static_cast<int>(height * impl->scale),
        title.c_str(), nullptr, nullptr);
    if (impl->window == nullptr) {
        LOG_ERROR << "Не удалось создать окно";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(impl->window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    applyStyle(impl->scale);
    loadFont(impl->scale);

    if (!ImGui_ImplGlfw_InitForOpenGL(impl->window, true)) {
        LOG_ERROR << "Не удалось инициализировать backend GLFW для ImGui";
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 130")) {
        LOG_ERROR << "Не удалось инициализировать backend OpenGL для ImGui";
        return false;
    }

    impl->imguiReady = true;
    impl->visible = true;
    return true;
}

void AppWindow::destroy() {
    if (impl->imguiReady) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        impl->imguiReady = false;
    }

    if (impl->window != nullptr) {
        glfwDestroyWindow(impl->window);
        impl->window = nullptr;
        glfwTerminate();
    }
}

bool AppWindow::shouldClose() const {
    return impl->window == nullptr || glfwWindowShouldClose(impl->window) != 0;
}

void AppWindow::requestClose() {
    if (impl->window != nullptr) {
        glfwSetWindowShouldClose(impl->window, GLFW_TRUE);
    }
}

void AppWindow::setVisible(bool visible) {
    if (impl->window == nullptr || impl->visible == visible) {
        return;
    }
    impl->visible = visible;
    if (visible) {
        glfwShowWindow(impl->window);
    }
    else {
        glfwHideWindow(impl->window);
    }
}

bool AppWindow::isVisible() const {
    return impl->visible;
}

void AppWindow::waitEvents() {
    glfwWaitEventsTimeout(impl->visible ? visibleWaitSeconds : hiddenWaitSeconds);
}

void AppWindow::wakeUp() {
    glfwPostEmptyEvent();
}

void AppWindow::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void AppWindow::endFrame() {
    ImGui::Render();

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(impl->window, &width, &height);
    glViewport(0, 0, width, height);
    glClearColor(0.078f, 0.086f, 0.102f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(impl->window);
}

float AppWindow::scale() const {
    return impl->scale;
}

}
