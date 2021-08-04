#pragma once

#include <memory>
#include <mraa/common.hpp>
#include <mraa/gpio.hpp>
#include "motor.h"

class MraaMotor : public Motor
{
    std::unique_ptr<mraa::Gpio> _enableLine;
    std::unique_ptr<mraa::Gpio> _directionLine;
    std::unique_ptr<mraa::Gpio> _stepLine;
    
public:
    MraaMotor(int enablePin, int directionPin, int stepPin);

    ~MraaMotor() override;

    void Enable(Direction dir) override;
    void Disable() override;
    void SingleStep(int stepDelayMicroseconds) override;
};
