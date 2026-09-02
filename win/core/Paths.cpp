#include "Paths.h"

#include <windows.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

#include "Logger.h"


namespace fs = std::filesystem;

namespace core {

namespace {

const char* configFileName = "pedal-buttons.ini";
const char* logFileName = "pedal-buttons.log";
const char* dataFolderName = "data";
const char* appFolderName = "pedal-buttons";

bool isWritable(const fs::path& directory) {
    std::error_code error;
    fs::create_directories(directory, error);
    if (!fs::exists(directory, error)) {
        return false;
    }
    std::ofstream probe(directory / logFileName, std::ios::out | std::ios::app);
    return probe.is_open();
}

std::string localAppDataDirectory() {
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, "LOCALAPPDATA") != 0 || value == nullptr) {
        return "";
    }
    const std::string result(value);
    std::free(value);
    return result;
}

std::string resolveDataDirectory() {
    const fs::path portable = fs::path(exeDirectory()) / dataFolderName;
    if (isWritable(portable)) {
        return portable.string();
    }

    const std::string localAppData = localAppDataDirectory();
    if (!localAppData.empty()) {
        const fs::path roaming = fs::path(localAppData) / appFolderName;
        if (isWritable(roaming)) {
            return roaming.string();
        }
    }

    return exeDirectory();
}

}

const std::string& exeDirectory() {
    static const std::string directory = [] {
        std::vector<char> buffer(MAX_PATH);
        DWORD length = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        while (length == buffer.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            buffer.resize(buffer.size() * 2);
            length = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        }
        return fs::path(std::string(buffer.data(), length)).parent_path().string();
    }();
    return directory;
}

const std::string& dataDirectory() {
    static const std::string directory = resolveDataDirectory();
    return directory;
}

std::string logFilePath() {
    return (fs::path(dataDirectory()) / logFileName).string();
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

    const fs::path candidates[] = {
        fs::path(dataDirectory()) / configFileName,
        fs::path(exeDirectory()) / configFileName,
        fs::current_path(error) / configFileName
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate, error)) {
            return candidate.string();
        }
    }

    LOG_ERROR << "Файл конфигурации не найден ни в папке данных (" << dataDirectory()
        << "), ни рядом с exe (" << exeDirectory()
        << "), ни в рабочей папке (" << fs::current_path(error).string() << ")";
    return "";
}

}
