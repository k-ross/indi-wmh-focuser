#include "gpiomotor.h"

using namespace std;

GpioMotor::GpioMotor(const string& consumer, 
        const string& enableChip, int enablePin, 
        const string& directionChip, int directionPin, 
        const string& stepChip, int stepPin)
{
    _enableChip.open(enableChip);
    _directionChip.open(directionChip);
    _stepChip.open(stepChip);

    if (_enableChip)    _enableLine     = _enableChip.get_line(enablePin);
    if (_directionChip) _directionLine  = _directionChip.get_line(directionPin);
    if (_stepChip)      _stepLine       = _stepChip.get_line(stepPin);

    gpiod::line_request request
    {
        .consumer = consumer,
        .request_type = gpiod::line_request::DIRECTION_OUTPUT
    };

    if (_enableLine)    _enableLine.request(request);
    if (_directionLine) _directionLine.request(request);
    if (_stepLine)      _stepLine.request(request);
}
	
GpioMotor::~GpioMotor()
{
}
	
void GpioMotor::Enable(Direction dir)
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

void GpioMotor::Disable()
{
    if (_revision == BoardRevision::Rev21)
        _enableLine.set_value (0);
    else
        _enableLine.set_value (1);
}

void GpioMotor::SingleStep(int stepDelayMicroseconds)
{
    _stepLine.set_value (1);
    _Delay (stepDelayMicroseconds);
    _stepLine.set_value (0);
    _Delay (stepDelayMicroseconds);
}

//figure out what the chip name should be for Raspberry Pi
//for Pi5 it should be gpiochip4; otherwise it should be gpiochip0
//the chip.label() should return something like 'pinctrl_xxx', so we'll use that to detect
string GpioMotor::getPiChip()
{
    static std::string piChip;
    if (piChip.empty()) {
        piChip = "gpiochip0";
        for (auto& it: ::gpiod::make_chip_iter()) {
            string substr = "pinctrl";
            if (strncmp(it.label().c_str(), substr.c_str(), substr.size())==0) {
                piChip = it.name();
            }
        }
    }
    return piChip;
}
