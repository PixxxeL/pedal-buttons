#pragma once

#include <cstddef>
#include <cstdint>
#include <string>


namespace core {

class LineAssembler {
public:
    explicit LineAssembler(std::size_t maxLineLength = 4096);

    void append(const uint8_t* data, std::size_t size);
    bool next(std::string& line);
    void reset();

private:
    std::size_t maxLineLength;
    std::string buffer;
};

}
