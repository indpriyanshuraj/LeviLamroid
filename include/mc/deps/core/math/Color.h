#pragma once

namespace mce {

struct Color {
    float r{1.0f};
    float g{1.0f};
    float b{1.0f};
    float a{1.0f};

    constexpr Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color WHITE() { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static constexpr Color BLACK() { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr Color RED()   { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static constexpr Color GREEN() { return {0.0f, 1.0f, 0.0f, 1.0f}; }
    static constexpr Color BLUE()  { return {0.0f, 0.0f, 1.0f, 1.0f}; }
    static constexpr Color ZERO()  { return {0.0f, 0.0f, 0.0f, 0.0f}; }
};

} // namespace mce
