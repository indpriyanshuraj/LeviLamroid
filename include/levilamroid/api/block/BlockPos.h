#pragma once
#include <cstdint>
#include <string>
#include <functional>

namespace levilamroid::block {

struct BlockPos {
    int32_t x{0};
    int32_t y{0};
    int32_t z{0};

    constexpr BlockPos() noexcept = default;
    constexpr BlockPos(int32_t inX, int32_t inY, int32_t inZ) noexcept : x(inX), y(inY), z(inZ) {}

    constexpr bool operator==(const BlockPos& other) const noexcept {
        return x == other.x && y == other.y && z == other.z;
    }

    constexpr bool operator!=(const BlockPos& other) const noexcept {
        return !(*this == other);
    }

    constexpr BlockPos operator+(const BlockPos& other) const noexcept {
        return {x + other.x, y + other.y, z + other.z};
    }

    constexpr BlockPos operator-(const BlockPos& other) const noexcept {
        return {x - other.x, y - other.y, z - other.z};
    }

    constexpr BlockPos& operator+=(const BlockPos& other) noexcept {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr BlockPos& operator-=(const BlockPos& other) noexcept {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    [[nodiscard]] constexpr BlockPos above(int32_t step = 1) const noexcept {
        return {x, y + step, z};
    }

    [[nodiscard]] constexpr BlockPos below(int32_t step = 1) const noexcept {
        return {x, y - step, z};
    }

    [[nodiscard]] constexpr BlockPos north(int32_t step = 1) const noexcept {
        return {x, y, z - step};
    }

    [[nodiscard]] constexpr BlockPos south(int32_t step = 1) const noexcept {
        return {x, y, z + step};
    }

    [[nodiscard]] constexpr BlockPos west(int32_t step = 1) const noexcept {
        return {x - step, y, z};
    }

    [[nodiscard]] constexpr BlockPos east(int32_t step = 1) const noexcept {
        return {x + step, y, z};
    }

    [[nodiscard]] std::string toString() const {
        return "(" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")";
    }
};

static_assert(sizeof(BlockPos) == 12, "BlockPos must be 12 bytes");

} // namespace levilamroid::block

namespace std {
template <>
struct hash<::levilamroid::block::BlockPos> {
    size_t operator()(const ::levilamroid::block::BlockPos& pos) const noexcept {
        size_t h1 = std::hash<int32_t>{}(pos.x);
        size_t h2 = std::hash<int32_t>{}(pos.y);
        size_t h3 = std::hash<int32_t>{}(pos.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std

namespace ll::block {
    using BlockPos = ::levilamroid::block::BlockPos;
}
