#pragma once

#include "ll/api/event/Event.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/service/Bedrock.h"
#include <cstdint>

namespace fabric::event {

class HudRenderEvent : public ::ll::event::Event {
private:
    void* mScreenContext;
    const void* mFrameRenderObject;

public:
    HudRenderEvent(void* screenCtx, const void* frameObj)
        : mScreenContext(screenCtx), mFrameRenderObject(frameObj) {}

    [[nodiscard]] void* getScreenContext() const noexcept { return mScreenContext; }
    [[nodiscard]] const void* getFrameRenderObject() const noexcept { return mFrameRenderObject; }
};

struct PlayerHungerStats {
    int foodLevel{20};
    float saturation{5.0f};
    float exhaustion{0.0f};
    bool isStarving{false};
};

class PlayerStats {
public:
    static PlayerHungerStats getHungerStats() {
        PlayerHungerStats stats{};
        auto* player = ::ll::service::getLocalPlayer();
        if (!player) return stats;

        // Dynamic resolution for hunger accessor or direct component read
        using GetHungerFn = int (*)(const void*);
        static auto sGetHunger = reinterpret_cast<GetHungerFn>(::ll::memory::findMinecraftSymbol("_ZNK6Player9getHungerEv"));
        if (sGetHunger) {
            stats.foodLevel = sGetHunger(player);
        } else {
            stats.foodLevel = 20; // Default full
        }

        using GetSatFn = float (*)(const void*);
        static auto sGetSat = reinterpret_cast<GetSatFn>(::ll::memory::findMinecraftSymbol("_ZNK6Player13getSaturationEv"));
        if (sGetSat) {
            stats.saturation = sGetSat(player);
        } else {
            stats.saturation = 5.0f;
        }

        using GetExhFn = float (*)(const void*);
        static auto sGetExh = reinterpret_cast<GetExhFn>(::ll::memory::findMinecraftSymbol("_ZNK6Player13getExhaustionEv"));
        if (sGetExh) {
            stats.exhaustion = sGetExh(player);
        }

        stats.isStarving = (stats.foodLevel <= 0);
        return stats;
    }

    static int getHealth() {
        auto* player = ::ll::service::getLocalPlayer();
        return player ? player->getHealth() : 20;
    }
};

} // namespace fabric::event

extern "C" {
typedef void (*FabricHudRenderCallback)(void* screenContext, const void* frameObj);
void fabric_register_hud_render_callback(FabricHudRenderCallback cb);
}
