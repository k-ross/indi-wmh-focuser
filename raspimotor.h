#pragma once

#include <gpiod.hpp>
#include <memory>
#include "motor.h"

class RasPiMotor : public Motor
{
    gpiod::chip _chip;
    gpiod::line _enableLine;
    gpiod::line _directionLine;
    gpiod::line _stepLine;
    
public:
	RasPiMotor(const char* consumer, int enablePin, int directionPin, int stepPin);
	
	~RasPiMotor() override;
	
	void Enable(Direction dir) override;
	void Disable() override;
	void SingleStep(int stepDelayMicroseconds) override;
};
