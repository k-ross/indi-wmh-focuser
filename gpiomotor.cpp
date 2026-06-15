#include "gpiomotor.h"
#include <iostream>

using namespace std;

filesystem::path GpioMotor::chipNameToPath(const string& name)
{
    if (!name.empty() && name[0] == '/')
        return filesystem::path(name);
    return filesystem::path("/dev") / name;
}

gpiod::line_request GpioMotor::requestOutputLine(
    const string& chipName, int pin, const string& consumer)
{
    gpiod::line_settings settings;
    settings.set_direction(gpiod::line::direction::OUTPUT);

    return gpiod::chip(chipNameToPath(chipName))
        .prepare_request()
        .set_consumer(consumer)
        .add_line_settings(gpiod::line::offset(pin), settings)
        .do_request();
}

GpioMotor::GpioMotor(const string& consumer,
        const string& enableChip, int enablePin,
        const string& directionChip, int directionPin,
        const string& stepChip, int stepPin)
    : _enablePin(enablePin), _directionPin(directionPin), _stepPin(stepPin)
{
    try {
        _enableRequest.emplace(requestOutputLine(enableChip, enablePin, consumer));
    } catch (const exception& e) {
        cerr << "GpioMotor: failed to request enable line (chip="
             << enableChip << " pin=" << enablePin << "): " << e.what() << endl;
    }

    try {
        _directionRequest.emplace(requestOutputLine(directionChip, directionPin, consumer));
    } catch (const exception& e) {
        cerr << "GpioMotor: failed to request direction line (chip="
             << directionChip << " pin=" << directionPin << "): " << e.what() << endl;
    }

    try {
        _stepRequest.emplace(requestOutputLine(stepChip, stepPin, consumer));
    } catch (const exception& e) {
        cerr << "GpioMotor: failed to request step line (chip="
             << stepChip << " pin=" << stepPin << "): " << e.what() << endl;
    }
}

GpioMotor::~GpioMotor() = default;

void GpioMotor::Enable(Direction dir)
{
    if (_enableRequest) {
        if (_revision == BoardRevision::Rev21)
            _enableRequest->set_value(_enablePin, gpiod::line::value::ACTIVE);
        else
            _enableRequest->set_value(_enablePin, gpiod::line::value::INACTIVE);
    }

    if (_directionRequest) {
        if (dir == Direction::Forward)
            _directionRequest->set_value(_directionPin, gpiod::line::value::INACTIVE);
        else
            _directionRequest->set_value(_directionPin, gpiod::line::value::ACTIVE);
    }
}

void GpioMotor::Disable()
{
    if (_enableRequest) {
        if (_revision == BoardRevision::Rev21)
            _enableRequest->set_value(_enablePin, gpiod::line::value::INACTIVE);
        else
            _enableRequest->set_value(_enablePin, gpiod::line::value::ACTIVE);
    }
}

void GpioMotor::SingleStep(int stepDelayMicroseconds)
{
    if (_stepRequest) {
        _stepRequest->set_value(_stepPin, gpiod::line::value::ACTIVE);
        _Delay(stepDelayMicroseconds);
        _stepRequest->set_value(_stepPin, gpiod::line::value::INACTIVE);
        _Delay(stepDelayMicroseconds);
    }
}

// Figure out what the chip name should be for Raspberry Pi.
// For Pi5 it should be gpiochip4; otherwise it should be gpiochip0.
// The chip label should return something like 'pinctrl_xxx', so we use that to detect.
string GpioMotor::getPiChip()
{
    static string piChip;
    if (piChip.empty()) {
        piChip = "gpiochip0";
        for (const auto& entry : filesystem::directory_iterator("/dev/")) {
            if (gpiod::is_gpiochip_device(entry.path())) {
                gpiod::chip chip(entry.path());
                auto info = chip.get_info();
                string label = info.label();
                if (label.rfind("pinctrl", 0) == 0) {
                    piChip = info.name();
                }
            }
        }
    }
    return piChip;
}
