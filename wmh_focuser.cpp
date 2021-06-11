/*******************************************************************************
 Copyright(c) 2019 Kevin Ross <kevin AT familyross DOT net>

 Based on work done by Radek Kaczorek  <rkaczorek AT gmail DOT com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory>
#include <typeinfo>
#include <iostream>
#include <fstream>

using namespace std;

extern "C"
{
	#include "DEV_Config.h"
	#include "DRV8825.h"
}

#include "config.h"
#include "wmh_focuser.h"

#define FOCUSNAMEF "Waveshare Motor HAT Focuser"

#define MICROSTEPPING 32 // must be set to match the state of the DIP switches on the board

// We declare a pointer to indiWMHFocuser.
std::unique_ptr<IndiWMHFocuser> indiWMHFocuser(new IndiWMHFocuser);

void ISPoll(void *p);
void ISGetProperties(const char *dev)
{
	indiWMHFocuser->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
	indiWMHFocuser->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
	indiWMHFocuser->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
	indiWMHFocuser->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
{
	INDI_UNUSED(dev);
	INDI_UNUSED(name);
	INDI_UNUSED(sizes);
	INDI_UNUSED(blobsizes);
	INDI_UNUSED(blobs);
	INDI_UNUSED(formats);
	INDI_UNUSED(names);
	INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
	indiWMHFocuser->ISSnoopDevice(root);
}

IndiWMHFocuser::IndiWMHFocuser()
{
	_usPerStep = 0;
	_reverse = false;
	setVersion(VERSION_MAJOR, VERSION_MINOR);
	setSupportedConnections(CONNECTION_NONE);
	FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_SYNC | FOCUSER_CAN_REVERSE | FOCUSER_CAN_ABORT);
	
	DEV_ModuleInit();
}

IndiWMHFocuser::~IndiWMHFocuser()
{
	if (_motionThread.joinable())
	{
		_abort = true;
		_motionThread.join();
	}
	DEV_ModuleExit();
}

const char * IndiWMHFocuser::getDefaultName()
{
	return FOCUSNAMEF;
}

bool IndiWMHFocuser::Connect()
{
	IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser connected successfully.");
	return true;
}

bool IndiWMHFocuser::Disconnect()
{
	// park focuser
	if (FocusParkingS[0].s == ISS_ON)
	{
		IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser is parking...");
		MoveAbsFocuser(FocusAbsPosN[0].min);
	}

	// make sure stepper motor is released
	DRV8825_Stop();

	IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser disconnected successfully.");
	return true;
}

bool IndiWMHFocuser::initProperties()
{
	INDI::Focuser::initProperties();

	// options tab
	IUFillNumber(&MotorSpeedN[0], "MOTOR_SPEED", "us", "%0.0f", 0, 1000, 10, 0);
	IUFillNumberVector(&MotorSpeedNP, MotorSpeedN, 1, getDeviceName(), "MOTOR_CONFIG", "Delay Per Step", OPTIONS_TAB, IP_RW, 0, IPS_OK);

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, 500, 1, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&FocusParkingS[0], "FOCUS_PARKON", "Enable", ISS_ON);
	IUFillSwitch(&FocusParkingS[1], "FOCUS_PARKOFF", "Disable", ISS_OFF);
	IUFillSwitchVector(&FocusParkingSP, FocusParkingS, 2, getDeviceName(), "FOCUS_PARK", "Parking", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

	IUFillSwitch(&FocusResetS[0], "FOCUS_RESET", "Reset", ISS_OFF);
	IUFillSwitchVector(&FocusResetSP, FocusResetS, 1, getDeviceName(), "FOCUS_RESET", "Position Reset", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

	IUFillSwitch(&BoardRevisionS[0], "BOARD_REV_ORIG", "Original", ISS_ON);
	IUFillSwitch(&BoardRevisionS[1], "BOARD_REV_2_1", "2.1", ISS_OFF);
	IUFillSwitchVector(&BoardRevisionSP, BoardRevisionS, 2, getDeviceName(), "BOARD_REV", "Board Revision", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

	// set default values
	_dir = FOCUS_OUTWARD;

	FocusAbsPosN[0].value = _loadPosition();
	
	return true;
}

void IndiWMHFocuser::ISGetProperties(const char *dev)
{
	if (dev && strcmp(dev, getDeviceName()))
		return;

	INDI::Focuser::ISGetProperties(dev);

	// addDebugControl();
	return;
}

bool IndiWMHFocuser::updateProperties()
{

	INDI::Focuser::updateProperties();

	if (isConnected())
	{
		defineProperty(&FocusParkingSP);
		defineProperty(&FocusResetSP);
		defineProperty(&MotorSpeedNP);
		defineProperty(&FocusBacklashNP);
		defineProperty(&BoardRevisionSP);
	}
	else
	{
		deleteProperty(FocusParkingSP.name);
		deleteProperty(FocusResetSP.name);
		deleteProperty(MotorSpeedNP.name);
		deleteProperty(FocusBacklashNP.name);
		deleteProperty(BoardRevisionSP.name);
	}

	return true;
}

bool IndiWMHFocuser::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
	// first we check if it's for our device
	if (strcmp(dev, getDeviceName()) == 0)
	{
		// handle step delay
		if (!strcmp(name, MotorSpeedNP.name))
		{
			IUUpdateNumber(&MotorSpeedNP, values, names, n);
			MotorSpeedNP.s = IPS_BUSY;
			IDSetNumber(&MotorSpeedNP, NULL);
			_usPerStep = (int)MotorSpeedN[0].value;
			MotorSpeedNP.s = IPS_OK;
			IDSetNumber(&MotorSpeedNP, "Waveshare Motor HAT Focuser delay per step set to %d us", (int)MotorSpeedN[0].value);
			return true;
		}

		// handle focus backlash
		if (!strcmp(name, FocusBacklashNP.name))
		{
			IUUpdateNumber(&FocusBacklashNP, values, names, n);
			FocusBacklashNP.s = IPS_OK;
			IDSetNumber(&FocusBacklashNP, "Waveshare Motor HAT Focuser backlash set to %d steps", (int)FocusBacklashN[0].value);
			return true;
		}

	}
	return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

bool IndiWMHFocuser::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
	if (!strcmp(dev, getDeviceName()))
	{
		// handle focus reset
		if (!strcmp(name, FocusResetSP.name))
		{
			IUUpdateSwitch(&FocusResetSP, states, names, n);

			if (FocusResetS[0].s == ISS_ON && FocusAbsPosN[0].value == FocusAbsPosN[0].min)
			{
				FocusAbsPosN[0].value = (int)FocusMaxPosN[0].value / 100;
				IDSetNumber(&FocusAbsPosNP, NULL);
				MoveAbsFocuser(0);
			}

			FocusResetS[0].s = ISS_OFF;
			IDSetSwitch(&FocusResetSP, NULL);
			return true;
		}

		// handle parking mode
		if (!strcmp(name, FocusParkingSP.name))
		{
			IUUpdateSwitch(&FocusParkingSP, states, names, n);
			FocusParkingSP.s = IPS_BUSY;
			IDSetSwitch(&FocusParkingSP, NULL);
			FocusParkingSP.s = IPS_OK;
			IDSetSwitch(&FocusParkingSP, NULL);
			return true;
		}
		
		// board revision
		if (!strcmp(name, BoardRevisionSP.name))
		{
			IUUpdateSwitch(&BoardRevisionSP, states, names, n);
			
			if (BoardRevisionS[0].s == ISS_ON)
				DRV8825_SetBoardRevision(REV_ORIG);
			else if (BoardRevisionS[1].s == ISS_ON)
				DRV8825_SetBoardRevision(REV_2_1);
			
			BoardRevisionSP.s = IPS_OK;
			IDSetSwitch(&BoardRevisionSP, NULL);
			return true;
		}
	}
	return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}
bool IndiWMHFocuser::saveConfigItems(FILE *fp)
{
	IUSaveConfigNumber(fp, &PresetNP);
	IUSaveConfigNumber(fp, &MotorSpeedNP);
	IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);
	IUSaveConfigSwitch(fp, &BoardRevisionSP);

	if (FocusParkingS[0].s == ISS_ON)
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

	return INDI::Focuser::saveConfigItems(fp);
}

string IndiWMHFocuser::_getPositionFilename()
{
	return getenv("HOME") + string("/.indi/") + getDeviceName() + ".position";
}

uint32_t IndiWMHFocuser::_loadPosition()
{
	uint32_t pos = 0;
	ifstream file(_getPositionFilename());
	if (file.is_open())
	{
		
		file >> pos;
		file.close();
	}
	
	return pos;
}

void IndiWMHFocuser::_savePosition(uint32_t pos)
{
	ofstream file(_getPositionFilename());
	if (file.is_open())
	{
		file << pos << '\n';
		file.close();
	}
}

IPState IndiWMHFocuser::MoveAbsFocuser(uint32_t targetTicks)
{
	if (targetTicks == FocusAbsPosN[0].value)
		return IPS_OK;
		
    if (!_gotoAbsolute(targetTicks))
        return IPS_ALERT;

    FocusRelPosNP.s       = IPS_BUSY;
	IDSetNumber(&FocusRelPosNP, nullptr);

    return IPS_BUSY;
}

IPState IndiWMHFocuser::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    int32_t newPosition = 0;

	if (ticks == 0)
		return IPS_OK;
		
    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    // Clamp
    newPosition = std::max(0, std::min(static_cast<int32_t>(FocusAbsPosN[0].max), newPosition));
    if (!_gotoAbsolute(newPosition))
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s       = IPS_BUSY;

    return IPS_BUSY;
}

bool IndiWMHFocuser::_gotoAbsolute(uint32_t targetTicks)
{
	if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
	{
		IDMessage(getDeviceName(), "Requested position is out of range.");
		return false;
	}

	if (_motionThread.joinable())
	{
		_abort = true;
		_motionThread.join();
	}

	_abort = false;
	_motionThread = std::thread([this](uint32_t targetPos)
	{
		FocusDirection lastDir = _dir;
		
		// set direction
		if (targetPos > FocusAbsPosN[0].value)
		{
			_dir = FOCUS_OUTWARD;
			IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser is moving outward by %u steps", abs((int)targetPos - (int)FocusAbsPosN[0].value));
		}
		else
		{
			_dir = FOCUS_INWARD;
			IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser is moving inward by %u steps", abs((int)targetPos - (int)FocusAbsPosN[0].value));
		}

		// if direction changed do backlash adjustment - TO DO
		if (FocusBacklashN[0].value != 0 && lastDir != _dir && FocusAbsPosN[0].value != 0)
		{
			IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser backlash compensation by %d steps...", (int)FocusBacklashN[0].value);
			StepperMotor(FocusBacklashN[0].value, _dir);
		}

		uint32_t currentPos = FocusAbsPosN[0].value;
		
		// GO
		int dir = _dir == FOCUS_OUTWARD ? BACKWARD : FORWARD;
		if (_reverse)
		    dir = 1 - dir;
		    
		DRV8825_SelectMotor(MOTOR1);
		DRV8825_SetMicroStep(HARDWARD, "");
		DRV8825_Start(dir);
		while (currentPos != targetPos && !_abort)
		{
			for (int i = 0; i < MICROSTEPPING; i++)
				DRV8825_SingleStep(_usPerStep);
			
			currentPos += _dir == FOCUS_OUTWARD ? 1 : -1;
			
			if (currentPos % 50 == 0)
			{
				FocusAbsPosN[0].value = currentPos;
				FocusAbsPosNP.s = IPS_BUSY;
				IDSetNumber(&FocusAbsPosNP, nullptr);
			}			
		}
		DRV8825_Stop();

		// update abspos value and status
		FocusAbsPosN[0].value = currentPos;
		FocusAbsPosNP.s = IPS_OK;
		IDSetNumber(&FocusAbsPosNP, "Waveshare Motor HAT Focuser moved to position %d", (int)currentPos);
		FocusRelPosNP.s = IPS_OK;
		IDSetNumber(&FocusRelPosNP, nullptr);
		
		_savePosition(currentPos);
	}, targetTicks);

	return true;
}

bool IndiWMHFocuser::SyncFocuser(uint32_t targetTicks)
{
	if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
	{
		IDMessage(getDeviceName(), "Requested position is out of range.");
		return false;
	}
	
	// update abspos value and status
	FocusAbsPosN[0].value = targetTicks;
	IDSetNumber(&FocusAbsPosNP, "Waveshare Motor HAT Focuser synced to position %d", (int)FocusAbsPosN[0].value);
	FocusAbsPosNP.s = IPS_OK;
	IDSetNumber(&FocusAbsPosNP, NULL);
	
	_savePosition(targetTicks);
	return IPS_OK;
}

bool IndiWMHFocuser::ReverseFocuser(bool enabled)
{
	_reverse = enabled;
	return true;
}

bool IndiWMHFocuser::AbortFocuser()
{
	if (_motionThread.joinable())
	{
		_abort = true;
		_motionThread.join();
	}
	
	IDMessage(getDeviceName(), "Aborted focuser motion.");
	return true;
}

int IndiWMHFocuser::StepperMotor(uint32_t steps, FocusDirection direction)
{
	int dir;

	if (direction == FOCUS_OUTWARD)
	{
		if (_reverse)
		{
			//clockwise out
			dir = FORWARD;
		}
		else
		{
			//clockwise in
			dir = BACKWARD;
		}
	}
	else
	{
		if (_reverse)
		{
			//clockwise in
			dir = BACKWARD;
		}
		else
		{
			//clockwise out
			dir = FORWARD;
		}
	}

	DRV8825_SelectMotor(MOTOR1);
	DRV8825_SetMicroStep(HARDWARD, "");
	DRV8825_TurnStep(dir, (uint32_t) (steps * MICROSTEPPING), _usPerStep);
	DRV8825_Stop();

	return 0;
}
