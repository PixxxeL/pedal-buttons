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
#include "ui/Autostart.h"
#include "ui/MainView.h"
#include "ui/TrayIcon.h"
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

ui::TrayState trayStateFor(const core::PedalService& service, bool leftFlash, bool rightFlash) {
    if (service.isPaused()) {
        return ui::TrayState::Paused;
    }
    if (!service.isConnected()) {
        return ui::TrayState::Disconnected;
    }
    if (leftFlash) {
        return ui::TrayState::LeftPressed;
    }
    if (rightFlash) {
        return ui::TrayState::RightPressed;
    }
    return ui::TrayState::Connected;
}

std::string trayTooltipFor(const core::PedalService& service) {
    if (service.isPaused()) {
        return "Pedal Buttons — пауза";
    }
    if (service.isConnected()) {
        return "Pedal Buttons — подключено к " + service.portName();
    }
    return "Pedal Buttons — не подключено";
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

    if (uiConfig.autostart() != ui::isAutostartEnabled()) {
        ui::setAutostartEnabled(uiConfig.autostart());
    }

    const std::string configuredPort = uiConfig.port();
    const std::string startPort = configuredPort.empty()
        ? "COM" + std::to_string(parsed.options.port)
        : configuredPort;
    service.start(startPort);

    ui::TrayIcon tray;
    const bool trayReady = uiConfig.useTray() && tray.create();

    bool windowVisible = true;
    if (trayReady && uiConfig.startMinimized()) {
        windowVisible = false;
        window.setVisible(false);
    }
    tray.setWindowVisible(windowVisible);
    tray.setIconVisible(trayReady &&
        !(uiConfig.hideTrayIconWithWindow() && windowVisible));

    ui::MainView view(service, uiConfig, logSink, startPort);

    bool running = true;
    while (running) {
        window.waitEvents();
        view.pumpEvents();

        ui::TrayCommand command = ui::TrayCommand::None;
        while (tray.poll(command)) {
            if (command == ui::TrayCommand::Exit) {
                running = false;
            }
            else if (command == ui::TrayCommand::TogglePause) {
                service.setPaused(!service.isPaused());
            }
            else if (command == ui::TrayCommand::ShowWindow) {
                windowVisible = true;
            }
            else if (command == ui::TrayCommand::ToggleWindow) {
                windowVisible = !windowVisible;
            }
        }

        if (window.shouldClose()) {
            window.clearCloseRequest();
            if (trayReady && uiConfig.closeToTray()) {
                windowVisible = false;
            }
            else {
                running = false;
            }
        }

        if (trayReady && uiConfig.minimizeToTray() && window.isMinimized()) {
            window.restore();
            windowVisible = false;
        }

        if (windowVisible != window.isVisible()) {
            window.setVisible(windowVisible);
            if (windowVisible) {
                window.focus();
            }
            tray.setWindowVisible(windowVisible);
        }

        if (trayReady) {
            tray.setIconVisible(!(uiConfig.hideTrayIconWithWindow() && windowVisible));
            tray.setPaused(service.isPaused());
            tray.setState(trayStateFor(service, view.leftFlashing(), view.rightFlashing()),
                trayTooltipFor(service));
        }

        if (!running) {
            break;
        }

        if (window.isVisible()) {
            window.beginFrame();
            view.draw();
            window.endFrame();
        }
    }

    saveWindowGeometry(iniFile, window.geometry());
    tray.destroy();

    service.stop();
    service.join();
    window.destroy();

    return 0;
}
