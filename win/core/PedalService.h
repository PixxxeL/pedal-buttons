#pragma once

#include <string>


namespace core {

class PedalService {
public:
    explicit PedalService(std::string configPath);

    void run(const std::string& portName);

private:
    static std::string toEventKey(const std::string& line);

    std::string configPath;
};

}
