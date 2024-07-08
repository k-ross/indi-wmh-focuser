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

#include "config.h"
#include "wmh_focuser.h"

#if ROCKPI
    #include "mraamotor.h"

    #define M1_ENABLE_PIN 32
    #define M1_DIR_PIN 33
    #define M1_STEP_PIN 35
#elif ROCKPI_ARMBIAN
    #include "gpiomotor.h"

    #define M1_ENABLE_CHIP "gpiochip3"
    #define M1_ENABLE_PIN 16
    #define M1_DIR_CHIP "gpiochip2"
    #define M1_DIR_PIN 12
    #define M1_STEP_CHIP "gpiochip4"
    #define M1_STEP_PIN 5
#elif ODROID_N2
    #include "gpiomotor.h"

    #define M1_ENABLE_CHIP "gpiochip0"
    #define M1_ENABLE_PIN 62
    #define M1_DIR_CHIP "gpiochip0"
    #define M1_DIR_PIN 66
    #define M1_STEP_CHIP "gpiochip0"
    #define M1_STEP_PIN 81
    // #define M1_ENABLE_CHIP "gpiochip0"
    // #define M1_ENABLE_PIN 61
    // #define M1_DIR_CHIP "gpiochip0"
    // #define M1_DIR_PIN 70
    // #define M1_STEP_CHIP "gpiochip0"
    // #define M1_STEP_PIN 71
#else
    #include "gpiomotor.h"
    
    #define M1_ENABLE_CHIP GpioMotor::getPiChip()
    #define M1_ENABLE_PIN 12
    #define M1_DIR_CHIP GpioMotor::getPiChip()
    #define M1_DIR_PIN 13
    #define M1_STEP_CHIP GpioMotor::getPiChip()
    #define M1_STEP_PIN 19
#endif

#define FOCUSNAMEF "Waveshare Motor HAT Focuser"

#define MICROSTEPPING 32 // must be set to match the state of the DIP switches on the board

// We declare a pointer to indiWMHFocuser.
std::unique_ptr<IndiWMHFocuser> indiWMHFocuser(new IndiWMHFocuser);

IndiWMHFocuser::IndiWMHFocuser()
{
    _usPerStep = 0;
    _reverse = false;
    setVersion(VERSION_MAJOR, VERSION_MINOR);
    setSupportedConnections(CONNECTION_NONE);
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_SYNC | FOCUSER_CAN_REVERSE | FOCUSER_CAN_ABORT);

#if ROCKPI
    _motor = make_unique<MraaMotor>(M1_ENABLE_PIN, M1_DIR_PIN, M1_STEP_PIN);
#else
    _motor = make_unique<GpioMotor>(FOCUSNAMEF, 
        M1_ENABLE_CHIP, M1_ENABLE_PIN, 
        M1_DIR_CHIP,    M1_DIR_PIN,
        M1_STEP_CHIP,   M1_STEP_PIN);
#endif
}

IndiWMHFocuser::~IndiWMHFocuser()
{
    if (_motionThread.joinable())
    {
        _abort = true;
        _motionThread.join();
    }
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
    if (FocusParkingSP[0].s == ISS_ON)
    {
        IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser is parking...");
        MoveAbsFocuser(FocusAbsPosN[0].min);
    }

    // make sure stepper motor is released
    _motor->Disable();

    IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser disconnected successfully.");
    return true;
}

bool IndiWMHFocuser::initProperties()
{
    INDI::Focuser::initProperties();

    // options tab
    MotorSpeedNP[0].fill("MOTOR_SPEED", "us", "%.f", 0, 1000, 10, 0);
    MotorSpeedNP.fill(getDeviceName(), "MOTOR_CONFIG", "Delay Per Step", OPTIONS_TAB, IP_RW, 0, IPS_OK);

    FocusBacklashNP[0].fill("FOCUS_BACKLASH_VALUE", "Steps", "%.f", 0, 500, 1, 0);
    FocusBacklashNP.fill(getDeviceName(), "FOCUS_BACKLASH", "Backlash", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    FocusParkingSP[0].fill("FOCUS_PARKON", "Enable", ISS_ON);
    FocusParkingSP[1].fill("FOCUS_PARKOFF", "Disable", ISS_OFF);
    FocusParkingSP.fill(getDeviceName(), "FOCUS_PARK", "Parking", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

    FocusResetSP[0].fill("FOCUS_RESET", "Reset", ISS_OFF);
    FocusResetSP.fill(getDeviceName(), "FOCUS_RESET", "Position Reset", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

    BoardRevisionSP[0].fill("BOARD_REV_ORIG", "Original", ISS_ON);
    BoardRevisionSP[1].fill("BOARD_REV_2_1", "2.1", ISS_OFF);
    BoardRevisionSP.fill(getDeviceName(), "BOARD_REV", "Board Revision", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);
    registerProperty(BoardRevisionSP);
    
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

    loadConfig(true, "BOARD_REV");

    // addDebugControl();
    return;
}

bool IndiWMHFocuser::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineProperty(FocusParkingSP);
        defineProperty(FocusResetSP);
        defineProperty(MotorSpeedNP);
        defineProperty(FocusBacklashNP);
    }
    else
    {
        deleteProperty(FocusParkingSP.getName());
        deleteProperty(FocusResetSP.getName());
        deleteProperty(MotorSpeedNP.getName());
        deleteProperty(FocusBacklashNP.getName());
    }

    return true;
}

bool IndiWMHFocuser::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // first we check if it's for our device
    if (strcmp(dev, getDeviceName()) == 0)
    {
        // handle step delay
        if (MotorSpeedNP.isNameMatch(name))
        {
            MotorSpeedNP.update(values, names, n);
            MotorSpeedNP.setState(IPS_BUSY);
            MotorSpeedNP.apply();
            _usPerStep = (int)MotorSpeedNP[0].getValue();
            MotorSpeedNP.setState(IPS_OK);
            MotorSpeedNP.apply("Waveshare Motor HAT Focuser delay per step set to %d us", (int)MotorSpeedNP[0].getValue());
            return true;
        }

        // handle focus backlash
        if (FocusBacklashNP.isNameMatch(name))
        {
            FocusBacklashNP.update(values, names, n);
            FocusBacklashNP.setState(IPS_OK);
            FocusBacklashNP.apply("Waveshare Motor HAT Focuser backlash set to %d steps", (int)FocusBacklashNP[0].getValue());
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
        if (FocusResetSP.isNameMatch(name))
        {
            FocusResetSP.update(states, names, n);

            if (FocusResetSP[0].getState() == ISS_ON && FocusAbsPosN[0].value == FocusAbsPosN[0].min)
            {
                FocusAbsPosN[0].value = (int)FocusMaxPosN[0].value / 100;
                IDSetNumber(&FocusAbsPosNP, NULL);
                MoveAbsFocuser(0);
            }

            FocusResetSP[0].setState(ISS_OFF);
            FocusResetSP.apply();
            return true;
        }

        // handle parking mode
        if (FocusParkingSP.isNameMatch(name))
        {
            FocusParkingSP.update(states, names, n);
            FocusParkingSP.setState(IPS_BUSY);
            FocusParkingSP.apply();
            FocusParkingSP.setState(IPS_OK);
            FocusParkingSP.apply();
            return true;
        }
        
        // board revision
        if (BoardRevisionSP.isNameMatch(name))
        {
            BoardRevisionSP.update(states, names, n);
            
            if (BoardRevisionSP[0].getState() == ISS_ON)
                _motor->SetBoardRevision(BoardRevision::Original);
            else if (BoardRevisionSP[1].getState() == ISS_ON)
                _motor->SetBoardRevision(BoardRevision::Rev21);

            _motor->Disable();
            
            BoardRevisionSP.setState(IPS_OK);
            BoardRevisionSP.apply();
            return true;
        }
    }
    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}
bool IndiWMHFocuser::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &PresetNP);
    MotorSpeedNP.save(fp);
    FocusBacklashNP.save(fp);
    FocusParkingSP.save(fp);
    BoardRevisionSP.save(fp);

    if (FocusParkingSP[0].getState() == ISS_ON)
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

    FocusRelPosNP.s = IPS_BUSY;
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

        auto dir = _dir == FOCUS_OUTWARD ? Motor::Direction::Backward : Motor::Direction::Forward;
        if (_reverse)
        {
            if (dir == Motor::Direction::Forward)
                dir = Motor::Direction::Backward;
            else
                dir = Motor::Direction::Forward;
        }

        _motor->Enable(dir);

        // if direction changed do backlash adjustment - TO DO
        if (FocusBacklashNP[0].getValue() != 0 && lastDir != _dir && FocusAbsPosN[0].value != 0)
        {
            IDMessage(getDeviceName(), "Waveshare Motor HAT Focuser backlash compensation by %d steps...", (int)FocusBacklashNP[0].getValue());
            for (int i = 0; i < FocusBacklashNP[0].getValue() * MICROSTEPPING; i++)
                _motor->SingleStep(_usPerStep);
        }

        uint32_t currentPos = FocusAbsPosN[0].value;
        
        // GO
        while (currentPos != targetPos && !_abort)
        {
            for (int i = 0; i < MICROSTEPPING; i++)
                _motor->SingleStep(_usPerStep);
            
            currentPos += _dir == FOCUS_OUTWARD ? 1 : -1;
            
            if (currentPos % 50 == 0)
            {
                FocusAbsPosN[0].value = currentPos;
                FocusAbsPosNP.s = IPS_BUSY;
                IDSetNumber(&FocusAbsPosNP, nullptr);
            }            
        }
        _motor->Disable();

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
