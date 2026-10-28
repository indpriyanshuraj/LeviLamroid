#pragma once

#include <atomic>
#include <mutex>

namespace fabric::camera {

class CameraSettings {
public:
    static CameraSettings& getInstance() {
        static CameraSettings instance;
        return instance;
    }

    void setZoom(float zoom) { mZoom = zoom; }
    float getZoom() const { return mZoom; }

    void setRoll(float degrees) { mRoll = degrees; }
    float getRoll() const { return mRoll; }

    void setViewModelOffset(float x, float y, float z, float scale) {
        std::lock_guard lock(mMutex);
        mViewModelX = x;
        mViewModelY = y;
        mViewModelZ = z;
        mViewModelScale = scale;
    }

    void getViewModelOffset(float* x, float* y, float* z, float* scale) {
        std::lock_guard lock(mMutex);
        if (x) *x = mViewModelX;
        if (y) *y = mViewModelY;
        if (z) *z = mViewModelZ;
        if (scale) *scale = mViewModelScale;
    }

    void setCinematic(bool cinematic) { mCinematic = cinematic; }
    bool isCinematic() const { return mCinematic; }

private:
    CameraSettings() = default;
    std::atomic<float> mZoom{1.0f};
    std::atomic<float> mRoll{0.0f};
    std::atomic<bool> mCinematic{false};

    std::mutex mMutex;
    float mViewModelX{0.0f};
    float mViewModelY{0.0f};
    float mViewModelZ{0.0f};
    float mViewModelScale{1.0f};
};

} // namespace fabric::camera
