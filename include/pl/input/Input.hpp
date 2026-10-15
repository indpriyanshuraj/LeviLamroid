#pragma once
#include <functional>

namespace pl::input {

struct KeyEvent {
    int keyCode;
    bool isDown;
};

void registerKeyCallback(std::function<bool(const KeyEvent&)> callback);

} // namespace pl::input
