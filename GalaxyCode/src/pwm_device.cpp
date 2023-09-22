#include "pwm_device.h"

PWM_Device::PWM_Device() {}

PWM_Device::PWM_Device(int pin) {
  this->pin = pin;
  pinMode(pin, OUTPUT);
}

/**
  * @brief Sets the brightness of the PWM device.
  * 
  * @param value The brightness of the PWM device (0-255).
*/
void PWM_Device::set(uint8_t value) {
  if (value == brightness) return; // If the brightness is unchanged, skip the operation
  if (value < 0 || value > 255) throw "Invalid PWM_Device value - must be between 0 and 255"; // If the brightness is out of range, throw an error
  
  brightness = value; // Update the brightness
  analogWrite(pin, brightness); // Set the brightness
}