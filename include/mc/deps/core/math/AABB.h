#pragma once
#include "mc/deps/core/math/Vec3.h"

struct AABB {
    Vec3 min{0.0f, 0.0f, 0.0f};
    Vec3 max{0.0f, 0.0f, 0.0f};

    constexpr AABB() = default;
    constexpr AABB(const Vec3& min_, const Vec3& max_) : min(min_), max(max_) {}
    constexpr AABB(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
        : min(minX, minY, minZ), max(maxX, maxY, maxZ) {}
};
