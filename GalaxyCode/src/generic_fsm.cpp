#include "generic_fsm.h"

GenericFSM::GenericFSM(String id, void (*onStateChangeCallback)())
: id(id), onStateChangeCallback(onStateChangeCallback) {}

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
void GenericFSM::nextState() {// Search for index of current state in iterativeStates
  int index = getStateIndex(currentState);

  // Check if the current state is the last state in the enum
  if (index == iterativeStates.size() - 1) {
    currentState = iterativeStates[0]; // Set the current state to the first state in the enum
  } else {
    currentState = iterativeStates[index + 1]; // Set the current state to the next state in the enum
  }

  // If the FSM is using a custom state, return to the set states
  custom = false;

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
 * @brief performs the action associated with the current state.
 */
void GenericFSM::performStateAction() {
  if (custom) {
    customState.performAction();
  }
  else {
    currentState.performAction();
  }
}

/**
 * @brief Sets the custom state.
 */
void GenericFSM::setCustomState(State customState) {
  this->customState = customState;
}

/**
 * @brief Sets the FSM to use the custom state.
 */
void GenericFSM::useCustomState() {
  if (custom) return; // If the FSM is already using the custom state, return
  custom = true;
  onStateChange(); // Invoke the callback function
}

bool custom = false;
std::vector<State> iterativeStates;
std::function<void()> onStateChangeCallback; // A callback function to be invoked when the state changes

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
int GenericFSM::getStateIndex(const State &state) const {
  // Search for index of current state in iterativeStates
  for (int i = 0; i < iterativeStates.size(); i++) {
    if (iterativeStates[i] == state) {
      return i;
    }
  }
  return -1;
}