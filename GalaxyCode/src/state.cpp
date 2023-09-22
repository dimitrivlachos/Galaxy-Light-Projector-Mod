#include "state.h"

State::State()
: name(""), action(nullptr) {}

State::State(std::string name) 
: name(name), action(nullptr) {}

State::State(std::string name, const std::function<void()> action) 
: name(name), action(action) {}

State::State(const State& other)
: name(other.name), action(other.action) {}

std::string State::getState() {
  return name;
}

void State::performAction() {
  if (action == nullptr) return;
  
  action();
}

void State::setAction(const std::function<void()> action) {
  this->action = std::move(action);
}

std::function<void()> State::getAction() {
  return action;
}

bool State::operator==(const State& other) const {
  return name == other.name;
}