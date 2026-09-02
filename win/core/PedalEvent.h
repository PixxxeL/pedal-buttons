#pragma once

#include <chrono>
#include <string>


namespace core {

enum class PedalEventType {
    Connected,
    Disconnected,
    Pedal,
    Unknown,
    Failure
};

struct PedalEvent {
    PedalEventType type = PedalEventType::Unknown;
    std::string name;
    std::string detail;
    std::chrono::system_clock::time_point time = std::chrono::system_clock::now();
};

}
