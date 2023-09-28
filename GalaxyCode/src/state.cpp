#include "state.h"

State::State()
: name(""), action(nullptr) {}

State::State(std::string name, std::function<void()> action) 
: name(name), action(action) {}

std::string State::getState() {
  return name;
}

void State::performAction() {
  if (action == nullptr) return;
  
  action();
}

bool State::operator==(const State& other) const {
  return name == other.name;
}