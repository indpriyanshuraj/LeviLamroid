#pragma once
#include <cstdint>

enum class Rotation : uint8_t {
    None = 0,
    Rotate90 = 1,
    Clockwise90 = 1,
    Rotate180 = 2,
    Clockwise180 = 2,
    Rotate270 = 3,
    CounterClockwise90 = 3,
    Total = 4
};
