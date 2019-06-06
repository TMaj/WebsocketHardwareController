#include "MessageParser.h"
#include <nlohmann/json.hpp> 
#include <spdlog/spdlog.h>   

using namespace nlohmann;

/// Parsing from engine-specific unit (pulses per second) to common unit (rotations per minute)
double ParseFromPPStoRPM(int pps)
{
	return (double)(pps * 0.006);
}
/// Parsing from common unit to  engine specific unit
int ParseFromRPMtoPPS(double rpm)
{
	return (int)(rpm * 166.666667);
}

HardwareState MessageParser::ParseToHardwareState(std::string message)
{
	try
	{
		json json = json::parse(message); 
		spdlog::info(json.dump());
	
		auto state = HardwareState();  
		state.DesiredTemperature = json["desiredTemperature"].get<int>();
		state.Engine1Direction = json["engine1Direction"].get<int>();
		state.Engine1Speed = ParseFromRPMtoPPS(json["engine1Speed"].get<double>());
		state.Engine2Direction = json["engine2Direction"].get<int>();
		state.Engine2Speed = ParseFromRPMtoPPS(json["engine2Speed"].get<double>());

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
	json["engine1Direction"] = state.Engine1Direction;
	json["engine2Direction"] = state.Engine2Direction;
	json["engine1Speed"] = ParseFromPPStoRPM(state.Engine1Speed);
	json["engine2Speed"] = ParseFromPPStoRPM(state.Engine2Speed);

	return json.dump();
}

std::string MessageParser::ParseToEngineConnectionState(bool connectedToEngines)
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
