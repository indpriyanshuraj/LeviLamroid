#pragma once
#include <algorithm>
#include "mc/world/level/BlockPos.h"

class BoundingBox {
public:
    BlockPos min{0, 0, 0};
    BlockPos max{0, 0, 0};

    constexpr BoundingBox() noexcept = default;

    constexpr BoundingBox(BlockPos const& pMin, BlockPos const& pMax) noexcept
        : min(std::min(pMin.x, pMax.x), std::min(pMin.y, pMax.y), std::min(pMin.z, pMax.z)),
          max(std::max(pMin.x, pMax.x), std::max(pMin.y, pMax.y), std::max(pMin.z, pMax.z)) {}

    constexpr BoundingBox(int32_t x0, int32_t y0, int32_t z0, int32_t x1, int32_t y1, int32_t z1) noexcept
        : min(std::min(x0, x1), std::min(y0, y1), std::min(z0, z1)),
          max(std::max(x0, x1), std::max(y0, y1), std::max(z0, z1)) {}

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    [[nodiscard]] constexpr bool contains(BlockPos const& pos) const noexcept {
        return pos.x >= min.x && pos.x <= max.x &&
               pos.y >= min.y && pos.y <= max.y &&
               pos.z >= min.z && pos.z <= max.z;
    }

    [[nodiscard]] constexpr BlockPos getSpan() const noexcept {
        return {max.x - min.x + 1, max.y - min.y + 1, max.z - min.z + 1};
    }

    [[nodiscard]] constexpr BlockPos getCenter() const noexcept {
        return {(min.x + max.x) / 2, (min.y + max.y) / 2, (min.z + max.z) / 2};
    }

    [[nodiscard]] constexpr bool operator==(BoundingBox const& rhs) const noexcept {
        return min == rhs.min && max == rhs.max;
    }

    [[nodiscard]] constexpr bool operator!=(BoundingBox const& rhs) const noexcept {
        return !(*this == rhs);
    }
};
