#include "LineAssembler.h"


namespace core {

LineAssembler::LineAssembler(std::size_t maxLineLength) : maxLineLength(maxLineLength) {}

void LineAssembler::append(const uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0) {
        return;
    }
    buffer.append(reinterpret_cast<const char*>(data), size);

    if (buffer.size() > maxLineLength && buffer.find('\n') == std::string::npos) {
        buffer.clear();
    }
}

bool LineAssembler::next(std::string& line) {
    const auto position = buffer.find('\n');
    if (position == std::string::npos) {
        return false;
    }

    line.assign(buffer, 0, position);
    buffer.erase(0, position + 1);

    while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }

    return true;
}

void LineAssembler::reset() {
    buffer.clear();
}

}
