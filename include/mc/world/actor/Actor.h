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

    virtual void setPos(const Vec3& pos);
    virtual void swing();
    virtual void setHealth(int health);

    [[nodiscard]] virtual Level& getLevel() const;
    [[nodiscard]] virtual Dimension& getDimension() const;

    [[nodiscard]] virtual const Vec3& getPosition() const noexcept;
    [[nodiscard]] virtual const Vec2& getRotation() const noexcept;
    [[nodiscard]] virtual float getEyeHeight() const noexcept;
    [[nodiscard]] virtual const AABB& getAABB() const noexcept;
    [[nodiscard]] virtual int getHealth() const noexcept;
    [[nodiscard]] virtual bool isAlive() const noexcept;

    [[nodiscard]] uintptr_t getVTable() const noexcept {
        return *reinterpret_cast<const uintptr_t*>(this);
    }
};
