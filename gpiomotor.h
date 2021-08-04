#pragma once

#include <gpiod.hpp>
#include <memory>
#include "motor.h"

class GpioMotor : public Motor
{
    gpiod::chip _enableChip;
    gpiod::chip _directionChip;
    gpiod::chip _stepChip;
    gpiod::line _enableLine;
    gpiod::line _directionLine;
    gpiod::line _stepLine;
    
public:
    GpioMotor(const std::string& consumer, 
        const std::string& enableChip, int enablePin, 
        const std::string& directionChip, int directionPin, 
        const std::string& stepChip, int stepPin);

    ~GpioMotor() override;

    void Enable(Direction dir) override;
    void Disable() override;
    void SingleStep(int stepDelayMicroseconds) override;
};
