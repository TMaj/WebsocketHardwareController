#pragma once    
#include "HardwareState.h"
#include "Enums.h"  
#include <math.h> 

typedef unsigned char BYTE;

class EnginesController
{
public: 
	EnginesController(
		int portNo,
		int baudrate, 
		int engine1SlaveNo,
		int engine2SlaveNo, 
		int engine1EasyStart,
		int engine2EasyStart,
		int engine1EasyStop,
		int engine2EasyStop,
		int engine1EasyChange,
		int engine2EasyChange
		);

	~EnginesController();

	bool Connect(); 
	void Disconnect();

	HardwareState UpdateHardwareState(HardwareState hardwareState);
	void UpdateEnginesState(HardwareState update);
	bool ToogleConnection(CommandType command);

private:
	/// Indicates if software is connected to engines driver
	bool IsConnectedToEngines;
	/// Specifies COM port on which connection to engines driver is made
	const BYTE portNo;
	/// Specifies engine 1 slave number
	const BYTE engine1SlaveNo;
	/// Specifies engine 2 slave number
	const BYTE engine2SlaveNo;
	/// Specifies COM connection baudrate
	const unsigned long baudrate;
	/// Indicates if engines are enabled
	bool enginesEnabled;
	/// Speed of engine 1 (in pps)
	int engine1Speed;
	/// Speed of engine 2 (in pps)
	int engine2Speed;
	/// Indicates if engine 1 will start directly or incrementally
	const bool engine1EasyStart;
	/// Indicates if engine 2 will start directly or incrementally
	const bool engine2EasyStart;
	/// Indicates if engine 1 will stop directly or incrementally
	const bool engine1EasyStop;
	/// Indicates if engine 2 will stop directly or incrementally
	const bool engine2EasyStop;
	/// Indicates if engine 1 will change directly or incrementally
	const bool engine1EasyChange;
	/// Indicates if engine 2 will change directly or incrementally
	const bool engine2EasyChange;

	/// 1 - right, 0 - left
	int engine1Direction;
	int engine2Direction;

	void SetEnginesEnabled(int enabled);

	void StartEngine(int engineSlaveNo);
	void StartEngines();

	void StopEngine(int engineSlaveNo, int previousSpeed);
	void StopEngines(int previousSpeed1, int previousSpeed2);
	void ChangeEngineSpeed(int engineSlaveNo, int previousSpeed);
	void ChangeEnginesSpeed(int previousSpeed1, int previousSpeed2);
};