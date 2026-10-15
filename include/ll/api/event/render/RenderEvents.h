#pragma once
#include "levilamroid/api/event/Event.h"
#include <cstdint>

namespace ll::event {

class GlClearEvent : public Event {
private:
    uint32_t mMask;
    int mViewport[4];

public:
    GlClearEvent(uint32_t mask, const int vp[4]) : mMask(mask) {
        mViewport[0] = vp[0];
        mViewport[1] = vp[1];
        mViewport[2] = vp[2];
        mViewport[3] = vp[3];
    }

    uint32_t getMask() const { return mMask; }
    int getX() const { return mViewport[0]; }
    int getY() const { return mViewport[1]; }
    int getWidth() const { return mViewport[2]; }
    int getHeight() const { return mViewport[3]; }
};

class CubemapRenderEvent : public Event {
public:
    CubemapRenderEvent() = default;
};

class ProgressBarRenderEvent : public Event {
private:
    float mPrimaryColor[4];
    float mSecondaryColor[4];
    bool mColorModified = false;

public:
    ProgressBarRenderEvent() = default;

    void setPrimaryColor(float r, float g, float b, float a = 1.0f) {
        mPrimaryColor[0] = r; mPrimaryColor[1] = g; mPrimaryColor[2] = b; mPrimaryColor[3] = a;
        mColorModified = true;
    }

    void setSecondaryColor(float r, float g, float b, float a = 1.0f) {
        mSecondaryColor[0] = r; mSecondaryColor[1] = g; mSecondaryColor[2] = b; mSecondaryColor[3] = a;
        mColorModified = true;
    }

    bool hasColorModified() const { return mColorModified; }
    const float* getPrimaryColor() const { return mPrimaryColor; }
    const float* getSecondaryColor() const { return mSecondaryColor; }
};

} // namespace ll::event
