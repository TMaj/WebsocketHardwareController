#include "Logger.h"



Logger::Logger()
{
	this->consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
}


Logger::~Logger()
{
}

void Logger::LogInfo(const char* info)
{
	this->Log(info, 11);
}

void Logger::LogWarn(const char* warning)
{
	this->Log(warning, 14);
}

void Logger::LogError(const char* error)
{
	this->Log(error, 12);
}

void Logger::LogError(const char* error, std::exception exception)
{
	this->Log(error, exception, 12);
}

void Logger::LogSuccess(const char* info)
{
	this->Log(info, 10);
}

void Logger::Log(const char * message, int colorIndex)
{
	SetConsoleTextAttribute(this->consoleHandle, colorIndex);
	std::cout << message << std::endl; 
	SetConsoleTextAttribute(this->consoleHandle, 15);
}

void Logger::Log(const char * message, std::exception exception, int colorIndex)
{
	SetConsoleTextAttribute(this->consoleHandle, colorIndex);
	std::cout << message << std::endl;
	std::cout << exception.what() << std::endl;
	SetConsoleTextAttribute(this->consoleHandle, 15);
}
