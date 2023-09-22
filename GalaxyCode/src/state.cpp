#include "state.h"

State::State()
: state(""), action(nullptr) {}

State::State(std::string name) 
: state(name), action(nullptr) {}

State::State(std::string name, std::function<void()> action) 
: state(name), action(action) {}

std::string State::getState() {
  return state;
}

void State::performAction() {
  if (action == nullptr) return;
  
  action();
}

void State::setAction(std::function<void()> action) {
  this->action = action;
}

std::function<void()> State::getAction() {
  return action;
}

bool State::operator==(const State& other) const {
  return state == other.state;
}