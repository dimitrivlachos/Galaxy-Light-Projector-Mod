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
    
    State();
    State(std::string name);
    State(std::string name, const std::function<void()> action);
    State(const State& other); // copy constructor

    std::string getState();
    void performAction();
    void setAction(const std::function<void()> action);
    std::function<void()> getAction();
    bool operator==(const State& other) const;

private:
    std::function<void()> action;
};