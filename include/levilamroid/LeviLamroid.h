#pragma once

#define LEVILAMROID_VERSION_MAJOR 1
#define LEVILAMROID_VERSION_MINOR 0
#define LEVILAMROID_VERSION_PATCH 0
#define LEVILAMROID_VERSION_STRING "1.0.0-beta"

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

// Canonical LeviLamroid namespace: ll (standard LeviLamina modding namespace)
namespace ll {

/// Initialize the LeviLamroid runtime, discover libminecraftpe.so, detect version, and install core hooks
bool init();

/// Shutdown and clean up hooks
void shutdown();

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
