#pragma once

#include <Arduino.h>

/**
 * @brief A generic PWM device.
 *
 * @param pin The pin number of the PWM device.
 */
class PWM_Device {
    public:
        PWM_Device();
        PWM_Device(int pin);
        void set(uint8_t value);

    private:
        int pin;
        int brightness = 0;
};