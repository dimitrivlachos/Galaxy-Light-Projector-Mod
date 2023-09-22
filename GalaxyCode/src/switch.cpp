#include "switch.h"

Switch::Switch(uint8_t pin, uint8_t mode, GenericFSM& fsm)
  : pin(pin), fsm(fsm) {
    pinMode(pin, mode);
}

/*
  * Checks the state of the switch and updates the switch state accordingly.
  * Must be called in the main loop.
*/
void Switch::update() {
  unsigned long lastDebounceTime = 0;
  unsigned long timeReleased = 0;
  const unsigned long debounceDelay = 100;   // Debounce delay in milliseconds
  unsigned long currentMillis = millis();

  int switchReading = digitalRead(pin);

  if (switchReading == LOW) { // If the switch is pressed
    if (!switchState) {
      switchState = true; // Update the switch state
      fsm.nextState(); // Advance to the next state in the FSM

      // Print the current state to the serial monitor
      Serial.printf("Current state: %s\n", fsm.getCurrentState().name.c_str());
    }
  } // No time debounce is required for the switch being pressed as the change in state provides this functionality
  
  else { // If the switch is released
    // Calculate the time since the switch was released
    unsigned long timeSinceRelease = currentMillis - timeReleased;
    if(switchState && timeSinceRelease > debounceDelay) {
      switchState = false;
      timeReleased = currentMillis;
    }
  } // A debounce delay is required for the switch being released as the change in state does not prevent the switch from activating immediately after being released
}