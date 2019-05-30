#pragma once
#include <string>
#include "HardwareState.h"
#include "Enums.h"

class MessageParser
{
public:
	HardwareState ParseToHardwareState(std::string);
	std::string ParseToUpdate(HardwareState);
	std::string ParseToEngineConnectionState(bool connectedToEngines);
	std::string ParseToTemperatureState(std::string temperature); 
	MessageType GetMessageType(std::string message);
	CommandType GetCommandType(std::string message);
};

