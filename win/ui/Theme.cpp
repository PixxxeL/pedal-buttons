#include "Theme.h"

#include <imgui.h>


namespace ui {

namespace {

ImFont* bold = nullptr;

}

void setBoldFont(void* font) {
    bold = static_cast<ImFont*>(font);
}

bool hasBoldFont() {
    return bold != nullptr;
}

void pushBoldFont() {
    if (bold != nullptr) {
        ImGui::PushFont(bold);
    }
}

void popBoldFont() {
    if (bold != nullptr) {
        ImGui::PopFont();
    }
}

}
