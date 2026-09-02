#include <windows.h>

#include <iostream>
#include <memory>
#include <string>

#include "core/Args.h"
#include "core/Logger.h"
#include "core/Paths.h"
#include "core/PedalService.h"
#include "core/SingleInstance.h"
#include "ui/ConsoleFrontend.h"
#include "version.h"


namespace {

void initLogging() {
    auto console = std::make_shared<core::ConsoleSink>();
    console->setMinLevel(core::LogLevel::Debug);
    core::Logger::instance().addSink(console);

    const std::string logPath = core::logFilePath();
    auto file = std::make_shared<core::FileSink>(logPath);
    if (!file->isOpen()) {
        std::cerr << "Не удалось создать лог-файл: " << logPath << std::endl;
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

    initLogging();
    LOG_DEBUG << "Папка данных: " << core::dataDirectory();

    if (parsed.options.showList) {
        ui::printPortsList(static_cast<int>(parsed.options.portCount));
    }

    const std::string iniFile = core::findConfigFile(parsed.options.iniPath);
    if (iniFile.empty()) {
        ui::showFatalMessage("Файл конфигурации не найден.\nРабота невозможна.");
        return 1;
    }

    core::PedalService service(iniFile);
    if (!service.loadConfig()) {
        LOG_ERROR << "Конфигурация не загружена. Работа невозможна.";
        ui::showFatalMessage("Конфигурация не загружена.\nРабота невозможна.");
        return 1;
    }

    const core::SingleInstance instance("pedal-buttons");
    if (!instance.acquired()) {
        LOG_ERROR << "Приложение уже запущено. Второй экземпляр займёт тот же COM-порт.";
        ui::showFatalMessage("Pedal Buttons уже запущен.");
        return 1;
    }

    service.run("COM" + std::to_string(parsed.options.port));

    return 0;
}
