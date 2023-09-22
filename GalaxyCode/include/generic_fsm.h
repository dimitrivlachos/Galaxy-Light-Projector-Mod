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
    GenericFSM();
    GenericFSM(State initialState, std::function<void()> onStateChangeCallback, State* customState);
    void addState(State state);
    void nextState();
    void setState(State newState);
    State getCurrentState() const;
    int getCurrentStateIndex() const;
    void setStateAction(State state, std::function<void()> action);
    void performStateAction();
    void setCustomState(State* customState);
    void useCustomState();

private:
    bool custom = false;
    State currentState;
    State* customState;
    std::vector<State> iterativeStates;
    std::function<void()> onStateChangeCallback;
    void initCustomState(State* customState);
    void onStateChange();
    int getStateIndex(State state) const;
};