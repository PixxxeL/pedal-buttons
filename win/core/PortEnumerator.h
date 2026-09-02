#pragma once

#include <set>
#include <string>


namespace core {

std::set<std::string> listPortsFromRegistry();
std::set<std::string> probePorts(int maxPort);

}
