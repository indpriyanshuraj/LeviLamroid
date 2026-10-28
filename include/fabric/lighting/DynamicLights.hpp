#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace fabric::lighting {

class DynamicLightsEngine {
public:
    static DynamicLightsEngine& getInstance() {
        static DynamicLightsEngine instance;
        return instance;
    }

    void setEnabled(bool enabled) { mEnabled.store(enabled, std::memory_order_relaxed); }
    bool isEnabled() const { return mEnabled.load(std::memory_order_relaxed); }

    void registerLightEmitter(const std::string& itemIdentifier, int lightLevel) {
        std::lock_guard lock(mMutex);
        mLightEmitters[itemIdentifier] = std::clamp(lightLevel, 0, 15);
    }

    int getItemLightLevel(const std::string& itemIdentifier) {
        if (itemIdentifier.empty()) return 0;
        {
            std::lock_guard lock(mMutex);
            auto it = mLightEmitters.find(itemIdentifier);
            if (it != mLightEmitters.end()) {
                return it->second;
            }
        }

        // Substring / fuzzy heuristic fallback for item/tile description IDs
        std::string_view s = itemIdentifier;
        if (s.find("soul_torch") != std::string_view::npos) return 10;
        if (s.find("redstone_torch") != std::string_view::npos) return 9;
        if (s.find("torch") != std::string_view::npos) return 14;
        if (s.find("soul_lantern") != std::string_view::npos) return 10;
        if (s.find("lantern") != std::string_view::npos) return 15;
        if (s.find("glowstone") != std::string_view::npos) return 15;
        if (s.find("sea_lantern") != std::string_view::npos) return 15;
        if (s.find("lava") != std::string_view::npos) return 15;
        if (s.find("soul_campfire") != std::string_view::npos) return 10;
        if (s.find("campfire") != std::string_view::npos) return 15;
        if (s.find("shroomlight") != std::string_view::npos) return 15;
        if (s.find("froglight") != std::string_view::npos) return 15;
        if (s.find("jack_o_lantern") != std::string_view::npos) return 15;
        if (s.find("beacon") != std::string_view::npos) return 15;
        if (s.find("glow_berries") != std::string_view::npos) return 12;
        if (s.find("crying_obsidian") != std::string_view::npos) return 10;
        if (s.find("magma") != std::string_view::npos) return 3;
        if (s.find("fire_charge") != std::string_view::npos) return 12;
        if (s.find("glow_ink") != std::string_view::npos) return 6;
        if (s.find("amethyst") != std::string_view::npos) return 5;
        return 0;
    }

    void setPlayerLight(float x, float y, float z, int level) {
        mPlayerX.store(x, std::memory_order_relaxed);
        mPlayerY.store(y, std::memory_order_relaxed);
        mPlayerZ.store(z, std::memory_order_relaxed);
        mPlayerLevel.store(std::clamp(level, 0, 15), std::memory_order_relaxed);
    }

    void addTransientSource(float x, float y, float z, int level) {
        std::lock_guard lock(mTransientMutex);
        size_t count = mTransientCount.load(std::memory_order_relaxed);
        if (count < MAX_TRANSIENT_SOURCES) {
            mTransientSources[count] = {x, y, z, std::clamp(level, 0, 15)};
            mTransientCount.store(count + 1, std::memory_order_release);
        }
    }

    void clearTransientSources() {
        std::lock_guard lock(mTransientMutex);
        mTransientCount.store(0, std::memory_order_release);
    }

    // Completely lock-free, zero-allocation computation safe for chunk worker threads
    float computeDynamicBrightness(int posX, int posY, int posZ, float baseBrightness) {
        if (!mEnabled.load(std::memory_order_relaxed)) return baseBrightness;

        float bestBrightness = baseBrightness;
        int lvl = mPlayerLevel.load(std::memory_order_relaxed);
        if (lvl > 0) {
            float px = mPlayerX.load(std::memory_order_relaxed);
            float py = mPlayerY.load(std::memory_order_relaxed);
            float pz = mPlayerZ.load(std::memory_order_relaxed);

            float dx = static_cast<float>(posX) - px;
            float dy = static_cast<float>(posY) - py;
            float dz = static_cast<float>(posZ) - pz;
            float distSq = dx * dx + dy * dy + dz * dz;
            float maxR = static_cast<float>(lvl);

            if (distSq < maxR * maxR) {
                float dist = std::sqrt(distSq);
                float light = static_cast<float>(lvl) - dist;
                if (light > 0.0f) {
                    float dyn = light / 15.0f;
                    if (dyn > bestBrightness) {
                        bestBrightness = std::min(1.0f, dyn);
                    }
                }
            }
        }

        // Lock-free check for transient sources
        size_t count = mTransientCount.load(std::memory_order_acquire);
        for (size_t i = 0; i < count && i < MAX_TRANSIENT_SOURCES; ++i) {
            const auto& src = mTransientSources[i];
            if (src.level <= 0) continue;
            float tdx = static_cast<float>(posX) - src.x;
            float tdy = static_cast<float>(posY) - src.y;
            float tdz = static_cast<float>(posZ) - src.z;
            float distSq = tdx * tdx + tdy * tdy + tdz * tdz;
            float maxR = static_cast<float>(src.level);
            if (distSq < maxR * maxR) {
                float dist = std::sqrt(distSq);
                float light = static_cast<float>(src.level) - dist;
                if (light > 0.0f) {
                    float dyn = light / 15.0f;
                    if (dyn > bestBrightness) {
                        bestBrightness = std::min(1.0f, dyn);
                    }
                }
            }
        }

        return bestBrightness;
    }

private:
    DynamicLightsEngine() {
        // Register canonical vanilla light emitters
        mLightEmitters["minecraft:torch"] = 14;
        mLightEmitters["minecraft:soul_torch"] = 10;
        mLightEmitters["minecraft:lantern"] = 15;
        mLightEmitters["minecraft:soul_lantern"] = 10;
        mLightEmitters["minecraft:glowstone"] = 15;
        mLightEmitters["minecraft:sea_lantern"] = 15;
        mLightEmitters["minecraft:lava_bucket"] = 15;
        mLightEmitters["minecraft:campfire"] = 15;
        mLightEmitters["minecraft:soul_campfire"] = 10;
        mLightEmitters["minecraft:shroomlight"] = 15;
        mLightEmitters["minecraft:redstone_torch"] = 9;
        mLightEmitters["minecraft:glow_berries"] = 12;
        mLightEmitters["minecraft:ochre_froglight"] = 15;
        mLightEmitters["minecraft:verdant_froglight"] = 15;
        mLightEmitters["minecraft:pearlescent_froglight"] = 15;
        mLightEmitters["minecraft:jack_o_lantern"] = 15;
        mLightEmitters["minecraft:beacon"] = 15;
        mLightEmitters["minecraft:crying_obsidian"] = 10;
        mLightEmitters["minecraft:magma"] = 3;
        mLightEmitters["minecraft:magma_block"] = 3;
        mLightEmitters["minecraft:fire_charge"] = 12;
    }

    std::atomic<bool> mEnabled{true};
    std::atomic<float> mPlayerX{0.0f};
    std::atomic<float> mPlayerY{0.0f};
    std::atomic<float> mPlayerZ{0.0f};
    std::atomic<int> mPlayerLevel{0};

    std::mutex mMutex;
    std::unordered_map<std::string, int> mLightEmitters;

    struct LightPoint {
        float x{0.0f};
        float y{0.0f};
        float z{0.0f};
        int level{0};
    };
    static constexpr size_t MAX_TRANSIENT_SOURCES = 16;
    LightPoint mTransientSources[MAX_TRANSIENT_SOURCES];
    std::atomic<size_t> mTransientCount{0};
    std::mutex mTransientMutex;
};

} // namespace fabric::lighting
