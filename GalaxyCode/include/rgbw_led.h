#pragma once

#include "pwm_device.h"

class RGBWLED {
public:
    RGBWLED();
    RGBWLED(int redPin, int greenPin, int bluePin, int whitePin);
    ~RGBWLED();
    void set(int red, int green, int blue, int white);

private:
    PWM_Device* redLED;
    PWM_Device* greenLED;
    PWM_Device* blueLED;
    PWM_Device* whiteLED;
};