#include "Paths.h"

#include <windows.h>

#include <filesystem>

#include "Logger.h"


namespace fs = std::filesystem;

namespace core {

namespace {

const char* configFileName = "pedal-buttons.ini";

}

std::string exeDirectory() {
    char buffer[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path().string();
}

std::string findConfigFile(const std::string& userPath) {
    std::error_code error;

    if (!userPath.empty()) {
        if (fs::exists(userPath, error)) {
            return userPath;
        }
        LOG_ERROR << "Файл конфигурации не найден: " << userPath;
        return "";
    }

    const fs::path exeDir = exeDirectory();
    const fs::path byExe = exeDir / configFileName;
    if (fs::exists(byExe, error)) {
        return byExe.string();
    }

    const fs::path workingDir = fs::current_path(error);
    const fs::path byWorkingDir = workingDir / configFileName;
    if (fs::exists(byWorkingDir, error)) {
        return byWorkingDir.string();
    }

    LOG_ERROR << "Файл конфигурации не найден ни рядом с exe (" << exeDir.string()
        << "), ни в рабочей папке (" << workingDir.string() << ")";
    return "";
}

}
