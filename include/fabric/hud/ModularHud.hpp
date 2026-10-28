#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <cmath>

namespace fabric::hud {

using HudWidgetCallback = void (*)(void* screenCtx, const void* frameObj);

struct HudWidget {
    std::string id;
    HudWidgetCallback callback;
};

class ModularHudRegistry {
public:
    static ModularHudRegistry& getInstance() {
        static ModularHudRegistry instance;
        return instance;
    }

    void registerWidget(const std::string& id, HudWidgetCallback cb) {
        if (!cb) return;
        std::lock_guard lock(mMutex);
        for (auto& w : mWidgets) {
            if (w.id == id) {
                w.callback = cb;
                return;
            }
        }
        mWidgets.push_back({id, cb});
    }

    void unregisterWidget(const std::string& id) {
        std::lock_guard lock(mMutex);
        mWidgets.erase(
            std::remove_if(mWidgets.begin(), mWidgets.end(), [&](const HudWidget& w) { return w.id == id; }),
            mWidgets.end()
        );
    }

    void renderAll(void* screenCtx, const void* frameObj) {
        std::lock_guard lock(mMutex);
        for (const auto& w : mWidgets) {
            if (w.callback) {
                try {
                    w.callback(screenCtx, frameObj);
                } catch (...) {}
            }
        }
    }

    static const char* computeCardinalDirection(float yaw) {
        // Normalize yaw to [0, 360)
        float normalized = std::fmod(yaw, 360.0f);
        if (normalized < 0.0f) normalized += 360.0f;

        // Bedrock yaw conventions:
        // ~0: South (+Z)
        // ~90: West (-X)
        // ~180: North (-Z)
        // ~270: East (+X)
        if (normalized >= 337.5f || normalized < 22.5f) return "South (+Z)";
        if (normalized >= 22.5f && normalized < 67.5f) return "South-West";
        if (normalized >= 67.5f && normalized < 112.5f) return "West (-X)";
        if (normalized >= 112.5f && normalized < 157.5f) return "North-West";
        if (normalized >= 157.5f && normalized < 202.5f) return "North (-Z)";
        if (normalized >= 202.5f && normalized < 247.5f) return "North-East";
        if (normalized >= 247.5f && normalized < 292.5f) return "East (+X)";
        return "South-East";
    }

private:
    ModularHudRegistry() = default;
    std::mutex mMutex;
    std::vector<HudWidget> mWidgets;
};

} // namespace fabric::hud
