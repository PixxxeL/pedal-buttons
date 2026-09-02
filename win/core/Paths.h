#pragma once

#include <string>


namespace core {

const std::string& exeDirectory();
const std::string& dataDirectory();
std::string logFilePath();
std::string findConfigFile(const std::string& userPath);

}
