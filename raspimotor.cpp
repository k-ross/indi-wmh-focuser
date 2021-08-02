#include "raspimotor.h"

RasPiMotor::RasPiMotor(const char *consumer, int enablePin, int directionPin, int stepPin)
{
    _chip.open("gpiochip0");
    if (_chip)
    {
        _enableLine =       _chip.get_line(enablePin);
        _directionLine =    _chip.get_line(directionPin);
        _stepLine =         _chip.get_line(stepPin);

        gpiod::line_request request
        {
            .consumer = consumer,
            .request_type = gpiod::line_request::DIRECTION_OUTPUT
        };
        if (_enableLine)    _enableLine.request(request);
        if (_directionLine) _directionLine.request(request);
        if (_stepLine)      _stepLine.request(request);
    }		
}
	
RasPiMotor::~RasPiMotor()
{
}
	
void RasPiMotor::Enable(Direction dir)
{
    if (_revision == BoardRevision::Rev21)
        _enableLine.set_value (1);
    else
        _enableLine.set_value (0);
        
    if (dir == Direction::Forward)
        _directionLine.set_value (0);
    else
        _directionLine.set_value (1);
}

void RasPiMotor::Disable()
{
    if (_revision == BoardRevision::Rev21)
        _enableLine.set_value (0);
    else
        _enableLine.set_value (1);
}

void RasPiMotor::SingleStep(int stepDelayMicroseconds)
{
    _stepLine.set_value (1);
    _Delay (stepDelayMicroseconds);
    _stepLine.set_value (0);
    _Delay (stepDelayMicroseconds);
}
