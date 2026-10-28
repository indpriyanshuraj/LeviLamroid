#pragma once

#include <atomic>
#include <mutex>

namespace fabric::environment {

class EnvironmentSettings {
public:
    static EnvironmentSettings& getInstance() {
        static EnvironmentSettings instance;
        return instance;
    }

    void setWeatherOverride(bool overrideWeather, int weatherType) {
        mOverrideWeather = overrideWeather;
        mWeatherType = weatherType;
    }

    bool hasWeatherOverride() const { return mOverrideWeather; }
    int getWeatherType() const { return mWeatherType; }

    void setTimeOverride(bool overrideTime, float timeOfDay) {
        mOverrideTime = overrideTime;
        mTimeOfDay = timeOfDay;
    }

    bool hasTimeOverride() const { return mOverrideTime; }
    float getTimeOfDay() const { return mTimeOfDay; }

    void setFogEnabled(bool enabled) { mFogEnabled = enabled; }
    bool isFogEnabled() const { return mFogEnabled; }

    void setGlintColor(float r, float g, float b, float a) {
        std::lock_guard lock(mMutex);
        mGlintR = r; mGlintG = g; mGlintB = b; mGlintA = a;
        mCustomGlint = true;
    }

    bool hasCustomGlint() const { return mCustomGlint; }

    void getGlintColor(float* r, float* g, float* b, float* a) {
        std::lock_guard lock(mMutex);
        if (r) *r = mGlintR;
        if (g) *g = mGlintG;
        if (b) *b = mGlintB;
        if (a) *a = mGlintA;
    }

    void resetGlintColor() {
        mCustomGlint = false;
    }

private:
    EnvironmentSettings() = default;

    std::atomic<bool> mOverrideWeather{false};
    std::atomic<int> mWeatherType{0}; // 0: Clear, 1: Rain, 2: Thunder

    std::atomic<bool> mOverrideTime{false};
    std::atomic<float> mTimeOfDay{1000.0f}; // 1000 = Day

    std::atomic<bool> mFogEnabled{true};

    std::atomic<bool> mCustomGlint{false};
    std::mutex mMutex;
    float mGlintR{0.56f};
    float mGlintG{0.14f};
    float mGlintB{1.0f};
    float mGlintA{0.5f};
};

} // namespace fabric::environment
