#pragma once
#include <boost/asio/serial_port.hpp>  
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>  
#include <spdlog/spdlog.h>  
#include <chrono>
#include <thread>

#include "HardwareState.h"

class PowerController
{
public:
	PowerController(std::string portName, int baudrate, boost::asio::io_service* io);
	bool UpdatePowerState(HardwareState update);

	void SetCurrentAndVoltage();
private:    
	boost::asio::serial_port port;
	//boost::asio::io_service io;
	std::string portName;
	int baudrate;
	int temperature;

	double CalculateVoltageByTemperature(int temperature);
	
}; 