#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);
void toggleLED();

#define ONBOARD_LED 2
bool ledOn = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World!");

  pinMode(ONBOARD_LED, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  toggleLED();
  delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

void toggleLED() {
  if(ledOn) {
    digitalWrite(ONBOARD_LED, LOW);
    ledOn = false;
  } else {
    digitalWrite(ONBOARD_LED, HIGH);
    ledOn = true;
  }
}