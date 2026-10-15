#pragma once
#include <cstdint>

struct Brightness {
    uint8_t value{0};

    constexpr Brightness() = default;
    constexpr explicit Brightness(uint8_t val) : value(val) {}

    static constexpr Brightness MIN() { return Brightness(0); }
    static constexpr Brightness MAX() { return Brightness(15); }
};
