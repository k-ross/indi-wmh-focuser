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

#pragma once

#include <indifocuser.h>

class IndiWMHFocuser : public INDI::Focuser
{
    protected:
    private:
        ISwitch FocusResetS[1];
        ISwitchVectorProperty FocusResetSP;
        ISwitch FocusParkingS[2];
        ISwitchVectorProperty FocusParkingSP;
        INumber FocusBacklashN[1];
        INumberVectorProperty FocusBacklashNP;
        INumber MotorSpeedN[1];
        INumberVectorProperty MotorSpeedNP;

    public:
        IndiWMHFocuser();
        virtual ~IndiWMHFocuser();

        const char *getDefaultName();

        virtual bool Connect() override;
        virtual bool Disconnect() override;
        virtual bool initProperties() override;
        virtual bool updateProperties() override;
        virtual void ISGetProperties (const char *dev) override;

        virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n) override;
        virtual bool saveConfigItems(FILE *fp) override;

        virtual IPState MoveAbsFocuser(uint32_t ticks) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual bool SyncFocuser(uint32_t targetTicks) override;
        virtual bool ReverseFocuser(bool enabled) override;

        FocusDirection dir;
        int msPerStep;
        bool reverse;

    protected:
        virtual int StepperMotor(uint32_t steps, FocusDirection dir);
};
