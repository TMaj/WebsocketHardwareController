#include "MessageParser.h"
#include <nlohmann/json.hpp> 
#include <spdlog/spdlog.h>   

using namespace nlohmann;

HardwareState MessageParser::ParseFromJson(std::string message)
{
	try
	{
		json json = json::parse(message); 
		spdlog::info(json.dump());
	
		auto state = HardwareState(); 
		state.DesiredTemperature = std::stoi(json["desiredTemperature"].get<std::string>());
		state.Engine1Direction = std::stoi(json["engine1Direction"].get<std::string>());
		state.Engine1Speed = std::stoi(json["engine1Speed"].get<std::string>());
		state.Engine2Direction = std::stoi(json["engine2Direction"].get<std::string>());
		state.Engine2Speed = std::stoi(json["engine2Speed"].get<std::string>());
	

		spdlog::info("Parsed message {} {} {} {} {}", state.DesiredTemperature, state.Engine1Direction, state.Engine1Speed, state.Engine2Direction, state.Engine2Speed);

		return state;
	}
	catch (std::exception ex)
	{
		spdlog::error("Error while parsing message: {}", ex.what());
	} 
}

std::string MessageParser::ParseToJson(HardwareState state)
{
	return std::string();
}
