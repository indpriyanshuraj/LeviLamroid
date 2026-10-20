#pragma once

#define LEVILAMROID_VERSION_MAJOR 1
#define LEVILAMROID_VERSION_MINOR 0
#define LEVILAMROID_VERSION_PATCH 3
#define LEVILAMROID_VERSION_STRING "1.0.3-beta"

// LeviLamina Windows Parity Headers
#include "ll/api/Logger.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ui/UIEvents.h"
#include "ll/api/event/render/RenderEvents.h"
#include "ll/api/event/sound/SoundEvents.h"
#include "ll/api/event/player/PlayerEvents.h"
#include "ll/api/event/world/WorldEvents.h"
#include "ll/api/event/entity/ActorEvents.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/chrono/GameTickScheduler.h"
#include "ll/api/form/Form.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/thread/ThreadPoolExecutor.h"
#include "ll/api/plugin/PluginManager.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

// LeviLamroid Native Android Extensions
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/memory/PatternScan.h"
#include "levilamroid/api/sound/LevelSoundEvent.h"
#include "levilamroid/api/sound/SoundEngine.h"
#include "levilamroid/api/sound/FmodSound.h"

// High-level human-friendly engine state & hooks
#include "ll/api/ui/UI.h"
#include "ll/api/render/Render.h"

// Item & Creative Inventory Subsystem
#include "levilamroid/api/math/HashedString.h"
#include "levilamroid/api/item/CreativeItemCategory.h"
#include "levilamroid/api/item/CreativeGroup.h"
#include "levilamroid/api/item/IconInfo.h"
#include "levilamroid/api/item/Item.h"
#include "levilamroid/api/item/ItemStack.h"
#include "levilamroid/api/item/ItemRegistry.h"

// Block Subsystem
#include "levilamroid/api/block/BlockGroup.h"
#include "levilamroid/api/block/BlockPos.h"
#include "levilamroid/api/block/BlockLegacy.h"
#include "levilamroid/api/block/Block.h"
#include "levilamroid/api/block/CustomBlockDefinition.h"
#include "levilamroid/api/block/BlockTypeRegistry.h"

extern "C" {
LL_API bool ll_is_in_world();
LL_API bool ll_is_main_menu();
LL_API bool ll_is_loading();
}

// Canonical LeviLamroid namespace: ll (standard LeviLamina modding namespace)
namespace ll {

/// Initialize the LeviLamroid runtime, discover libminecraftpe.so, detect version, and install core hooks
bool init();

/// Shutdown and clean up hooks
void shutdown();

/// Human function: Check if player is currently in-world
inline bool isInWorld() {
    using Fn = bool (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(ui::getLoaderHandle(), "ll_is_in_world"));
    if (fn) return fn();
    return false;
}

/// Human function: Check if client is currently in main menu
inline bool isMainMenu() {
    using Fn = bool (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(ui::getLoaderHandle(), "ll_is_main_menu"));
    if (fn) return fn();
    return false;
}

/// Human function: Check if client is currently loading resources/world
inline bool isLoading() {
    using Fn = bool (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(ui::getLoaderHandle(), "ll_is_loading"));
    if (fn) return fn();
    return false;
}

// Engine subsystem aliases under ll::
namespace sound = ::levilamroid::sound;
namespace mcpe  = ::levilamroid::mcpe;

} // namespace ll

// Alias levilamroid and llntol to ll for backwards compatibility
namespace levilamroid {
    using namespace ll;
    using ll::init;
    using ll::shutdown;
}

namespace llntol {
    using namespace ll;
    using ll::init;
    using ll::shutdown;
}
