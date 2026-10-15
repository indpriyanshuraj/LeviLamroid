#pragma once
#include <cstdint>
#include "mc/deps/input/MouseAction.h"

class MouseDevice {
public:
    virtual ~MouseDevice() = default;

    virtual void feed(
        char actionButtonId,
        char buttonData,
        short x,
        short y,
        short dx,
        short dy,
        bool isRelative
    ) {
        (void)actionButtonId;
        (void)buttonData;
        (void)x;
        (void)y;
        (void)dx;
        (void)dy;
        (void)isRelative;
    }
};
