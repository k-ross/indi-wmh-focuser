#pragma once

#include <memory>
#include <mraa/common.hpp>
#include <mraa/gpio.hpp>
#include "motor.h"

class RockPiMotor : public Motor
{
    std::unique_ptr<mraa::Gpio> _enableLine;
    std::unique_ptr<mraa::Gpio> _directionLine;
    std::unique_ptr<mraa::Gpio> _stepLine;
    
public:
    RockPiMotor(int enablePin, int directionPin, int stepPin);

    ~RockPiMotor() override;

    void Enable(Direction dir) override;
    void Disable() override;
    void SingleStep(int stepDelayMicroseconds) override;
};
