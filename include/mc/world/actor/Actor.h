#pragma once
#include <cstdint>
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/math/Vec2.h"
#include "mc/deps/core/math/AABB.h"

class Level;
class Dimension;

class Actor {
public:
    Vec3 mPosition{0.0f, 0.0f, 0.0f};
    Vec2 mRotation{0.0f, 0.0f};
    AABB mAABB{};
    int  mHealth{20};

    virtual ~Actor() = default;

    void setPos(const Vec3& pos) noexcept { mPosition = pos; }
    void swing() noexcept {}
    void setHealth(int health) noexcept { mHealth = health; }

    [[nodiscard]] virtual Level& getLevel() const;
    [[nodiscard]] virtual Dimension& getDimension() const;

    [[nodiscard]] const Vec3& getPosition() const noexcept { return mPosition; }
    [[nodiscard]] const Vec2& getRotation() const noexcept { return mRotation; }
    [[nodiscard]] float getEyeHeight() const noexcept { return 1.62f; }
    [[nodiscard]] const AABB& getAABB() const noexcept { return mAABB; }
    [[nodiscard]] int getHealth() const noexcept { return mHealth; }
    [[nodiscard]] bool isAlive() const noexcept { return mHealth > 0; }

    [[nodiscard]] uintptr_t getVTable() const noexcept {
        return *reinterpret_cast<const uintptr_t*>(this);
    }
};
