#pragma once

#include <string>
#include "Config.h"


class SerialPortManager
{
private:
	SerialPortManager() = delete;

protected:
	static bool isShowList;
	static unsigned int portCount;
	static unsigned int port;
	static std::string configPath;

public:
	static void run(unsigned int argc, char** argv);
	static void parseArgs(unsigned int argc, char** argv);
	static void readFormPort(std::string portName, const Config& config);
	static std::string printPortsList(int maxPort=9);
	static void validatePortCount(unsigned int value);
	static void validatePort(unsigned int value);
};
