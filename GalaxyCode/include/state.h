#pragma once

#include <string>
#include <functional>

/**
 * @brief A generic state class.
 * 
 * @param state The name of the state.
 * @param action A function to be invoked when the state is entered.
 */
class State {
public:
    State();
    State(std::string name);
    State(std::string name, std::function<void()> action);

    std::string getState();
    void performAction();
    void setAction(std::function<void()> action);
    std::function<void()> getAction();
    bool operator==(const State& other) const;

private:
    std::string state;
    std::function<void()> action;
};