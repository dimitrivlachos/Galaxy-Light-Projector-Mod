#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);
void checkButtons();

#pragma region Pin Definitions

// RGBW LED Pin Definitions
#define GREEN_LED 17
#define BLUE_LED 18
#define WHITE_LED 19
#define BROWN_LED 21

// Projector LED Pin Definitions
#define PROJECTOR_LED 27

// Motor Pin Definitions
#define MOTOR_BJT 4

// Switch Pin Definitions
#define MOTOR_SWITCH 32
#define BRIGHTNESS_SWITCH 33
#define COLOUR_SWITCH 25
#define STATE_SWITCH 26

#pragma endregion

int ledStates = 0b100000;

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World!");

  // Pin Setup
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(WHITE_LED, OUTPUT);
  pinMode(BROWN_LED, OUTPUT);
  pinMode(PROJECTOR_LED, OUTPUT);
  pinMode(MOTOR_BJT, OUTPUT);

  pinMode(MOTOR_SWITCH, INPUT_PULLUP);
  pinMode(BRIGHTNESS_SWITCH, INPUT_PULLUP);
  pinMode(COLOUR_SWITCH, INPUT_PULLUP);
  pinMode(STATE_SWITCH, INPUT_PULLUP);
}

float lastChangeTime = 0;

void loop() {
  if (millis() - lastChangeTime > 1000) {
    // Cycle through each LED, turning one on at a time
    digitalWrite(GREEN_LED, ledStates & 0b100000);
    digitalWrite(BLUE_LED, ledStates & 0b010000);
    digitalWrite(WHITE_LED, ledStates & 0b001000);
    digitalWrite(BROWN_LED, ledStates & 0b000100);
    digitalWrite(MOTOR_BJT, ledStates & 0b000010);
    digitalWrite(PROJECTOR_LED, ledStates & 0b000001);

    ledStates = ledStates >> 1;
    //Reset the LED states if all LEDs have been turned off
    if(ledStates == 0) {
      ledStates = 0b100000;
    }
    lastChangeTime = millis();
  }
  checkButtons();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}

void checkButtons() {
  if(digitalRead(MOTOR_SWITCH) == LOW) {
    Serial.println("Motor Switch Pressed");
  }
  if(digitalRead(BRIGHTNESS_SWITCH) == LOW) {
    Serial.println("Brightness Switch Pressed");
  }
  if(digitalRead(COLOUR_SWITCH) == LOW) {
    Serial.println("Colour Switch Pressed");
  }
  if(digitalRead(STATE_SWITCH) == LOW) {
    Serial.println("State Switch Pressed");
  }
}