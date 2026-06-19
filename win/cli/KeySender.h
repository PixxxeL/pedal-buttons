#pragma once

#include <string>
#include <vector>
#include <unordered_map>


class KeySender {
private:
    static const std::unordered_map<std::string, int>& getKeyMap();

public:
    static void send(const std::vector<std::string>& keys);
};
