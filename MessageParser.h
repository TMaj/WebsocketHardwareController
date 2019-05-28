#pragma once
#include <string>
#include "HardwareState.h"

class MessageParser
{
public:
	HardwareState ParseFromJson(std::string);
	std::string ParseToJson(HardwareState);
};

