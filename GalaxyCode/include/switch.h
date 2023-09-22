#pragma once

#include <Arduino.h>
#include "generic_fsm.h"

/**
 * @brief A generic switch.
 *
 * @param pin The pin number of the switch.
 * @param mode The mode of the switch (INPUT, INPUT_PULLUP, etc.).
 * @param fsm The FSM to be updated when the switch is pressed.
 */
class Switch {
public:
    Switch(uint8_t pin, uint8_t mode, GenericFSM& fsm);
    void update();

private:
    uint8_t pin;
    bool switchState = false;
    GenericFSM& fsm;
};