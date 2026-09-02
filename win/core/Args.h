#pragma once

#include <string>


namespace core {

struct Options {
    bool showHelp = false;
    bool showList = false;
    unsigned int portCount = 9;
    unsigned int port = 9;
    std::string iniPath;
};

struct ParseResult {
    Options options;
    bool ok = true;
    std::string error;
};

ParseResult parseArgs(int argc, char** argv);
std::string helpText();

}
