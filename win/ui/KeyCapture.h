#pragma once

#include <string>


namespace ui {

class KeyCapture {
public:
    KeyCapture() = default;
    ~KeyCapture();

    KeyCapture(const KeyCapture&) = delete;
    KeyCapture& operator=(const KeyCapture&) = delete;

    bool start();
    void cancel();
    bool isActive() const;
    bool take(std::string& chord);
};

}
