#pragma once

#include <climits>


namespace core {

struct WindowGeometry {
    static constexpr int unsetPosition = INT_MIN;

    int x = unsetPosition;
    int y = unsetPosition;
    int width = 0;
    int height = 0;
    bool maximized = false;

    bool hasSize() const {
        return width > 0 && height > 0;
    }

    bool hasPosition() const {
        return x != unsetPosition && y != unsetPosition;
    }
};

}
