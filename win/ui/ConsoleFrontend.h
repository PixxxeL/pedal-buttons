#pragma once

#include <functional>
#include <string>


namespace ui {

bool attachParentConsole();
void printPortsList();
void showFatalMessage(const std::string& text);
void installInterruptHandler(std::function<void()> onInterrupt);

}
