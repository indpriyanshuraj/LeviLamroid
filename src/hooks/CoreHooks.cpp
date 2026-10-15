#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerEvents.h"

#include "ll/api/event/world/WorldEvents.h"
#include "ll/api/chrono/GameTickScheduler.h"

namespace levilamroid::hooks {

bool installUIHooks();
bool installRenderHooks();

static void* sOrigLocalPlayerMove = nullptr;
static void* sOrigPlayerNormalTick = nullptr;
static void* sOrigLevelTick = nullptr;
static void* sOrigGameModeDestroyBlock = nullptr;
static void* sOrigGameModeUseItem = nullptr;

static void hooked_localPlayerMove(void* self, const Vec3& delta) {
    ll::event::PlayerMoveEvent event(reinterpret_cast<Player*>(self), delta);
    ll::event::EventBus::getInstance().publish(event);

    if (!event.isCancelled() && sOrigLocalPlayerMove) {
        reinterpret_cast<void(*)(void*, const Vec3&)>(sOrigLocalPlayerMove)(self, event.getDelta());
    }
}

static void hooked_playerNormalTick(void* self) {
    ll::event::PlayerTickEvent event(reinterpret_cast<Player*>(self));
    ll::event::EventBus::getInstance().publish(event);

    // Run game tick scheduler tasks
    ll::chrono::GameTickScheduler::getInstance().tick();

    if (!event.isCancelled() && sOrigPlayerNormalTick) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayerNormalTick)(self);
    }
}

static void hooked_levelTick(void* self) {
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

static bool installGameplayHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.localPlayerMove) {
        if (ll::memory::hookRva(off.localPlayerMove, (void*)&hooked_localPlayerMove, &sOrigLocalPlayerMove)) {
            logger.info("Hooked LocalPlayer::move at RVA 0x", std::hex, off.localPlayerMove);
        }
    }
    if (off.playerNormalTick) {
        if (ll::memory::hookRva(off.playerNormalTick, (void*)&hooked_playerNormalTick, &sOrigPlayerNormalTick)) {
            logger.info("Hooked Player::normalTick at RVA 0x", std::hex, off.playerNormalTick);
        }
    }
    if (off.levelTick) {
        if (ll::memory::hookRva(off.levelTick, (void*)&hooked_levelTick, &sOrigLevelTick)) {
            logger.info("Hooked Level::tick at RVA 0x", std::hex, off.levelTick);
        }
    }
    if (off.gameModeDestroyBlock) {
        if (ll::memory::hookRva(off.gameModeDestroyBlock, (void*)&hooked_gameModeDestroyBlock, &sOrigGameModeDestroyBlock)) {
            logger.info("Hooked GameMode::destroyBlock at RVA 0x", std::hex, off.gameModeDestroyBlock);
        }
    }
    if (off.gameModeUseItem) {
        if (ll::memory::hookRva(off.gameModeUseItem, (void*)&hooked_gameModeUseItem, &sOrigGameModeUseItem)) {
            logger.info("Hooked GameMode::useItem at RVA 0x", std::hex, off.gameModeUseItem);
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
    ok = installGameplayHooks() && ok;
    logger.info("All LeviLamroid core engine hooks active.");
    return ok;
}

} // namespace levilamroid::hooks

namespace ll::hooks {

LL_API bool installAllHooks() {
    return ::levilamroid::hooks::installAllHooks();
}

} // namespace ll::hooks
