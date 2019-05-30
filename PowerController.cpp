#include "PowerController.h"

PowerController::PowerController(std::string portName, int baudrate, boost::asio::io_service* io) : baudrate(baudrate), temperature(0), portName(portName), port(*io)
{  
	port.open(this->portName);
	port.set_option(boost::asio::serial_port_base::baud_rate(this->baudrate));
	port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
	port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

	auto data = std::string("SOUT1\r");
	boost::asio::write(port, boost::asio::buffer(data, 10));
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

	data = std::string("SABC 0\r");
	boost::asio::write(port, boost::asio::buffer(data, sizeof(data)));
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	port.close();
}

bool PowerController::UpdatePowerState(HardwareState update)
{ 
	if (update.DesiredTemperature == temperature) 
	{
		return true;
	}

	this->temperature = update.DesiredTemperature; 

	try 
	{    
		std::thread t([this]{ this->SetCurrentAndVoltage(); });
		t.detach();

		return true;
	}
	catch (boost::system::system_error e)
	{ 
		spdlog::error("Error in power controller {}", e.what());
		return false;
	} 
}

double PowerController::CalculateVoltageByTemperature(int temperature)
{ 
	return (double)temperature * 0.025643;
}

void PowerController::SetCurrentAndVoltage()
{
	port.open(this->portName);
	port.set_option(boost::asio::serial_port_base::baud_rate(this->baudrate));
	port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
	port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one)); 

	auto data = std::string("SOUT1\r");
	boost::asio::write(port, boost::asio::buffer(data, sizeof(data)));
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

	auto voltage = this->CalculateVoltageByTemperature(this->temperature);
	 
	char s[10]; 
	snprintf(s, 10, "%05.2f", voltage); 
	std::string str(s);
	boost::erase_all(str, ".");

	data = std::string("VOLT 0") + str + std::string("\r"); 
	auto x = data.c_str();
	boost::asio::write(port, boost::asio::buffer(data, sizeof(data)));
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	data = std::string("CURR 00200") + std::string("\r");
	boost::asio::write(port, boost::asio::buffer(data, sizeof(data)));
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); 

	port.close();
}
