#include "EnginesController.h" 
#include "MessageParser.h"  
#include "PowerController.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>  
#include <mutex>
#include <spdlog/spdlog.h> 
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp> 
#include <boost/asio/serial_port.hpp> 
#include <boost/asio.hpp> 
#include <sstream>

#include "ConfigFile.h"    

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

std::mutex websocket_mutex; 

class MessageHandler { 

	websocket::stream<tcp::socket>* websocket;  
	MessageParser messageParser;
	EnginesController* enginesController;
	PowerController* powerController;

public:
	MessageHandler(websocket::stream<tcp::socket>* ws,  EnginesController* enginesController, PowerController* powerController)
	{
		spdlog::info("Constructing message handler");
		this->websocket = ws;  
		this->enginesController = enginesController;
		this->powerController = powerController;
	} 

	void handleMessages()
	{
		try 
		{
			beast::flat_buffer buffer;
			for (;;)
			{
				// Read a message into our buffer
				this->websocket->read(buffer);

				std::stringstream messagestream;
				messagestream << beast::make_printable(buffer.data());
				auto messageString = messagestream.str();

				spdlog::info("Handling incoming message: {}", messageString);

				auto messageType = this->messageParser.GetMessageType(messageString);

				switch (messageType)
				{
					case MessageType::Update:
					{ 
						auto stateUpdate = this->messageParser.ParseToHardwareState(messageString);
						auto returnedState = this->enginesController->UpdateHardwareState(stateUpdate);
						this->powerController->UpdatePowerState(stateUpdate);
						this->websocket->write(net::buffer(messageParser.ParseToUpdate(returnedState)));
						break;
					}
					case MessageType::Command: 
					{
						auto commandType = this->messageParser.GetCommandType(messageString);
						auto connectionState = this->enginesController->ToogleConnection(commandType); 
						this->websocket->write(net::buffer(messageParser.ParseToEngineConnectionState(connectionState)));
						break;
					}
				} 
				
				buffer.clear();
			}
		}
		catch (std::exception const& e)
		{
			spdlog::error("Error in message handler: {}", e.what()); 
		}
	} 
}; 

class ArduinoHandler {

	websocket::stream<tcp::socket>* websocket;
	MessageParser messageParser; 
	std::string portName;
	int baudrate;

public:
	ArduinoHandler(websocket::stream<tcp::socket>* ws, std::string portName, int arduinoPortBaudrate)
	{
		this->websocket = ws;
		this->portName = portName;
		this->baudrate = arduinoPortBaudrate;
	}

	void handleSerial()
	{
		net::io_service io;
		net::serial_port port(io);

		try
		{  
			port.open(this->portName); 
			port.set_option(net::serial_port_base::baud_rate(this->baudrate)); 
			PurgeComm(port.lowest_layer().native_handle(), PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);

			for (;;)
			{
				char c;
				std::string result;
				for (;;)
				{
					boost::asio::read(port, boost::asio::buffer(&c, 1));

					if (c == '\r' || c== '\n') 
					{
						break;
					}

					result += c; 
				} 
				spdlog::info("arduino: {}", result); 
				websocket->write(net::buffer(messageParser.ParseToTemperatureState(result)));
			}  
		}
		catch (boost::system::system_error e)
		{
			port.close();
			spdlog::error("Error in arduino handler: {}", e.what());
		}
	}
}; 

// Sends a WebSocket message and prints the response
int main(int argc, char** argv)
{
	try {  
		ConfigFile cf("config.txt"); 

		std::string host = cf.Value("connection", "host");
		std::string port = cf.Value("connection", "port");
		std::string enginesPortNo = cf.Value("serial ports", "enginesPortNo");
		std::string enginesPortBaudrate = cf.Value("serial ports", "enginesPortBaudrate");
		std::string arduinoPortName = cf.Value("serial ports", "arduinoPortName");
		std::string arduinoPortBaudrate = cf.Value("serial ports", "arduinoPortBaudrate");
		std::string powerSupplyPortName = cf.Value("serial ports", "powerSupplyPortName");
		std::string powerSupplyPortBaudrate = cf.Value("serial ports", "powerSupplyPortBaudrate");

		std::string engine1SlaveNo = cf.Value("engines", "engine1SlaveNo");
		std::string engine2SlaveNo = cf.Value("engines", "engine2SlaveNo"); 
		std::string engine1EasyStart = cf.Value("engines", "engine1EasyStart");
		std::string engine2EasyStart = cf.Value("engines", "engine2EasyStart");
		std::string engine1EasyStop = cf.Value("engines", "engine1EasyStop");
		std::string engine2EasyStop = cf.Value("engines", "engine2EasyStop");
		std::string engine1EasyChange = cf.Value("engines", "engine1EasyChange");
		std::string engine2EasyChange = cf.Value("engines", "engine2EasyChange"); 

		EnginesController enginesController(
			std::stoi(enginesPortNo),
			std::stoi(enginesPortBaudrate),
			std::stoi(engine1SlaveNo),
			std::stoi(engine2SlaveNo),
			std::stoi(engine1EasyStart),
			std::stoi(engine2EasyStart),
			std::stoi(engine1EasyStop),
			std::stoi(engine2EasyStop),
			std::stoi(engine1EasyChange),
			std::stoi(engine2EasyChange)
		);  

		net::io_service io;
		PowerController powerController(powerSupplyPortName, std::stoi(powerSupplyPortBaudrate), &io);

		for (;;)
		{ 
			try
			{ 
				// The io_context is required for all I/O
				net::io_context ioc;

				// These objects perform our I/O
				tcp::resolver resolver{ ioc };
				websocket::stream<tcp::socket> ws{ ioc }; 
				
				// Look up the domain name
				auto const results = resolver.resolve(host, port);   

				spdlog::info("Attempting to connect websocket server {} on port {}", host, port);
				// Make the connection on the IP address we get from a lookup
				net::connect(ws.next_layer(), results.begin(), results.end());

				// Set a decorator to change the User-Agent of the handshake
				ws.set_option(websocket::stream_base::decorator([](websocket::request_type& req)
				{
					req.set(http::field::user_agent,
						std::string(BOOST_BEAST_VERSION_STRING) +
						" websocket-client-coro" + " wds-hardware-controller");
				}));

				// Perform the websocket handshake
				ws.handshake(host, "/"); 
				
				spdlog::info("Successfully connected to websocket server {} on port {}", host, port);  

				std::thread t(&MessageHandler::handleMessages, MessageHandler(&ws, &enginesController, &powerController));  
				std::thread t2(&ArduinoHandler::handleSerial, ArduinoHandler(&ws, arduinoPortName, std::stoi(arduinoPortBaudrate)));

				t.join(); 
				t2.join();
			}
			catch (std::exception const& e)
			{		
				spdlog::error("Error: {} ", e.what()); 
			} 

			Sleep(1000);
		}
	}
	catch (std::exception const& e)
	{
		spdlog::error("Error: {} ", e.what());
		return EXIT_FAILURE;
	} 

	return EXIT_SUCCESS;
}