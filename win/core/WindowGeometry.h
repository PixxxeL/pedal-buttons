#pragma once


namespace core {

struct WindowGeometry {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool maximized = false;

    bool hasSize() const {
        return width > 0 && height > 0;
    }

    bool hasPosition() const {
        return x != 0 || y != 0;
    }
};

}
