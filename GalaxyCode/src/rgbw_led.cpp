#include "rgbw_led.h"

RGBWLED::RGBWLED() {}

RGBWLED::RGBWLED(int redPin, int greenPin, int bluePin, int whitePin) {
  redLED = PWM_Device(redPin);
  greenLED = PWM_Device(greenPin);
  blueLED = PWM_Device(bluePin);
  whiteLED = PWM_Device(whitePin);
}

void RGBWLED::set(int red, int green, int blue, int white) {
  redLED.set(red);
  greenLED.set(green);
  blueLED.set(blue);
  whiteLED.set(white);
}