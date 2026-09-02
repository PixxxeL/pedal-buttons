#include <windows.h>

#include <iostream>
#include <memory>
#include <string>

#include "core/Args.h"
#include "core/Logger.h"
#include "core/Paths.h"
#include "core/PedalService.h"
#include "core/SingleInstance.h"
#include "ui/AppWindow.h"
#include "ui/ConsoleFrontend.h"
#include "ui/MainView.h"
#include "version.h"


namespace {

std::shared_ptr<core::MemorySink> initLogging(bool hasConsole) {
    if (hasConsole) {
        auto console = std::make_shared<core::ConsoleSink>();
        console->setMinLevel(core::LogLevel::Debug);
        core::Logger::instance().addSink(console);
    }

    auto memory = std::make_shared<core::MemorySink>();
    memory->setMinLevel(core::LogLevel::Debug);
    core::Logger::instance().addSink(memory);

    const std::string logPath = core::logFilePath();
    auto file = std::make_shared<core::FileSink>(logPath);
    if (file->isOpen()) {
        file->setMinLevel(core::LogLevel::Info);
        core::Logger::instance().addSink(file);
    }
    else if (hasConsole) {
        std::cerr << "Не удалось создать лог-файл: " << logPath << std::endl;
    }

    return memory;
}

void saveWindowGeometry(const std::string& configPath, const core::WindowGeometry& geometry) {
    if (!geometry.hasSize()) {
        return;
    }

    core::Config stored;
    if (!stored.load(configPath)) {
        return;
    }

    stored.setWindowGeometry(geometry);
    stored.save();
}

}

int main(int argc, char** argv) {
    const bool hasConsole = ui::attachParentConsole();

    const core::ParseResult parsed = core::parseArgs(argc, argv);
    if (!parsed.ok) {
        if (hasConsole) {
            std::cerr << parsed.error << std::endl << std::endl << core::helpText();
        }
        else {
            ui::showFatalMessage(parsed.error);
        }
        return 1;
    }
    if (parsed.options.showHelp) {
        if (hasConsole) {
            std::cout << "Pedal Buttons " << PEDAL_BUTTONS_VERSION << std::endl << std::endl
                << core::helpText();
        }
        return 0;
    }

    auto logSink = initLogging(hasConsole);
    LOG_INFO << "Pedal Buttons " << PEDAL_BUTTONS_VERSION;
    LOG_DEBUG << "Папка данных: " << core::dataDirectory();

    if (parsed.options.showList) {
        ui::printPortsList();
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

    core::Config uiConfig;
    uiConfig.load(iniFile);

    const core::SingleInstance instance("pedal-buttons");
    if (!instance.acquired()) {
        LOG_ERROR << "Приложение уже запущено. Второй экземпляр займёт тот же COM-порт.";
        ui::showFatalMessage("Pedal Buttons уже запущен.");
        return 1;
    }

    ui::AppWindow window;
    if (!window.create("Pedal Buttons " PEDAL_BUTTONS_VERSION, uiConfig.windowGeometry())) {
        ui::showFatalMessage("Не удалось создать окно приложения.");
        return 1;
    }

    ui::installInterruptHandler([&window] {
        window.requestClose();
    });

    service.setNotifier(&ui::AppWindow::wakeUp);
    service.setAutoReconnect(uiConfig.autoReconnect());

    const std::string configuredPort = uiConfig.port();
    const std::string startPort = configuredPort.empty()
        ? "COM" + std::to_string(parsed.options.port)
        : configuredPort;
    service.start(startPort);

    ui::MainView view(service, uiConfig, logSink, startPort);
    while (!window.shouldClose()) {
        window.waitEvents();
        view.pumpEvents();

        if (window.isVisible()) {
            window.beginFrame();
            view.draw();
            window.endFrame();
        }
    }

    saveWindowGeometry(iniFile, window.geometry());

    service.stop();
    service.join();
    window.destroy();

    return 0;
}
