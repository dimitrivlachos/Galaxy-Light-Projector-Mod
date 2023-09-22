#include "generic_fsm.h"

GenericFSM::GenericFSM() {}

GenericFSM::GenericFSM(
  State initialState, std::function<void()> onStateChangeCallback, State *customState)
  : currentState(initialState), onStateChangeCallback(onStateChangeCallback) {
    initCustomState(customState);
  }

/**
 * @brief Appends a state to the list of iterative states.
 * 
 * @param state 
 */
void GenericFSM::addState(State state) {
    iterativeStates.push_back(state);

    // If this is the first state added, set it as the current state
    if (iterativeStates.size() == 1) {
        currentState = state;
    }
}

/**
 * @brief Advances to the next state in the enum.
 */
void GenericFSM::nextState() {
  // If the FSM is using a custom state, return to the previous state
  if (custom) {
    custom = false;
  }

  // Search for index of current state in iterativeStates
  int index = getStateIndex(currentState);

  // Check if the current state is the last state in the enum
  if (index == iterativeStates.size() - 1) {
    currentState = iterativeStates[0]; // Set the current state to the first state in the enum
  } else {
    currentState = iterativeStates[index + 1]; // Set the current state to the next state in the enum
  }

  onStateChange(); // Invoke the callback function
}

/**
 * @brief Sets the state to a specified state.
 *
 * @param newState The state to set.
 */
void GenericFSM::setState(State newState) {
  // If the new state is the same as the current state, return
  if (newState == currentState) {
    return;
  }

  // Check if newState is in iterativeStates, if not, return
  if (getStateIndex(newState) == -1) {
    Serial.println("Invalid state");
    return;
  }

  currentState = newState;
  onStateChange(); // Invoke the callback function
}

/**
 * @brief Retrieves the current state of the FSM.
 *
 * @return The current state.
 */
State GenericFSM::getCurrentState() const {
    return currentState;
}

/**
 * @brief Retrieves the index of the current state of the FSM.
 *
 * @return The index of the current state.
 */
int GenericFSM::getCurrentStateIndex() const {
  return getStateIndex(currentState);
}

/**
 * @brief Adds an action to be performed when in a specified state.
 *
 * @param state The state to add the action to.
 * @param action The action to be performed.
 */
void GenericFSM::setStateAction(State state, std::function<void()> action) {
  // Check if state is in iterativeStates, if not, return
  int index = getStateIndex(state);
  if (index == -1) {
    Serial.printf("Invalid state: %s\n", state.getState().c_str());
    return;
  }

  iterativeStates[index].setAction(action);
}

/**
 * @brief performs the action associated with the current state.
 */
void GenericFSM::performStateAction() {
  if (custom) {
    customState->performAction();
    return;
  }

  currentState.performAction();
}

/**
 * @brief Sets the custom state.
 */
void GenericFSM::setCustomState(State *customState) {
  initCustomState(customState);
}

/**
 * @brief Sets the FSM to use the custom state.
 */
void GenericFSM::useCustomState() {
  if (customState == nullptr) throw "Custom state must be initialized before use";
  if (custom) return; // If the FSM is already using the custom state, return
  custom = true;
  onStateChange(); // Invoke the callback function
}

bool custom = false;
std::vector<State> iterativeStates;
std::function<void()> onStateChangeCallback; // A callback function to be invoked when the state changes

/**
 * @brief Initializes the custom state.
 */
void GenericFSM::initCustomState(State *customState) {
  // Check that customState has an action
  if (customState != nullptr && customState->getAction() == nullptr) {
    throw "Custom state must have an action";
  }

  this->customState = customState;
}

/**
 * @brief Invokes the callback function.
 */
void GenericFSM::onStateChange() {
    // Invoke the callback function if it is not null
    if (onStateChangeCallback != nullptr) {
        onStateChangeCallback();
    }
}

/**
 * @brief Get the index of a specified state in iterativeStates.
 * 
 * @param state 
 * @return int 
 */
int GenericFSM::getStateIndex(State state) const {
  // Search for index of current state in iterativeStates
  for (int i = 0; i < iterativeStates.size(); i++) {
    if (iterativeStates[i] == currentState) {
      return i;
    }
  }
  return -1;
}