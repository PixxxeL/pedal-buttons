#pragma once

#include <string>
#include <vector>


namespace core {

struct PortInfo {
    std::string name;
    std::string description;
    std::string hardwareId;
};

std::vector<PortInfo> listPorts();

}
