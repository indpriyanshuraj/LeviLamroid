#pragma once
#include "mc/deps/core/math/Vec3.h"

namespace mce {

class Camera {
public:
    Vec3  mPosition{};
    Vec3  mTarget{};
    Vec3  mUp{0.0f, 1.0f, 0.0f};
    float mFov{70.0f};
    float mNearPlane{0.1f};
    float mFarPlane{1000.0f};
    float mAspectRatio{16.0f / 9.0f};

    Camera() = default;
    virtual ~Camera() = default;

    [[nodiscard]] Vec3 const& getPosition() const noexcept { return mPosition; }
    [[nodiscard]] Vec3 const& getTarget() const noexcept { return mTarget; }
    [[nodiscard]] Vec3 const& getUp() const noexcept { return mUp; }
    [[nodiscard]] float getFov() const noexcept { return mFov; }
    [[nodiscard]] float getNear() const noexcept { return mNearPlane; }
    [[nodiscard]] float getFar() const noexcept { return mFarPlane; }

    void setPosition(Vec3 const& pos) noexcept { mPosition = pos; }
    void setTarget(Vec3 const& target) noexcept { mTarget = target; }
    void setUp(Vec3 const& up) noexcept { mUp = up; }
    void setFov(float fov) noexcept { mFov = fov; }
};

} // namespace mce
