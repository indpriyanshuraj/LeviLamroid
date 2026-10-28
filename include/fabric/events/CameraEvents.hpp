#pragma once

#include "ll/api/event/Event.h"
#include "mc/deps/core/math/Vec3.h"

namespace fabric::event {

class CameraTransformEvent : public ::ll::event::Event {
private:
    Vec3 mPosition{0.0f, 0.0f, 0.0f};
    float mYaw{0.0f};
    float mPitch{0.0f};
    float mRoll{0.0f}; // In degrees (Bedrock default is 0)
    bool mModified{false};
    bool mDetachFromPlayer{false};

public:
    CameraTransformEvent(const Vec3& pos, float yaw, float pitch, float roll = 0.0f)
        : mPosition(pos), mYaw(yaw), mPitch(pitch), mRoll(roll) {}

    [[nodiscard]] const Vec3& getPosition() const noexcept { return mPosition; }
    [[nodiscard]] float getYaw() const noexcept { return mYaw; }
    [[nodiscard]] float getPitch() const noexcept { return mPitch; }
    [[nodiscard]] float getRoll() const noexcept { return mRoll; }
    [[nodiscard]] bool isModified() const noexcept { return mModified; }
    [[nodiscard]] bool isDetached() const noexcept { return mDetachFromPlayer; }

    void setPosition(const Vec3& pos) noexcept {
        mPosition = pos;
        mModified = true;
    }

    void setRotation(float yaw, float pitch, float roll) noexcept {
        mYaw = yaw;
        mPitch = pitch;
        mRoll = roll;
        mModified = true;
    }

    /// Sets the camera roll angle (in degrees). Enables Elytra Barrel Roll!
    void setRoll(float rollDeg) noexcept {
        mRoll = rollDeg;
        mModified = true;
    }

    /// Decouples camera from LocalPlayer for ReplayMod or Freecam
    void setDetached(bool detached) noexcept {
        mDetachFromPlayer = detached;
        mModified = true;
    }
};

} // namespace fabric::event

extern "C" {
typedef void (*FabricCameraCallback)(float* inOutYaw, float* inOutPitch, float* inOutRoll, float* inOutX, float* inOutY, float* inOutZ);
void fabric_register_camera_callback(FabricCameraCallback cb);
}
