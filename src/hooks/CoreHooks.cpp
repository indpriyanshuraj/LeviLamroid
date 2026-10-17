#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerEvents.h"
#include "ll/api/event/world/WorldEvents.h"
#include "ll/api/chrono/GameTickScheduler.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/resource/ResourceManager.h"
#include <atomic>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <dlfcn.h>
#include <sys/types.h>

namespace levilamroid::hooks {

bool installUIHooks();
bool installRenderHooks();
bool installResourceHooks();
void dispatchStartMenuOpen();
} // namespace levilamroid::hooks

namespace levilamroid::item {
bool installItemHooks();
}

namespace levilamroid::hooks {

static std::atomic<bool> sIsInWorld = false;
static std::atomic<bool> sIsMainMenu = false;
static std::atomic<bool> sIsLoading = true; // True at initial boot
static std::atomic<bool> sCameraLocked = false;

static void* sCurrentLocalPlayer = nullptr;
static void* sCurrentClientInstance = nullptr;
static void* sCurrentLevel = nullptr;

static void* sOrigClientInstanceUpdate = nullptr;
static void* sOrigPlayerNormalTick = nullptr;
static void* sOrigApplyTurnDelta = nullptr;
static void* sOrigCommandRegistryRegisterCommand = nullptr;
static void* sEngineCommandRegistry = nullptr;

static std::unordered_map<std::string, std::function<void(const std::string&)>> sClientCommands;
static std::mutex sClientCommandMutex;

LL_API void ll_command_register_client(const char* name, void (*handler)(const char* args)) {
    if (!name || !handler) return;
    std::lock_guard<std::mutex> lock(sClientCommandMutex);
    std::string key = name;
    if (!key.empty() && key[0] == '/') key = key.substr(1);
    sClientCommands[key] = [handler](const std::string& args) {
        handler(args.c_str());
    };
    ll::Logger::getInstance().info("Registered native client command: /", key);
}

LL_API bool ll_command_dispatch_client(const char* fullCommand) {
    if (!fullCommand) return false;
    std::string cmd = fullCommand;
    if (!cmd.empty() && (cmd[0] == '/' || cmd[0] == '!')) cmd = cmd.substr(1);

    auto spacePos = cmd.find(' ');
    std::string baseCmd = (spacePos == std::string::npos) ? cmd : cmd.substr(0, spacePos);
    std::string args = (spacePos == std::string::npos) ? "" : cmd.substr(spacePos + 1);

    // 1. Check ll::command::CommandRegistrar first
    if (ll::command::CommandRegistrar::getInstance().dispatchCommand(fullCommand)) {
        return true;
    }
    if (ll::command::CommandRegistrar::getInstance().dispatchCommand(cmd)) {
        return true;
    }

    // 2. Check sClientCommands map
    std::function<void(const std::string&)> cb;
    {
        std::lock_guard<std::mutex> lock(sClientCommandMutex);
        auto it = sClientCommands.find(baseCmd);
        if (it != sClientCommands.end()) {
            cb = it->second;
        }
    }

    if (cb) {
        cb(args);
        return true;
    }
    return false;
}



void setInWorld(bool inWorld) {
    sIsInWorld.store(inWorld);
    if (inWorld) {
        sIsMainMenu.store(false);
        sIsLoading.store(false);
    } else {
        sCurrentLocalPlayer = nullptr;
        sCurrentLevel = nullptr;
    }
}

void setIsMainMenu(bool isMenu) {
    sIsMainMenu.store(isMenu);
    if (isMenu) {
        sIsInWorld.store(false);
        sIsLoading.store(false);
        sCurrentLocalPlayer = nullptr;
        sCurrentLevel = nullptr;
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

static void hooked_clientInstanceUpdate(void* self) {
    sCurrentClientInstance = self;
    if (!sIsInWorld.load()) {
        sIsMainMenu.store(true);
        sIsLoading.store(false);
        static bool sFirstMenuOpen = false;
        if (!sFirstMenuOpen) {
            sFirstMenuOpen = true;
            dispatchStartMenuOpen();
        }
    }
    if (sOrigClientInstanceUpdate) {
        reinterpret_cast<void(*)(void*)>(sOrigClientInstanceUpdate)(self);
    }
}

static void hooked_playerNormalTick(void* self) {
    sCurrentLocalPlayer = self;
    if (self) {
        sCurrentLevel = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 472);
    }
    if (!sIsInWorld.load()) {
        setInWorld(true);
    }

    // Publish PlayerTickEvent
    ll::event::PlayerTickEvent ev(reinterpret_cast<Player*>(self));
    ll::event::EventBus::getInstance().publish(ev);

    if (sOrigPlayerNormalTick) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayerNormalTick)(self);
    }
}

static void hooked_applyTurnDelta(void* self, void* delta) {
    if (sCameraLocked.load()) {
        return; // Freeze camera orientation when camera locked
    }
    if (sOrigApplyTurnDelta) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigApplyTurnDelta)(self, delta);
    }
}

static bool installGameplayHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.clientInstanceUpdate) {
        if (ll::memory::hookRva(off.clientInstanceUpdate, (void*)&hooked_clientInstanceUpdate, &sOrigClientInstanceUpdate)) {
            logger.info("Hooked ClientInstance::update [RVA: 0x", std::hex, off.clientInstanceUpdate, "]");
        }
    }

    if (off.playerNormalTick) {
        if (ll::memory::hookRva(off.playerNormalTick, (void*)&hooked_playerNormalTick, &sOrigPlayerNormalTick)) {
            logger.info("Hooked Player::normalTick [RVA: 0x", std::hex, off.playerNormalTick, "]");
        }
    }

    if (off.localPlayerApplyTurnDelta) {
        if (ll::memory::hookRva(off.localPlayerApplyTurnDelta, (void*)&hooked_applyTurnDelta, &sOrigApplyTurnDelta)) {
            logger.info("Hooked LocalPlayer::applyTurnDelta [RVA: 0x", std::hex, off.localPlayerApplyTurnDelta, "]");
        }
    }

    logger.info("LeviLamroid gameplay subsystem active.");
    return true;
}

bool installAllHooks() {
    auto& logger = ll::Logger::getInstance();
    logger.info("Installing LeviLamroid Core Engine Hooks...");
    bool ok = true;
    ok = installUIHooks() && ok;
    ok = installRenderHooks() && ok;
    ok = installGameplayHooks() && ok;
    ok = installResourceHooks() && ok;
    ok = ::levilamroid::item::installItemHooks() && ok;
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

LL_API void* ll_service_get_local_player() {
    return ::levilamroid::hooks::sCurrentLocalPlayer;
}

LL_API void* ll_service_get_client_instance() {
    return ::levilamroid::hooks::sCurrentClientInstance;
}

LL_API void* ll_service_get_level() {
    return ::levilamroid::hooks::sCurrentLevel;
}

LL_API void ll_camera_set_locked(bool locked) {
    ::levilamroid::hooks::sCameraLocked.store(locked);
}

LL_API bool ll_camera_is_locked() {
    return ::levilamroid::hooks::sCameraLocked.load();
}

LL_API void ll_command_register_native(const char* name, const char* desc) {
    if (!name) return;
    if (::levilamroid::hooks::sEngineCommandRegistry && ::levilamroid::hooks::sOrigCommandRegistryRegisterCommand) {
        std::string n = name;
        const char* d = desc ? desc : "";
        reinterpret_cast<void(*)(void*, const std::string&, const char*, int, int, int)>(
            ::levilamroid::hooks::sOrigCommandRegistryRegisterCommand)(
            ::levilamroid::hooks::sEngineCommandRegistry, n, d, 0, 0, 0);
    }
}

LL_API void* ll_command_get_engine_registry() {
    return ::levilamroid::hooks::sEngineCommandRegistry;
}

} // extern "C"

namespace ll::hooks {

LL_API bool installAllHooks() {
    return ::levilamroid::hooks::installAllHooks();
}

} // namespace ll::hooks
