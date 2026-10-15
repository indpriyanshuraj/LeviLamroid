#pragma once
#include <cstdint>
#include <string_view>
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/BlockPos.h"

namespace Facing {

enum class Name : unsigned char {
    Down = 0,
    Up = 1,
    North = 2,
    South = 3,
    West = 4,
    East = 5,
    Max = 6,
    Undefined = 0xFF
};

inline constexpr Name OPPOSITE_FACING_ARRAY[] = {
    Name::Up,    // Down -> Up
    Name::Down,  // Up -> Down
    Name::South, // North -> South
    Name::North, // South -> North
    Name::East,  // West -> East
    Name::West   // East -> West
};

inline const unsigned char* OPPOSITE_FACING() noexcept {
    static const unsigned char sOpposite[6] = {1, 0, 3, 2, 5, 4};
    return sOpposite;
}

inline const BlockPos* DIRECTION() noexcept {
    static const BlockPos sDirs[6] = {
        {0, -1, 0},
        {0, 1, 0},
        {0, 0, -1},
        {0, 0, 1},
        {-1, 0, 0},
        {1, 0, 0}
    };
    return sDirs;
}

[[nodiscard]] inline constexpr Name getOpposite(Name facing) noexcept {
    auto const index = static_cast<size_t>(facing);
    if (index < 6) return OPPOSITE_FACING_ARRAY[index];
    return Name::Undefined;
}

[[nodiscard]] inline constexpr unsigned char getOpposite(unsigned char facing) noexcept {
    return static_cast<unsigned char>(getOpposite(static_cast<Name>(facing)));
}

[[nodiscard]] inline constexpr BlockPos getStep(Name facing) noexcept {
    switch (facing) {
        case Name::Down:  return {0, -1, 0};
        case Name::Up:    return {0, 1, 0};
        case Name::North: return {0, 0, -1};
        case Name::South: return {0, 0, 1};
        case Name::West:  return {-1, 0, 0};
        case Name::East:  return {1, 0, 0};
        default:          return {0, 0, 0};
    }
}

} // namespace Facing
