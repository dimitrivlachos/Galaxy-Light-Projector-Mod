#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>
#include "state.h"

/**
 * @brief A generic Finite State Machine (FSM) template.
 *
 * @param initialState The initial state of the FSM.
 * @param onStateChangeCallback A callback function to be invoked when the state changes.
 */
class GenericFSM {
public:
    bool custom = false;
    String id;
    GenericFSM(String id, void (*onStateChangeCallback)());
    void addState(State state);
    void nextState();
    State getCurrentState() const;
    int getCurrentStateIndex() const;
    void performStateAction();
    void setCustomState(State customState);
    void useCustomState();

private:
    State currentState;
    State customState;
    std::vector<State> iterativeStates;
    void (*onStateChangeCallback)();
    void onStateChange();
    int getStateIndex(const State &state) const;
};