#pragma once

#include <string>
#include <unordered_map>
#include <vector>


namespace core {

class KeySender {
public:
    static void send(const std::vector<std::string>& keys);
    static std::string describe(const std::vector<std::string>& keys);

private:
    static const std::unordered_map<std::string, int>& getKeyMap();
};

}
