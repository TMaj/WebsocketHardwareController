#include "MessageParser.h"
#include <nlohmann/json.hpp> 
#include <spdlog/spdlog.h>   

using namespace nlohmann;

HardwareState MessageParser::ParseToHardwareState(std::string message)
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

		spdlog::info("Parsed message dt: {} e1d: {} e1s:{} e2d: {} e2s: {}", state.DesiredTemperature, state.Engine1Direction, state.Engine1Speed, state.Engine2Direction, state.Engine2Speed);

		return state;
	}
	catch (std::exception ex)
	{
		spdlog::error("Error while parsing message: {}", ex.what());
	} 
}

std::string MessageParser::ParseToUpdate(HardwareState state)
{
	json json;
	json["type"] = "update";
	json["engine1Direction"] = std::to_string(state.Engine1Direction);
	json["engine2Direction"] = std::to_string(state.Engine2Direction);
	json["engine1Speed"] = std::to_string(state.Engine1Speed);
	json["engine2Speed"] = std::to_string(state.Engine2Speed);

	return json.dump();
}

std::string MessageParser::ParseToConnectionsState(bool connectedToEngines)
{
	json j;

	j["type"] = "status";
	j["connectedToEngines"] = connectedToEngines;

	return j.dump();
}

std::string MessageParser::ParseToTemperatureState(std::string temperature)
{
	json j;

	j["type"] = "update";
	j["currentTemperature"] = temperature;

	return j.dump();
}

MessageType MessageParser::GetMessageType(std::string message)
{
	try 
	{
		json json = json::parse(message);
		auto type = json["type"].get<std::string>();
	   
		if (type == "update") 
		{
			return MessageType::Update;
		} 

		if (type == "status")
		{
			return MessageType::Status;
		}

		if (type == "command")
		{
			return MessageType::Command;
		}

		return MessageType::Unknown;
	}
	catch (std::exception ex)
	{
		spdlog::error("Error while parsing message: {}", ex.what());
	}
}

CommandType MessageParser::GetCommandType(std::string message)
{
	try
	{
		json json = json::parse(message);
		auto command = json["command"].get<std::string>();

		if (command == "connect")
		{
			return CommandType::Connect;
		}

		if (command == "disconnect")
		{
			return CommandType::Disconnect;
		}
		  
		return CommandType::Other;
	}
	catch (std::exception ex)
	{
		spdlog::error("Error while parsing message: {}", ex.what());
	}
}
