#pragma once

#include <iostream>
#include <windows.h>

class Logger
{
public:
	Logger();

	~Logger();

	void LogInfo(const char * info);

	void LogWarn(const char * warning);

	void LogError(const char * error);

	void LogError(const char * error, std::exception exception);

	void LogSuccess(const char * info);


private:
	HANDLE consoleHandle;

	void Log(const char * message, int colorIndex);

	void Log(const char * message, std::exception exception, int colorIndex);
};

