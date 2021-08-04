#include "mraamotor.h"

MraaMotor::MraaMotor(int enablePin, int directionPin, int stepPin)
{
    _enableLine =       std::make_unique<mraa::Gpio>(enablePin);
    _directionLine =    std::make_unique<mraa::Gpio>(directionPin);
    _stepLine =         std::make_unique<mraa::Gpio>(stepPin);

    _enableLine->dir(mraa::DIR_OUT);
    _directionLine->dir(mraa::DIR_OUT);
    _stepLine->dir(mraa::DIR_OUT);
}
	
MraaMotor::~MraaMotor()
{
}
	
void MraaMotor::Enable(Direction dir)
{
    if (_revision == BoardRevision::Rev21)
        _enableLine->write (1);
    else
        _enableLine->write (0);
        
    if (dir == Direction::Forward)
        _directionLine->write (0);
    else
        _directionLine->write (1);
}

void MraaMotor::Disable()
{
    if (_revision == BoardRevision::Rev21)
        _enableLine->write (0);
    else
        _enableLine->write (1);
}

void MraaMotor::SingleStep(int stepDelayMicroseconds)
{
    _stepLine->write (1);
    _Delay (stepDelayMicroseconds);
    _stepLine->write (0);
    _Delay (stepDelayMicroseconds);
}
