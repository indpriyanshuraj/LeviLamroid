#pragma once

struct BlockPos {
    int x{0};
    int y{0};
    int z{0};

    constexpr BlockPos() = default;
    constexpr BlockPos(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}

    constexpr BlockPos operator+(const BlockPos& o) const noexcept { return {x + o.x, y + o.y, z + o.z}; }
    constexpr BlockPos operator-(const BlockPos& o) const noexcept { return {x - o.x, y - o.y, z - o.z}; }
    constexpr BlockPos operator*(int s) const noexcept { return {x * s, y * s, z * s}; }
    constexpr bool operator<(const BlockPos& o) const noexcept {
        if (x != o.x) return x < o.x;
        if (y != o.y) return y < o.y;
        return z < o.z;
    }
    constexpr bool operator==(const BlockPos& o) const noexcept { return x == o.x && y == o.y && z == o.z; }
    constexpr bool operator!=(const BlockPos& o) const noexcept { return !(*this == o); }
};
