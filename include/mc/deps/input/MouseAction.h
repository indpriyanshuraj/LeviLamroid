#pragma once
#include <cstdint>

namespace MouseAction {

enum Type : int32_t {
    ActionNone = 0,
    ActionDown = 1,
    ActionUp = 2,
    ActionMove = 3,
    ActionWheel = 4
};

} // namespace MouseAction
