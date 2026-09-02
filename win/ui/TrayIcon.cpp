#include "TrayIcon.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>

#include "../core/Logger.h"


namespace ui {

namespace {

constexpr UINT trayCallbackMessage = WM_APP + 17;
constexpr UINT menuShow = 1;
constexpr UINT menuPause = 2;
constexpr UINT menuExit = 3;

const wchar_t* windowClassName = L"PedalButtonsTray";

std::wstring toWide(const std::string& text) {
    if (text.empty()) {
        return L"";
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return L"";
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
        result.data(), length);
    return result;
}

COLORREF stateColor(TrayState state) {
    switch (state) {
        case TrayState::Connected: return RGB(76, 209, 140);
        case TrayState::LeftPressed: return RGB(76, 209, 140);
        case TrayState::RightPressed: return RGB(250, 174, 56);
        case TrayState::Paused: return RGB(150, 156, 168);
        case TrayState::Disconnected: return RGB(224, 90, 90);
    }
    return RGB(150, 156, 168);
}

HICON buildStateIcon(HICON base, TrayState state) {
    const int size = (std::max)(16, GetSystemMetrics(SM_CXSMICON));

    BITMAPV5HEADER header = { 0 };
    header.bV5Size = sizeof(header);
    header.bV5Width = size;
    header.bV5Height = -size;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;

    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);

    void* pixels = nullptr;
    HBITMAP colorBitmap = CreateDIBSection(memory, reinterpret_cast<BITMAPINFO*>(&header),
        DIB_RGB_COLORS, &pixels, nullptr, 0);

    HICON result = nullptr;

    if (colorBitmap != nullptr && pixels != nullptr) {
        HGDIOBJ previous = SelectObject(memory, colorBitmap);
        std::memset(pixels, 0, static_cast<std::size_t>(size) * size * 4);

        if (base != nullptr) {
            DrawIconEx(memory, 0, 0, base, size, size, 0, nullptr, DI_NORMAL);
        }
        GdiFlush();

        const COLORREF color = stateColor(state);
        const std::uint32_t fill = 0xFF000000u
            | (static_cast<std::uint32_t>(GetRValue(color)) << 16)
            | (static_cast<std::uint32_t>(GetGValue(color)) << 8)
            | static_cast<std::uint32_t>(GetBValue(color));
        const std::uint32_t outline = 0xFF101216u;

        auto* raster = static_cast<std::uint32_t*>(pixels);
        const float radius = size * 0.22f;
        const float centerX = size * 0.5f - 0.5f;
        const float centerY = size * 0.5f - 0.5f;

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                const float dx = x - centerX;
                const float dy = y - centerY;
                const float distance = std::sqrt(dx * dx + dy * dy);
                if (distance <= radius - 1.0f) {
                    raster[y * size + x] = fill;
                }
                else if (distance <= radius) {
                    raster[y * size + x] = outline;
                }
            }
        }

        SelectObject(memory, previous);

        HBITMAP maskBitmap = CreateBitmap(size, size, 1, 1, nullptr);
        if (maskBitmap != nullptr) {
            ICONINFO info = { 0 };
            info.fIcon = TRUE;
            info.hbmColor = colorBitmap;
            info.hbmMask = maskBitmap;
            result = CreateIconIndirect(&info);
            DeleteObject(maskBitmap);
        }
    }

    if (colorBitmap != nullptr) {
        DeleteObject(colorBitmap);
    }
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    return result;
}

}

struct TrayIcon::Impl {
    static Impl* active;
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    HWND window = nullptr;
    HICON base = nullptr;
    std::array<HICON, 5> icons{};
    NOTIFYICONDATAW data = { 0 };
    UINT taskbarCreatedMessage = 0;

    bool added = false;
    bool wantIcon = true;
    bool windowVisible = true;
    bool paused = false;
    TrayState state = TrayState::Disconnected;
    std::string tooltip;
    std::deque<TrayCommand> commands;

    HICON iconFor(TrayState value) const {
        return icons[static_cast<std::size_t>(value)];
    }

    void apply(DWORD message) {
        data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        data.hIcon = iconFor(state);
        const std::wstring tip = toWide(tooltip);
        wcsncpy_s(data.szTip, tip.c_str(), _TRUNCATE);
        Shell_NotifyIconW(message, &data);
    }

    void refresh() {
        if (wantIcon && !added) {
            added = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
            if (added) {
                apply(NIM_MODIFY);
            }
        }
        else if (wantIcon && added) {
            apply(NIM_MODIFY);
        }
        else if (!wantIcon && added) {
            Shell_NotifyIconW(NIM_DELETE, &data);
            added = false;
        }
    }

    void showMenu() {
        HMENU menu = CreatePopupMenu();
        if (menu == nullptr) {
            return;
        }

        AppendMenuW(menu, MF_STRING, menuShow,
            windowVisible ? L"Скрыть окно" : L"Показать окно");
        AppendMenuW(menu, MF_STRING | (paused ? MF_CHECKED : 0), menuPause, L"Пауза");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, menuExit, L"Выход");

        POINT cursor = { 0, 0 };
        GetCursorPos(&cursor);

        SetForegroundWindow(window);
        const int choice = TrackPopupMenu(menu,
            TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
            cursor.x, cursor.y, 0, window, nullptr);
        PostMessageW(window, WM_NULL, 0, 0);
        DestroyMenu(menu);

        if (choice == menuShow) {
            commands.push_back(TrayCommand::ToggleWindow);
        }
        else if (choice == menuPause) {
            commands.push_back(TrayCommand::TogglePause);
        }
        else if (choice == menuExit) {
            commands.push_back(TrayCommand::Exit);
        }
    }
};

TrayIcon::Impl* TrayIcon::Impl::active = nullptr;

LRESULT CALLBACK TrayIcon::Impl::windowProc(HWND window, UINT message,
        WPARAM wparam, LPARAM lparam) {
    if (active != nullptr) {
        if (message == trayCallbackMessage) {
            const UINT event = LOWORD(lparam);
            if (event == WM_LBUTTONUP) {
                active->commands.push_back(TrayCommand::ToggleWindow);
            }
            else if (event == WM_LBUTTONDBLCLK) {
                active->commands.push_back(TrayCommand::ShowWindow);
            }
            else if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU) {
                active->showMenu();
            }
            return 0;
        }

        if (active->taskbarCreatedMessage != 0 && message == active->taskbarCreatedMessage) {
            active->added = false;
            active->refresh();
            return 0;
        }
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

TrayIcon::TrayIcon() : impl(std::make_unique<Impl>()) {}

TrayIcon::~TrayIcon() {
    destroy();
}

bool TrayIcon::create() {
    if (impl->window != nullptr) {
        return true;
    }

    WNDCLASSEXW windowClass = { 0 };
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Impl::windowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = windowClassName;
    RegisterClassExW(&windowClass);

    impl->window = CreateWindowExW(0, windowClassName, L"Pedal Buttons",
        WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (impl->window == nullptr) {
        LOG_ERROR << "Не удалось создать окно для иконки в трее";
        return false;
    }

    Impl::active = impl.get();
    impl->taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    impl->base = static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

    for (std::size_t i = 0; i < impl->icons.size(); i++) {
        impl->icons[i] = buildStateIcon(impl->base, static_cast<TrayState>(i));
        if (impl->icons[i] == nullptr) {
            impl->icons[i] = impl->base;
        }
    }

    impl->data.cbSize = sizeof(impl->data);
    impl->data.hWnd = impl->window;
    impl->data.uID = 1;
    impl->data.uCallbackMessage = trayCallbackMessage;
    impl->data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    impl->data.hIcon = impl->iconFor(impl->state);
    wcsncpy_s(impl->data.szTip, L"Pedal Buttons", _TRUNCATE);

    impl->refresh();
    return true;
}

void TrayIcon::destroy() {
    if (impl->added) {
        Shell_NotifyIconW(NIM_DELETE, &impl->data);
        impl->added = false;
    }

    for (auto& icon : impl->icons) {
        if (icon != nullptr && icon != impl->base) {
            DestroyIcon(icon);
        }
        icon = nullptr;
    }
    if (impl->base != nullptr) {
        DestroyIcon(impl->base);
        impl->base = nullptr;
    }

    if (impl->window != nullptr) {
        DestroyWindow(impl->window);
        impl->window = nullptr;
    }

    if (Impl::active == impl.get()) {
        Impl::active = nullptr;
    }
}

void TrayIcon::setIconVisible(bool visible) {
    if (impl->wantIcon == visible) {
        return;
    }
    impl->wantIcon = visible;
    impl->refresh();
}

void TrayIcon::setState(TrayState state, const std::string& tooltip) {
    if (impl->state == state && impl->tooltip == tooltip) {
        return;
    }
    impl->state = state;
    impl->tooltip = tooltip;
    if (impl->added) {
        impl->apply(NIM_MODIFY);
    }
}

void TrayIcon::setWindowVisible(bool visible) {
    impl->windowVisible = visible;
}

void TrayIcon::setPaused(bool paused) {
    impl->paused = paused;
}

bool TrayIcon::poll(TrayCommand& command) {
    if (impl->commands.empty()) {
        return false;
    }
    command = impl->commands.front();
    impl->commands.pop_front();
    return true;
}

}
