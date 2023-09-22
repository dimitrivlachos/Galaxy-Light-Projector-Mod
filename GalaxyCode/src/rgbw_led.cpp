#include "rgbw_led.h"

RGBWLED::RGBWLED() {}

RGBWLED::RGBWLED(int redPin, int greenPin, int bluePin, int whitePin) {
  redLED = new PWM_Device(redPin);
  greenLED = new PWM_Device(greenPin);
  blueLED = new PWM_Device(bluePin);
  whiteLED = new PWM_Device(whitePin);
}

RGBWLED::~RGBWLED() {
  delete redLED;
  delete greenLED;
  delete blueLED;
  delete whiteLED;
}

void RGBWLED::set(int red, int green, int blue, int white) {
  redLED->set(red);
  greenLED->set(green);
  blueLED->set(blue);
  whiteLED->set(white);
}