#include <windows.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "core/Args.h"
#include "core/Config.h"
#include "core/Logger.h"
#include "core/Paths.h"
#include "core/PedalService.h"
#include "ui/ConsoleFrontend.h"
#include "version.h"


namespace {

void initLogging(const std::string& fileName) {
    auto console = std::make_shared<core::ConsoleSink>();
    console->setMinLevel(core::LogLevel::Debug);
    core::Logger::instance().addSink(console);

    if (fileName.empty()) {
        return;
    }

    const std::filesystem::path logPath = std::filesystem::current_path() / fileName;
    auto file = std::make_shared<core::FileSink>(logPath.string());
    if (!file->isOpen()) {
        std::cerr << "Не удалось создать лог-файл: " << logPath.string() << std::endl;
        return;
    }
    file->setMinLevel(core::LogLevel::Info);
    core::Logger::instance().addSink(file);
}

}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    const core::ParseResult parsed = core::parseArgs(argc, argv);
    if (!parsed.ok) {
        std::cerr << parsed.error << std::endl << std::endl << core::helpText();
        return 1;
    }
    if (parsed.options.showHelp) {
        std::cout << "Pedal Buttons " << PEDAL_BUTTONS_VERSION << std::endl << std::endl
            << core::helpText();
        return 0;
    }

    initLogging("pedal-buttons.log");

    if (parsed.options.showList) {
        ui::printPortsList(static_cast<int>(parsed.options.portCount));
    }

    const std::string iniFile = core::findConfigFile(parsed.options.iniPath);
    core::Config config;
    if (iniFile.empty() || !config.load(iniFile)) {
        LOG_ERROR << "Конфигурация не загружена. Работа невозможна.";
        ui::showFatalMessage("Конфигурация не загружена.\nРабота невозможна.");
        return 1;
    }

    LOG_INFO << "Загружен конфиг: " << iniFile << ", приложение: " << config.getAppName();

    core::PedalService service(iniFile);
    service.run("COM" + std::to_string(parsed.options.port));

    return 0;
}
