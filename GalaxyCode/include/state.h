#pragma once

#include <Arduino.h>
#include <string>
#include <functional>

/**
 * @brief A generic state class.
 * 
 * @param name The name of the state.
 * @param action A function to be invoked.
 */
class State {
public:
    std::string name;
    std::function<void()> action;
    State();
    State(std::string name, std::function<void()> action);

    std::string getState();
    void performAction();
    bool operator==(const State& other) const;
};