#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerEvents.h"
#include "ll/api/event/world/WorldEvents.h"
#include "ll/api/chrono/GameTickScheduler.h"
#include <atomic>

namespace levilamroid::hooks {

bool installUIHooks();
bool installRenderHooks();

static std::atomic<bool> sIsInWorld = false;
static std::atomic<bool> sIsMainMenu = false;
static std::atomic<bool> sIsLoading = true; // True at initial boot

void setInWorld(bool inWorld) {
    sIsInWorld.store(inWorld);
    if (inWorld) {
        sIsMainMenu.store(false);
        sIsLoading.store(false);
    }
}

void setIsMainMenu(bool isMenu) {
    sIsMainMenu.store(isMenu);
    if (isMenu) {
        sIsInWorld.store(false);
        sIsLoading.store(false);
    }
}

void setIsLoading(bool loading) {
    sIsLoading.store(loading);
    if (loading) {
        sIsMainMenu.store(false);
    }
}

bool isInWorld() {
    return sIsInWorld.load();
}

bool isMainMenu() {
    if (sIsLoading.load()) return false;
    if (sIsInWorld.load()) return false;
    return sIsMainMenu.load();
}

bool isLoading() {
    return sIsLoading.load();
}

static void* sOrigLocalPlayerMove = nullptr;
static void* sOrigPlayerNormalTick = nullptr;
static void* sOrigLevelTick = nullptr;
static void* sOrigActorNormalTick = nullptr;
static void* sOrigGameModeDestroyBlock = nullptr;
static void* sOrigGameModeUseItem = nullptr;

static void hooked_actorNormalTick(void* self) {
    setInWorld(true);
    if (sOrigActorNormalTick) {
        reinterpret_cast<void(*)(void*)>(sOrigActorNormalTick)(self);
    }
}

static void hooked_localPlayerMove(void* self, const Vec3& delta) {
    setInWorld(true);
    ll::event::PlayerMoveEvent event(reinterpret_cast<Player*>(self), delta);
    ll::event::EventBus::getInstance().publish(event);

    if (!event.isCancelled() && sOrigLocalPlayerMove) {
        reinterpret_cast<void(*)(void*, const Vec3&)>(sOrigLocalPlayerMove)(self, event.getDelta());
    }
}

static void hooked_playerNormalTick(void* self) {
    setInWorld(true);
    ll::event::PlayerTickEvent event(reinterpret_cast<Player*>(self));
    ll::event::EventBus::getInstance().publish(event);

    // Run game tick scheduler tasks
    ll::chrono::GameTickScheduler::getInstance().tick();

    if (!event.isCancelled() && sOrigPlayerNormalTick) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayerNormalTick)(self);
    }
}

static void hooked_levelTick(void* self) {
    setInWorld(true);
    ll::event::LevelTickEvent event(reinterpret_cast<Level*>(self));
    ll::event::EventBus::getInstance().publish(event);

    if (!event.isCancelled() && sOrigLevelTick) {
        reinterpret_cast<void(*)(void*)>(sOrigLevelTick)(self);
    }
}

static bool hooked_gameModeDestroyBlock(void* self, const BlockPos& pos, signed char face) {
    ll::event::BlockBreakEvent event(nullptr, pos, nullptr);
    ll::event::EventBus::getInstance().publish(event);

    if (event.isCancelled()) {
        return false;
    }
    if (sOrigGameModeDestroyBlock) {
        return reinterpret_cast<bool(*)(void*, const BlockPos&, signed char)>(sOrigGameModeDestroyBlock)(self, pos, face);
    }
    return false;
}

static bool hooked_gameModeUseItem(void* self, void* itemStack) {
    ll::event::PlayerUseItemEvent event(nullptr, itemStack);
    ll::event::EventBus::getInstance().publish(event);

    if (event.isCancelled()) {
        return false;
    }
    if (sOrigGameModeUseItem) {
        return reinterpret_cast<bool(*)(void*, void*)>(sOrigGameModeUseItem)(self, itemStack);
    }
    return false;
}

[[maybe_unused]] static bool installGameplayHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    // LocalPlayer::move
    // Raw RVA: 1.26.44.3: 0x0EA8E37C | 1.26.45.1: 0x0EA8DF44 | 1.26.51.1: 0x0F2870A8 | 1.26.52.3: 0x0F287188
    if (off.localPlayerMove) {
        if (ll::memory::hookSignature("FF 03 02 D1 FD 7B 06 A9 F4 4F 07 A9 FD 83 01 91 00 E4 00 2F 54 D0 3B D5 F3 03 00 AA 88 16 40 F9",
                                      (void*)&hooked_localPlayerMove, &sOrigLocalPlayerMove, off.localPlayerMove)) { /* 0x0EA8E37C */
            logger.info("Hooked LocalPlayer::move via signature [Raw RVA: 0x", std::hex, off.localPlayerMove, "]");
        }
    }

    // Player::normalTick
    // Raw RVA: 1.26.44.3: 0x0F0B1BE0 | 1.26.45.1: 0x0F0B17A8 | 1.26.51.1: 0x0F9E25E0 | 1.26.52.3: 0x0F9E26C0
    if (off.playerNormalTick) {
        if (ll::memory::hookSignature("FF C3 00 D1 FD 7B 01 A9 F4 4F 02 A9 FD 43 00 91 54 D0 3B D5 F3 03 00 AA 88 16 40 F9 E8 07 00 F9",
                                      (void*)&hooked_playerNormalTick, &sOrigPlayerNormalTick, off.playerNormalTick)) { /* 0x0F0B1BE0 */
            logger.info("Hooked Player::normalTick via signature [Raw RVA: 0x", std::hex, off.playerNormalTick, "]");
        }
    }

    // Level::tick
    // Raw RVA: 1.26.44.3: 0x0F22326C | 1.26.45.1: 0x0F222E34 | 1.26.51.1: 0x0FB312E4 | 1.26.52.3: 0x0FB313C4
    if (off.levelTick) {
        if (ll::memory::hookSignature("09 78 41 F9 21 7C 40 92 20 0D 40 F9 ? ? ? 14 09 78 41 F9 21 7C 40 92 20 0D 40 F9",
                                      (void*)&hooked_levelTick, &sOrigLevelTick, off.levelTick)) { /* 0x0F22326C */
            logger.info("Hooked Level::tick via signature [Raw RVA: 0x", std::hex, off.levelTick, "]");
        }
    }

    // Actor::normalTick
    // Raw RVA: 1.26.44.3: 0x0EA41838 | 1.26.45.1: 0x0EA41400 | 1.26.51.1: 0x0F27D9B8 | 1.26.52.3: 0x0F27DA98
    if (off.actorNormalTick) {
        if (ll::memory::hookSignature("FF C3 00 D1 FD 7B 01 A9 F4 4F 02 A9 FD 43 00 91 54 D0 3B D5 F3 03 00 AA 88 16 40 F9 E8 07 00 F9",
                                      (void*)&hooked_actorNormalTick, &sOrigActorNormalTick, off.actorNormalTick)) { /* 0x0EA41838 */
            logger.info("Hooked Actor::normalTick via signature [Raw RVA: 0x", std::hex, off.actorNormalTick, "]");
        }
    }


    // GameMode::destroyBlock
    // Raw RVA: 1.26.44.3: 0x0EF6F8F0 | 1.26.45.1: 0x0EF6F4B8 | 1.26.51.1: 0x0F8A02CC | 1.26.52.3: 0x0F8A03AC
    if (off.gameModeDestroyBlock) {
        if (ll::memory::hookSignature("FF 03 02 D1 FD 7B 04 A9 F8 5F 05 A9 F6 57 06 A9 F4 4F 07 A9",
                                      (void*)&hooked_gameModeDestroyBlock, &sOrigGameModeDestroyBlock, off.gameModeDestroyBlock)) {
            logger.info("Hooked GameMode::destroyBlock via signature [Raw RVA: 0x", std::hex, off.gameModeDestroyBlock, "]");
        }
    }

    // GameMode::useItem
    // Raw RVA: 1.26.44.3: 0x04C0B120 | 1.26.45.1: 0x04C0B120 | 1.26.51.1: 0x04C0B120 | 1.26.52.3: 0x04C0B120
    if (off.gameModeUseItem) {
        if (ll::memory::hookSignature("FF 83 01 D1 FD 7B 01 A9 F9 13 00 F9 F8 5F 03 A9 F6 57 04 A9",
                                      (void*)&hooked_gameModeUseItem, &sOrigGameModeUseItem, off.gameModeUseItem)) {
            logger.info("Hooked GameMode::useItem via signature [Raw RVA: 0x", std::hex, off.gameModeUseItem, "]");
        }
    }
    return true;
}

bool installAllHooks() {
    auto& logger = ll::Logger::getInstance();
    logger.info("Installing LeviLamroid Core Engine Hooks...");
    bool ok = true;
    ok = installUIHooks() && ok;
    ok = installRenderHooks() && ok;
    logger.info("All LeviLamroid core engine hooks active.");
    return ok;
}

} // namespace levilamroid::hooks

extern "C" {

LL_API bool ll_is_in_world() {
    return ::levilamroid::hooks::isInWorld();
}

LL_API bool ll_is_main_menu() {
    return ::levilamroid::hooks::isMainMenu();
}

LL_API bool ll_is_loading() {
    return ::levilamroid::hooks::isLoading();
}

} // extern "C"

namespace ll::hooks {

LL_API bool installAllHooks() {
    return ::levilamroid::hooks::installAllHooks();
}

} // namespace ll::hooks
