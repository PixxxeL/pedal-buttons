#include "Args.h"

#include <sstream>
#include <stdexcept>
#include <string>


namespace core {

namespace {

bool parseUnsigned(const std::string& text, unsigned int& out) {
    if (text.empty()) {
        return false;
    }
    try {
        std::size_t consumed = 0;
        const unsigned long value = std::stoul(text, &consumed);
        if (consumed != text.size() || value > 0xFFFFFFFFul) {
            return false;
        }
        out = static_cast<unsigned int>(value);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool matches(const std::string& argument, const char* shortName, const char* longName, std::string& attached) {
    const std::string shortPrefix = std::string("-") + shortName;
    const std::string longPrefix = std::string("--") + longName;

    if (argument == shortPrefix || argument == longPrefix) {
        attached.clear();
        return true;
    }
    if (argument.rfind(longPrefix + "=", 0) == 0) {
        attached = argument.substr(longPrefix.size() + 1);
        return true;
    }
    if (argument.rfind(shortPrefix, 0) == 0 && argument.size() > shortPrefix.size()) {
        attached = argument.substr(shortPrefix.size());
        return true;
    }
    return false;
}

}

ParseResult parseArgs(int argc, char** argv) {
    ParseResult result;

    for (int i = 1; i < argc; i++) {
        const std::string argument = argv[i];
        std::string attached;

        if (argument == "--help" || argument == "-h" || argument == "-?") {
            result.options.showHelp = true;
            return result;
        }

        if (argument == "--list" || argument == "-l") {
            result.options.showList = true;
            continue;
        }

        const bool isPortCount = matches(argument, "c", "portCount", attached);
        const bool isPort = !isPortCount && matches(argument, "p", "port", attached);
        const bool isIni = !isPortCount && !isPort && matches(argument, "i", "ini", attached);

        if (!isPortCount && !isPort && !isIni) {
            continue;
        }

        std::string value = attached;
        if (value.empty()) {
            if (i + 1 >= argc) {
                result.ok = false;
                result.error = "Не указано значение для " + argument;
                return result;
            }
            value = argv[++i];
        }

        if (isIni) {
            result.options.iniPath = value;
            continue;
        }

        unsigned int number = 0;
        if (!parseUnsigned(value, number)) {
            result.ok = false;
            result.error = "Некорректное числовое значение для " + argument + ": " + value;
            return result;
        }

        if (isPortCount) {
            result.options.portCount = number;
        }
        else {
            result.options.port = number;
        }
    }

    if (result.options.portCount < 1 || result.options.portCount > 20) {
        result.ok = false;
        result.error = "portCount должен быть от 1 до 20, получено: "
            + std::to_string(result.options.portCount);
        return result;
    }

    if (result.options.port < 1 || result.options.port > result.options.portCount) {
        result.ok = false;
        result.error = "port должен быть от 1 до " + std::to_string(result.options.portCount)
            + ", получено: " + std::to_string(result.options.port);
        return result;
    }

    return result;
}

std::string helpText() {
    std::ostringstream out;
    out << "Управление ножными кнопками\n\n"
        << "  -h [ --help ]              Показать справку\n"
        << "  -l [ --list ]              Показать доступные сериальные порты\n"
        << "  -c [ --portCount ] <n>     Сколько портов сканировать? (от 1 до 20, по умолчанию 9)\n"
        << "  -p [ --port ] <n>          Номер порта для подключения (от 1 до portCount, по умолчанию 9)\n"
        << "  -i [ --ini ] <path>        Путь к файлу конфигурации .ini\n";
    return out.str();
}

}
