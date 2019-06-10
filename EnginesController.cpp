#include "EnginesController.h" 

#include <fastech/StdAfx.h>
#include <iostream> 
#include <fastech/FAS_EziMOTIONPlusR.h>
#include <fastech/COMM_Define.h>
#include <fastech/MOTION_DEFINE.h>
#include <fastech/ReturnCodes_Define.h>
#include <spdlog/spdlog.h>  

EnginesController::EnginesController(
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
	this->IsConnectedToEngines = FALSE;
	this->engine1Direction = 1;
	this->engine2Direction = 1;
	this->enginesEnabled = false;
	this->engine1Speed = 0;
	this->engine2Speed = 0;
}

EnginesController::~EnginesController()
{
}

bool EnginesController::Connect()
{
	if (this->IsConnectedToEngines)
	{
		spdlog::info("Hardware Controller :: Already connected to engines.");
		return true;
	}

	spdlog::info("Hardware Controller :: Connecting on portNo {} with baudrate {}.", this->portNo, this->baudrate);

	// Connect to engines

	// D E B U G

	//if (false)
	if (!FAS_Connect(this->portNo, this->baudrate))
	{
		spdlog::error("Hardware Controller :: Connecting on portNo {} with baudrate {} failed.", this->portNo, this->baudrate);
		this->IsConnectedToEngines = FALSE;
		return false;
	}

	// D E B U G
	//this->enginesEnabled = true;
	this->SetEnginesEnabled(1);
	this->IsConnectedToEngines = TRUE;

	spdlog::info("Hardware Controller :: Successfully connected to engines.");

	return true;
}

void EnginesController::Disconnect()
{
	if (!this->IsConnectedToEngines)
	{
		return;
	}

	// D E B U G
	//this->enginesEnabled = false;
	this->SetEnginesEnabled(0);
	FAS_Close((byte)this->portNo);
	this->IsConnectedToEngines = FALSE;

	spdlog::info("Hardware Controller :: Disconnected from engines.");
}

bool EnginesController::ToogleConnection(CommandType command)
{
	switch (command)
	{
	case CommandType::Connect:
	{
		//return true;
		this->Connect();
		return this->IsConnectedToEngines;
	}
	case CommandType::Disconnect:
	{
		//return false;
		this->Disconnect();
		return this->IsConnectedToEngines;
	}
	}

	return false;
}

HardwareState EnginesController::UpdateHardwareState(HardwareState update)
{
	// Set engines parameters
	if (!this->IsConnectedToEngines)
	{
		spdlog::warn("Hardware Controller :: Attempted to change engine parameters while they were not connected.");
		return HardwareState();
	}

	this->UpdateEnginesState(update);

	auto returnedState = HardwareState();
	returnedState.Engine1Direction = this->engine1Direction;
	returnedState.Engine2Direction = this->engine2Direction;
	returnedState.Engine1Speed = this->engine1Speed;
	returnedState.Engine2Speed = this->engine2Speed;

	return returnedState;
}

void EnginesController::UpdateEnginesState(HardwareState update)
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

	bool engine1SpeedChanged = abs(this->engine1Speed - update.Engine1Speed) > 3;
	bool engine2SpeedChanged = abs(this->engine2Speed - update.Engine2Speed) > 3;

	bool engine1DirectionChanged = this->engine1Direction != update.Engine1Direction;
	bool engine2DirectionChanged = this->engine2Direction != update.Engine2Direction;

	int previousEngine1Speed = this->engine1Speed;
	int previousEngine2Speed = this->engine2Speed;
	int previousEngine1Direction = this->engine1Direction;
	int previousEngine2Direction = this->engine2Direction;

	this->engine1Speed = update.Engine1Speed;
	this->engine2Speed = update.Engine2Speed;
	this->engine1Direction = update.Engine1Direction;
	this->engine2Direction = update.Engine2Direction;

	if (!this->enginesEnabled)
	{
		spdlog::warn("Hardware Controller :: Attempted to change engine parameters while they were not enabled. Enabling...");
		this->SetEnginesEnabled(true);
	}

	if (engine1Starting && engine2Starting)
	{
		spdlog::warn("Hardware Controller :: Starting both engines...");
		StartEngines();
		return;
	}

	if (engine1Stopping && engine2Stopping)
	{
		spdlog::warn("Hardware Controller :: Stopping both engines...");
		StopEngines(previousEngine1Speed, previousEngine2Speed);
		return;
	}

	if (engine1Starting)
	{
		spdlog::warn("Hardware Controller :: Starting engine 1...");
		StartEngine(this->engine1SlaveNo);
	}

	if (engine2Starting)
	{
		spdlog::warn("Hardware Controller :: Starting engine 2...");
		StartEngine(this->engine2SlaveNo);
	}

	if (engine1Stopping)
	{
		spdlog::warn("Hardware Controller :: Stopping engine 1...");
		StopEngine(this->engine1SlaveNo, previousEngine1Speed);
	}

	if (engine2Stopping)
	{
		spdlog::warn("Hardware Controller :: Stopping engine 2...");
		StopEngine(this->engine2SlaveNo, previousEngine2Speed);
	}

	if (engine1SpeedChanged && engine2SpeedChanged && !engine1Stopping && !engine1Starting && !engine2Stopping && !engine2Starting)
	{
		spdlog::warn("Hardware Controller :: Changing both engines speed...");
		ChangeEnginesSpeed(previousEngine1Speed, previousEngine2Speed, previousEngine1Direction, previousEngine2Direction);
		return;
	}

	if (engine1SpeedChanged && !engine1Stopping && !engine1Starting)
	{
		spdlog::warn("Hardware Controller :: Changing engine 1 speed...");
		ChangeEngineSpeed(this->engine1SlaveNo, previousEngine1Speed, previousEngine1Direction);
		return;
	}

	if (engine2SpeedChanged && !engine2Stopping && !engine2Starting)
	{
		spdlog::warn("Hardware Controller :: Changing engine 2 speed...");
		ChangeEngineSpeed(this->engine2SlaveNo, previousEngine2Speed, previousEngine2Direction);
		return;
	}

	if (engine1DirectionChanged && engine2DirectionChanged) 
	{
		spdlog::warn("Hardware Controller :: Changing both engines direction...");
		ChangeEnginesDirection(previousEngine1Speed, previousEngine2Speed);
		return;
	}

	if (engine1DirectionChanged)
	{
		spdlog::warn("Hardware Controller :: Changing engine 1 direction...");
		ChangeEngineDirection(this->engine1SlaveNo, previousEngine1Speed);
		return;
	}

	if (engine2DirectionChanged)
	{
		spdlog::warn("Hardware Controller :: Changing engine 2 direction...");
		ChangeEngineDirection(this->engine2SlaveNo, previousEngine2Speed);
		return;
	}
}

void EnginesController::SetEnginesEnabled(int enabled)
{
	int result1 = 0;
	int result2 = 0;

	if (!enabled)
	{
		result1 = FAS_MoveStop(this->portNo, this->engine1SlaveNo);
		result2 = FAS_MoveStop(this->portNo, this->engine2SlaveNo);
	}

	if (FAS_IsSlaveExist((byte)this->portNo, this->engine1SlaveNo))
	{
		result1 = FAS_ServoEnable(this->portNo, this->engine1SlaveNo, enabled);
	}

	if (FAS_IsSlaveExist((byte)this->portNo, this->engine2SlaveNo))
	{
		result2 = FAS_ServoEnable(this->portNo, this->engine2SlaveNo, enabled);
	}

	if (result1 == FMM_OK && result2 == FMM_OK)
	{
		spdlog::info("Hardware Controller :: Both engines enabled flag has been sucessfully set to {}", enabled);
		this->enginesEnabled = enabled;
	}
	else
	{
		spdlog::error("Hardware Controller :: Attempt to set engines enabled flag to {} failed", enabled);
		this->enginesEnabled = !enabled;
	}
}

void EnginesController::StartEngine(int engineSlaveNo)
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
		double speed = 0;
		int maxSpeed = engineSpeed;
		int iterations = 100;
		double velocityStep = (double)maxSpeed / iterations;
		speed = velocityStep;
		result = FAS_MoveVelocity(this->portNo, engineSlaveNo, (int)ceil(speed), engineDirection);

		for (int i = 1; i < iterations; i++)
		{
			speed += velocityStep;
			result = FAS_VelocityOverride(this->portNo, engineSlaveNo, (int)ceil(speed));
			Sleep(10);
		}
	}
}

void EnginesController::StartEngines()
{
	int result;

	if (!this->engine1EasyStart && !this->engine2EasyStart)
	{
		result = FAS_MoveVelocity(this->portNo, this->engine1SlaveNo, this->engine1Speed, this->engine1Direction);
		result = FAS_MoveVelocity(this->portNo, this->engine2SlaveNo, this->engine2Speed, this->engine2Direction);
	}
	else
	{
		double speed1 = 0;
		double speed2 = 0;

		int maxSpeed1 = this->engine1Speed;
		int maxSpeed2 = this->engine2Speed;

		int iterations = 100;

		double velocityStep1 = double(maxSpeed1 / (double)iterations);
		double velocityStep2 = double(maxSpeed2 / (double)iterations);

		speed1 = velocityStep1;
		speed2 = velocityStep2;

		result = FAS_MoveVelocity(this->portNo, this->engine1SlaveNo, (int)ceil(speed1), this->engine1Direction);
		result = FAS_MoveVelocity(this->portNo, this->engine2SlaveNo, (int)ceil(speed2), this->engine2Direction);

		for (int i = 1; i < iterations; i++)
		{
			speed1 += velocityStep1;
			speed2 += velocityStep2;
			result = FAS_VelocityOverride(this->portNo, this->engine1SlaveNo, (int)ceil(speed1));
			result = FAS_VelocityOverride(this->portNo, this->engine2SlaveNo, (int)ceil(speed2));
			Sleep(10);
		}
	}
}

void EnginesController::StopEngine(int engineSlaveNo, int previousSpeed)
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
		double speed = 0;
		int maxSpeed = previousSpeed;
		int iterations = 100;
		double velocityStep = double(maxSpeed / (double)iterations);
		speed = maxSpeed;

		for (int i = 1; i < iterations; i++)
		{
			speed -= velocityStep;
			result = FAS_VelocityOverride(this->portNo, engineSlaveNo, (int)ceil(speed));
			Sleep(10);
		}

		result = FAS_MoveStop(this->portNo, engineSlaveNo);
	}
}

void EnginesController::StopEngines(int previousSpeed1, int previousSpeed2)
{
	int result;

	if (!this->engine1EasyStop && !this->engine1EasyStop)
	{
		result = FAS_MoveStop(this->portNo, this->engine1SlaveNo);
		result = FAS_MoveStop(this->portNo, this->engine2SlaveNo);
	}
	else
	{
		double speed1 = 0;
		double speed2 = 0;

		int maxSpeed1 = previousSpeed1;
		int maxSpeed2 = previousSpeed2;

		int iterations = 100;

		double velocityStep1 = double(maxSpeed1 / (double)iterations);
		double velocityStep2 = double(maxSpeed2 / (double)iterations);

		speed1 = maxSpeed1;
		speed2 = maxSpeed2;

		for (int i = 1; i < iterations; i++)
		{
			speed1 -= velocityStep1;
			speed2 -= velocityStep2;
			result = FAS_VelocityOverride(this->portNo, this->engine1SlaveNo, (int)ceil(speed1));
			result = FAS_VelocityOverride(this->portNo, this->engine2SlaveNo, (int)ceil(speed2));
			Sleep(10);
		}

		result = FAS_MoveStop(this->portNo, this->engine1SlaveNo);
		result = FAS_MoveStop(this->portNo, this->engine2SlaveNo);
	}
}

void EnginesController::ChangeEngineSpeed(int engineSlaveNo, int previousSpeed, int previousDirection)
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

	if (previousDirection != engineDirection) 
	{
		this->ChangeEngineDirection(engineSlaveNo, previousSpeed);
		return;
	}

	if (!easyChange)
	{
		result = FAS_VelocityOverride(this->portNo, engineSlaveNo, engineSpeed);
	}
	else
	{
		double speed = 0;
		int speedChange = engineSpeed - previousSpeed;
		int iterations = 100;
		double velocityStep = (double)speedChange / iterations;
		speed = (double)previousSpeed + velocityStep;

		for (int i = 1; i < iterations; i++)
		{
			speed += velocityStep;
			auto s = (int)ceil(speed);
			result = FAS_VelocityOverride(this->portNo, engineSlaveNo, (int)ceil(speed));
			Sleep(10);
		}
	}
}

void EnginesController::ChangeEnginesSpeed(int previousSpeed1, int previousSpeed2, int previousDirection1, int previousDirection2)
{
	int result;

	if (this->engine1Speed == 0 || this->engine2Speed == 0)
	{
		spdlog::warn("Hardware Controller :: Change engine speed method can be used only when engine is already moving.");
	}

	if (previousDirection1 != this->engine1Direction && previousDirection2 != this->engine2Direction)
	{
		this->ChangeEnginesDirection(previousSpeed1, previousSpeed2);
		return;
	}

	if (previousDirection1 != this->engine1Direction || previousDirection2 != this->engine2Direction)
	{
		if (previousDirection1 != this->engine1Direction)
		{
			this->ChangeEngineDirection(this->engine1SlaveNo, previousSpeed1); 
			this->ChangeEngineSpeed(this->engine2SlaveNo, previousSpeed2, previousDirection2);
		}

		if (previousDirection2 != this->engine2Direction)
		{
			this->ChangeEngineDirection(this->engine2SlaveNo, previousSpeed2);
			this->ChangeEngineSpeed(this->engine1SlaveNo, previousSpeed1, previousDirection1);
		}

		return;
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

void EnginesController::ChangeEngineDirection(int engineSlaveNo, int previousSpeed) 
{
	this->StopEngine(engineSlaveNo, previousSpeed);
	Sleep(1500);
	this->StartEngine(engineSlaveNo);
}

void EnginesController::ChangeEnginesDirection(int previousSpeed1, int previousSpeed2)
{ 
	this->StopEngines(previousSpeed1, previousSpeed2);
	Sleep(1500);
	this->StartEngines();
}