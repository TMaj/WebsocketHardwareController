#include "HardwareController.h" 
#include "MessageParser.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>  
#include <spdlog/spdlog.h> 
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp> 
#include <sstream>

#include "ConfigFile.h"  

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp> 

class MessageHandler { 

	websocket::stream<tcp::socket>* websocket;  
	MessageParser messageParser;
	HardwareController* hardwareController;

public:
	MessageHandler(websocket::stream<tcp::socket>* ws,  HardwareController* hardwareController)
	{
		this->websocket = ws;  
		this->hardwareController = hardwareController;
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
				spdlog::info("Handling incoming message");
				spdlog::info(messagestream.str());
				auto stateUpdate = this->messageParser.ParseFromJson(messagestream.str());
				this->hardwareController->UpdateHardwareState(stateUpdate);
				buffer.clear();
			}
		}
		catch (std::exception const& e)
		{
			std::cerr << "Error in message handler: " << e.what() << std::endl; 
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
		std::string portNo = cf.Value("hardware", "portNo");
		std::string baudrate = cf.Value("hardware", "baudrate");
		std::string engine1SlaveNo = cf.Value("hardware", "engine1SlaveNo");
		std::string engine2SlaveNo = cf.Value("hardware", "engine2SlaveNo"); 
		std::string engine1EasyStart = cf.Value("hardware", "engine1EasyStart");
		std::string engine2EasyStart = cf.Value("hardware", "engine2EasyStart");
		std::string engine1EasyStop = cf.Value("hardware", "engine1EasyStop");
		std::string engine2EasyStop = cf.Value("hardware", "engine2EasyStop");
		std::string engine1EasyChange = cf.Value("hardware", "engine1EasyChange");
		std::string engine2EasyChange = cf.Value("hardware", "engine2EasyChange"); 

		HardwareController hardwareController(
			std::stoi(portNo),
			std::stoi(baudrate),
			std::stoi(engine1SlaveNo),
			std::stoi(engine2SlaveNo),
			std::stoi(engine1EasyStart),
			std::stoi(engine2EasyStart),
			std::stoi(engine1EasyStop),
			std::stoi(engine2EasyStop),
			std::stoi(engine1EasyChange),
			std::stoi(engine2EasyChange)
		);

		hardwareController.Connect();


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
						" websocket-client-coro");
				}));

				// Perform the websocket handshake
				ws.handshake(host, "/"); 
		 
				spdlog::info("Successfully connected to websocket server {} on port {}", host, port);

				std::thread t(&MessageHandler::handleMessages, MessageHandler(&ws, &hardwareController));  

				t.join(); 
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