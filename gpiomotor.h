#pragma once

#include <gpiod.hpp>
#include <optional>
#include <filesystem>
#include <string>
#include "motor.h"

class GpioMotor : public Motor
{
    std::optional<gpiod::line_request> _enableRequest;
    std::optional<gpiod::line_request> _directionRequest;
    std::optional<gpiod::line_request> _stepRequest;
    gpiod::line::offset _enablePin;
    gpiod::line::offset _directionPin;
    gpiod::line::offset _stepPin;

public:
    GpioMotor(const std::string& consumer,
        const std::string& enableChip, int enablePin,
        const std::string& directionChip, int directionPin,
        const std::string& stepChip, int stepPin);

    ~GpioMotor() override;

    void Enable(Direction dir) override;
    void Disable() override;
    void SingleStep(int stepDelayMicroseconds) override;

    static std::string getPiChip();

private:
    static std::filesystem::path chipNameToPath(const std::string& name);
    static gpiod::line_request requestOutputLine(
        const std::string& chipName, int pin, const std::string& consumer);
};
