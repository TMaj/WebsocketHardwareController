#include "HardwareController.h" 

#include <fastech/StdAfx.h>
#include <iostream> 
#include <fastech/FAS_EziMOTIONPlusR.h>
#include <fastech/COMM_Define.h>
#include <fastech/MOTION_DEFINE.h>
#include <fastech/ReturnCodes_Define.h>
#include <spdlog/spdlog.h>  

HardwareController::HardwareController(
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
) : portNo(portNo),
	baudrate((unsigned long)baudrate), 
	engine1SlaveNo(engine1SlaveNo),
	engine2SlaveNo(engine2SlaveNo),
	engine1EasyStart(engine1EasyStart),
	engine2EasyStart(engine2EasyStart),
	engine1EasyStop(engine1EasyStop),
	engine2EasyStop(engine2EasyStop),
	engine1EasyChange(engine1EasyChange),
	engine2EasyChange(engine2EasyChange)
{
	spdlog::info("Created Hardware Controller to connect on portNo {} with baudrate {}", portNo, baudrate);

	//Changeable engines settings 
	this->connected = FALSE;
	this->engine1Direction = 1;
	this->engine2Direction = 1;  
	this->enginesEnabled = false;
	this->engine1Speed = 0;
	this->engine2Speed = 0;
}

HardwareController::~HardwareController()
{
}

void HardwareController::Connect()
{
	if (this->connected)
	{
		return;
	}

	spdlog::info("Hardware Controller :: Connecting on portNo {} with baudrate {}.", this->portNo, this->baudrate);

	// Connect to engines
	if (!FAS_Connect(this->portNo, this->baudrate))
	{
		spdlog::error("Hardware Controller :: Connecting on portNo {} with baudrate {} failed.", this->portNo, this->baudrate);
		return;
	}

	this->SetEnginesEnabled(1);

	this->connected = TRUE;

	spdlog::info("Hardware Controller :: Successfully connected to engines."); 
}

void HardwareController::Disconnect()
{
	if (!this->connected)
	{
		return;
	}

	this->SetEnginesEnabled(0);

	// Disconnect engines
	FAS_Close((byte)this->portNo); 
	
	this->connected = FALSE;

	spdlog::info("Hardware Controller :: disconnected from engines.");
}

void HardwareController::UpdateHardwareState(HardwareState update)
{
	// Set engines parameters
	if (!this->connected)
	{
		spdlog::warn("Hardware Controller :: Attempted to change engine parameters while they were not connected.");
		return;
	}

	this->UpdateEnginesState(update);
}

void HardwareController::UpdateEnginesState(HardwareState update)
{  
	long actualEngine1Speed = 0;
	long actualEngine2Speed = 0;
	FAS_GetActualVel(this->portNo, this->engine1SlaveNo, &actualEngine1Speed);
	FAS_GetActualVel(this->portNo, this->engine2SlaveNo, &actualEngine2Speed);

	this->engine1Speed = actualEngine1Speed;
	this->engine2Speed = actualEngine2Speed;

	spdlog::info("Hardware Controller :: Current engine 1 speed: {} current engine 2 speed {}", actualEngine1Speed, actualEngine2Speed);
	spdlog::info("Hardware Controller :: Updated engine 1 speed: {} current engine 2 speed {}", update.Engine1Speed, update.Engine2Speed);

	bool engine1Starting = this->engine1Speed <= 0 && update.Engine1Speed > 0;
	bool engine1Stopping = this->engine1Speed > 0 && update.Engine1Speed <= 0;

	bool engine2Starting = this->engine2Speed <= 0 && update.Engine2Speed > 0;
	bool engine2Stopping = this->engine2Speed > 0 && update.Engine2Speed <= 0;

	bool engine1SpeedChanged = this->engine1Speed != update.Engine1Speed;
	bool engine2SpeedChanged = this->engine2Speed != update.Engine2Speed;

	int previousEngine1Speed = this->engine1Speed;
	int previousEngine2Speed = this->engine2Speed;

	this->engine1Speed = update.Engine1Speed;
	this->engine2Speed = update.Engine2Speed;
	this->engine1Direction = update.Engine1Direction;
	this->engine2Direction = update.Engine2Direction;

	if (!this->enginesEnabled)
	{
		spdlog::error("Hardware Controller :: Attempted to change engine parameters while they were not enabled."); 
		return;
	} 

	if (engine1Starting && engine2Starting)
	{
		StartEngines();
		return;
	}

	if (engine1Stopping && engine2Stopping)
	{
		StopEngines(previousEngine1Speed, previousEngine2Speed);
		return;
	}

	if (engine1Starting)
	{
		StartEngine(this->engine1SlaveNo);
	}

	if (engine2Starting)
	{
		StartEngine(this->engine2SlaveNo);
	}

	if (engine1Stopping)
	{
		StopEngine(this->engine1SlaveNo, previousEngine1Speed);
	}

	if (engine2Stopping)
	{
		StopEngine(this->engine2SlaveNo, previousEngine2Speed);
	}

	if (engine1SpeedChanged && engine2SpeedChanged && !engine1Stopping && !engine1Starting && !engine2Stopping && !engine2Starting)
	{
		ChangeEnginesSpeed(previousEngine1Speed, previousEngine2Speed);
		return;
	}

	if (engine1SpeedChanged && !engine1Stopping && !engine1Starting)
	{
		ChangeEngineSpeed(this->engine1SlaveNo, previousEngine1Speed);
		return;
	}

	if (engine2SpeedChanged && !engine2Stopping && !engine2Starting)
	{
		ChangeEngineSpeed(this->engine2SlaveNo, previousEngine2Speed);
		return;
	}
}

void HardwareController::SetEnginesEnabled(int enabled)
{
	if (FAS_IsSlaveExist((byte)this->portNo, this->engine1SlaveNo))
	{
		FAS_ServoEnable(this->portNo, this->engine1SlaveNo, enabled);
	}

	if (FAS_IsSlaveExist((byte)this->portNo, this->engine2SlaveNo))
	{
		FAS_ServoEnable(this->portNo, this->engine2SlaveNo, enabled);
	}

	this->enginesEnabled = enabled;
}

void HardwareController::StartEngine(int engineSlaveNo)
{
	int result;
	int engineSpeed;
	int engineDirection;
	bool easyStart;

	if (engineSlaveNo == this->engine1SlaveNo)
	{ 
		engineSpeed = this->engine1Speed;
		engineDirection = this->engine1Direction;
		easyStart = this->engine1EasyStart;
	}
	else 
	{ 
		engineSpeed = this->engine2Speed;
		engineDirection = this->engine2Direction;
		easyStart = this->engine2EasyStart;
	}

	if (!easyStart)
	{
		result = FAS_MoveVelocity(this->portNo, engineSlaveNo, engineSpeed, engineDirection);
	}
	else
	{
		int speed = 0;
		int maxSpeed = engineSpeed;
		int iterations = 200;
		int velocityStep = int(maxSpeed / iterations);
		speed = velocityStep;
		result = FAS_MoveVelocity(this->portNo, engineSlaveNo, speed, engineDirection);

		for (int i = 1; i < iterations; i++)
		{
			speed += velocityStep;
			result = FAS_VelocityOverride(this->portNo, engineSlaveNo, speed);
			Sleep(10);
		} 
	} 
}  

void HardwareController::StartEngines()
{
	int result;

	if (!this->engine1EasyStart && !this->engine2EasyStart)
	{
		result = FAS_MoveVelocity(this->portNo, this->engine1SlaveNo, this->engine1Speed, this->engine1Direction);
		result = FAS_MoveVelocity(this->portNo, this->engine2SlaveNo, this->engine2Speed, this->engine2Direction);
	}
	else
	{
		int speed1 = 0;
		int speed2 = 0;

		int maxSpeed1 = this->engine1Speed;
		int maxSpeed2 = this->engine2Speed;
		
		int iterations = 200;

		int velocityStep1 = int(maxSpeed1 / iterations);
		int velocityStep2 = int(maxSpeed2 / iterations);

		speed1 = velocityStep1;
		speed2 = velocityStep2;

		result = FAS_MoveVelocity(this->portNo, this->engine1SlaveNo, speed1, this->engine1Direction);
		result = FAS_MoveVelocity(this->portNo, this->engine2SlaveNo, speed2, this->engine2Direction);

		for (int i = 1; i < iterations; i++)
		{
			speed1 += velocityStep1;
			speed2 += velocityStep2;
			result = FAS_VelocityOverride(this->portNo, this->engine1SlaveNo, speed1);
			result = FAS_VelocityOverride(this->portNo, this->engine2SlaveNo, speed2);
			Sleep(10);
		} 
	}
}

void HardwareController::StopEngine(int engineSlaveNo, int previousSpeed)
{ 
	int result;
	int engineSpeed;
	int engineDirection;
	bool easyStop;

	if (engineSlaveNo == this->engine1SlaveNo)
	{ 
		engineSpeed = this->engine1Speed;
		engineDirection = this->engine1Direction;
		easyStop = this->engine1EasyStop;
	}
	else
	{ 
		engineSpeed = this->engine2Speed;
		engineDirection = this->engine2Direction;
		easyStop = this->engine2EasyStop;
	}

	if (easyStop)
	{
		result = FAS_MoveStop(this->portNo, engineSlaveNo);
	}
	else
	{ 
		int speed = 0;
		int maxSpeed = previousSpeed;
		int iterations = 200;
		int velocityStep = int(maxSpeed / iterations); 
		speed = maxSpeed; 

		for (int i = 1; i < iterations; i++)
		{
			speed -= velocityStep;
			result = FAS_VelocityOverride(this->portNo, engineSlaveNo, speed);
			Sleep(10);
		} 

		result = FAS_MoveStop(this->portNo, engineSlaveNo);
	}
}

void HardwareController::StopEngines(int previousSpeed1, int previousSpeed2)
{
	int result;

	if (!this->engine1EasyStart && !this->engine2EasyStart)
	{
		result = FAS_MoveStop(this->portNo, this->engine1SlaveNo);
		result = FAS_MoveStop(this->portNo, this->engine2SlaveNo);
	}
	else
	{
		int speed1 = 0;
		int speed2 = 0;

		int maxSpeed1 = previousSpeed1;
		int maxSpeed2 = previousSpeed2;

		int iterations = 200;

		int velocityStep1 = int(maxSpeed1 / iterations);
		int velocityStep2 = int(maxSpeed2 / iterations);

		speed1 = maxSpeed1;
		speed2 = maxSpeed2;

		for (int i = 1; i < iterations; i++)
		{
			speed1 -= velocityStep1;
			speed2 -= velocityStep2;
			result = FAS_VelocityOverride(this->portNo, this->engine1SlaveNo, speed1);
			result = FAS_VelocityOverride(this->portNo, this->engine2SlaveNo, speed2);
			Sleep(10);
		} 

		result = FAS_MoveStop(this->portNo, this->engine1SlaveNo);
		result = FAS_MoveStop(this->portNo, this->engine2SlaveNo);
	}
}

void HardwareController::ChangeEngineSpeed(int engineSlaveNo, int previousSpeed)
{  
	int result;
	int engineSpeed;
	int engineDirection;
	bool easyChange;

	if (engineSlaveNo == this->engine1SlaveNo)
	{
		engineSpeed = this->engine1Speed;
		engineDirection = this->engine1Direction;
		easyChange = this->engine1EasyChange; 
	}
	else
	{
		engineSpeed = this->engine2Speed;
		engineDirection = this->engine2Direction;
		easyChange = this->engine2EasyChange;
	} 

	if (engineSpeed == 0)
	{
		spdlog::warn("Hardware Controller :: Change engine speed method can be used only when engine is already moving.");
	}

	if (!easyChange)
	{
		result = FAS_VelocityOverride(this->portNo, engineSlaveNo, engineSpeed);
	}
	else
	{ 
		int speed = 0;
		int speedChange = engineSpeed - previousSpeed;
		int iterations = 100;
		int velocityStep = int(speedChange / iterations);
		speed = previousSpeed + velocityStep;

		for (int i = 1; i < iterations; i++)
		{ 
			speed += velocityStep; 	
			result = FAS_VelocityOverride(this->portNo, engineSlaveNo, speed);
			Sleep(10);
		}
	}
}

void HardwareController::ChangeEnginesSpeed(int previousSpeed1, int previousSpeed2)
{
	int result;

	if (this->engine1Speed == 0 || this->engine2Speed == 0)
	{
		spdlog::warn("Hardware Controller :: Change engine speed method can be used only when engine is already moving.");
	}

	if (!this->engine1EasyChange && !this->engine2EasyChange)
	{
		result = FAS_VelocityOverride(this->portNo, this->engine1SlaveNo, this->engine1Speed);
		result = FAS_VelocityOverride(this->portNo, this->engine2SlaveNo, this->engine2Speed);
	}
	else
	{
		int speed1 = 0;
		int speed2 = 0;

		int speedChange1 = this->engine1Speed - previousSpeed1;
		int speedChange2 = this->engine2Speed - previousSpeed2;

		int iterations = 100;

		int velocityStep1 = int(speedChange1 / iterations);
		int velocityStep2 = int(speedChange2 / iterations);

		speed1 = previousSpeed1 + velocityStep1;
		speed2 = previousSpeed2 + velocityStep2;

		for (int i = 1; i < iterations; i++)
		{
			speed1 += velocityStep1;
			speed2 += velocityStep2;
			result = FAS_VelocityOverride(this->portNo, this->engine1SlaveNo, speed1);
			result = FAS_VelocityOverride(this->portNo, this->engine2SlaveNo, speed2);
			Sleep(10);
		} 
	}
}
