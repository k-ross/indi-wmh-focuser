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

extern "C"
{
	#include "DEV_Config.h"
	#include "DRV8825.h"
}

#include "wmh_focuser.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#define FOCUSNAMEF "Waveshare Motor HAT Focuser"

#define MICROSTEPPING 4 // must be set to match the state of the DIP switches on the board

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
	msPerStep = 0;
        reverse = false;
	setVersion(VERSION_MAJOR, VERSION_MINOR);
	setSupportedConnections(CONNECTION_NONE);
	FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_SYNC | FOCUSER_CAN_REVERSE);
	
	DEV_ModuleInit();

	DRV8825_SelectMotor(MOTOR1);
	DRV8825_Stop();
}

IndiWMHFocuser::~IndiWMHFocuser()
{
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
	IUFillNumber(&MotorSpeedN[0], "MOTOR_SPEED", "ms", "%0.0f", 0, 50, 1, 0);
	IUFillNumberVector(&MotorSpeedNP, MotorSpeedN, 1, getDeviceName(), "MOTOR_CONFIG", "Delay Per Step", OPTIONS_TAB, IP_RW, 0, IPS_OK);

	IUFillNumber(&FocusBacklashN[0], "FOCUS_BACKLASH_VALUE", "Steps", "%0.0f", 0, 500, 1, 0);
	IUFillNumberVector(&FocusBacklashNP, FocusBacklashN, 1, getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

	IUFillSwitch(&FocusParkingS[0], "FOCUS_PARKON", "Enable", ISS_ON);
	IUFillSwitch(&FocusParkingS[1], "FOCUS_PARKOFF", "Disable", ISS_OFF);
	IUFillSwitchVector(&FocusParkingSP, FocusParkingS, 2, getDeviceName(), "FOCUS_PARK", "Parking", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

	IUFillSwitch(&FocusResetS[0], "FOCUS_RESET", "Reset", ISS_OFF);
	IUFillSwitchVector(&FocusResetSP, FocusResetS, 1, getDeviceName(), "FOCUS_RESET", "Position Reset", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

	// set default values
	dir = FOCUS_OUTWARD;

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
		defineSwitch(&FocusParkingSP);
		defineSwitch(&FocusResetSP);
		defineNumber(&MotorSpeedNP);
		defineNumber(&FocusBacklashNP);
	}
	else
	{
		deleteProperty(FocusParkingSP.name);
		deleteProperty(FocusResetSP.name);
		deleteProperty(MotorSpeedNP.name);
		deleteProperty(FocusBacklashNP.name);
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
			msPerStep = (int)MotorSpeedN[0].value;
			MotorSpeedNP.s = IPS_OK;
			IDSetNumber(&MotorSpeedNP, "Waveshare Motor HAT Focuser delay per step set to %d ms", (int)MotorSpeedN[0].value);
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
	}
	return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}
bool IndiWMHFocuser::saveConfigItems(FILE *fp)
{
	IUSaveConfigNumber(fp, &PresetNP);
	IUSaveConfigNumber(fp, &MotorSpeedNP);
	IUSaveConfigNumber(fp, &FocusBacklashNP);
	IUSaveConfigSwitch(fp, &FocusParkingSP);

	if (FocusParkingS[0].s == ISS_ON)
		IUSaveConfigNumber(fp, &FocusAbsPosNP);

	return INDI::Focuser::saveConfigItems(fp);
}

IPState IndiWMHFocuser::MoveRelFocuser(FocusDirection direction, uint32_t ticks)
{
	uint32_t targetTicks = FocusAbsPosN[0].value + ((int)ticks * (direction == FOCUS_INWARD ? -1 : 1));
	return MoveAbsFocuser(targetTicks);
}

IPState IndiWMHFocuser::MoveAbsFocuser(uint32_t targetTicks)
{
	if (targetTicks < FocusAbsPosN[0].min || targetTicks > FocusAbsPosN[0].max)
	{
		IDMessage(getDeviceName(), "Requested position is out of range.");
		return IPS_ALERT;
	}

	if (targetTicks == FocusAbsPosN[0].value)
	{
		IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser already in the requested position.");
		return IPS_OK;
	}

	// set focuser busy
	FocusAbsPosNP.s = IPS_BUSY;
	IDSetNumber(&FocusAbsPosNP, NULL);

	// check last motion direction for backlash triggering
	FocusDirection lastdir = dir;

	// set direction
	if (targetTicks > FocusAbsPosN[0].value)
	{
		dir = FOCUS_OUTWARD;
		IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser is moving outward by %u steps", abs((int)targetTicks - (int)FocusAbsPosN[0].value));
	}
	else
	{
		dir = FOCUS_INWARD;
		IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser is moving inward by %u steps", abs((int)targetTicks - (int)FocusAbsPosN[0].value));
	}

	// if direction changed do backlash adjustment - TO DO
	if (FocusBacklashN[0].value != 0 && lastdir != dir && FocusAbsPosN[0].value != 0)
	{
		IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser backlash compensation by %d steps...", (int)FocusBacklashN[0].value);
		StepperMotor(FocusBacklashN[0].value, dir);
	}

	// process targetTicks
	uint32_t ticks = (uint32_t) abs((int)targetTicks - (int)FocusAbsPosN[0].value);

	// GO
	StepperMotor(ticks, dir);

	// update abspos value and status
	FocusAbsPosN[0].value = targetTicks;
	IDSetNumber(&FocusAbsPosNP, "Waveshare Motor HAT Focuser moved to position %d", (int)FocusAbsPosN[0].value);
	FocusAbsPosNP.s = IPS_OK;
	IDSetNumber(&FocusAbsPosNP, NULL);

	return IPS_OK;
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
	
	return IPS_OK;
}

bool IndiWMHFocuser::ReverseFocuser(bool enabled)
{
	reverse = enabled;
	return true;
}

int IndiWMHFocuser::StepperMotor(uint32_t steps, FocusDirection direction)
{
	int dir;

	if (direction == FOCUS_OUTWARD)
	{
		if (reverse)
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
		if (reverse)
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
	DRV8825_TurnStep(dir, (uint32_t) (steps * MICROSTEPPING), msPerStep);
	DRV8825_Stop();

	return 0;
}
