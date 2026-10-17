#pragma once
#include "mc/client/ClientInstance.h"
#include "mc/world/level/Level.h"
#include "mc/world/actor/player/LocalPlayer.h"

#include <dlfcn.h>

namespace ll::service {

inline ClientInstance* getClientInstance() {
    static ClientInstance sInstance{};
    return &sInstance;
}

inline Level* getLevel() {
    using GetLevelFn = void* (*)();
    static GetLevelFn sFn = (GetLevelFn)dlsym(RTLD_DEFAULT, "ll_service_get_level");
    if (sFn) {
        if (void* lvl = sFn()) return reinterpret_cast<Level*>(lvl);
    }
    static Level sLevel{};
    return &sLevel;
}

inline Level* getMultiPlayerLevel() {
    return getLevel();
}

inline LocalPlayer* getLocalPlayer() {
    using GetPlayerFn = void* (*)();
    static GetPlayerFn sFn = (GetPlayerFn)dlsym(RTLD_DEFAULT, "ll_service_get_local_player");
    if (sFn) {
        if (void* p = sFn()) return reinterpret_cast<LocalPlayer*>(p);
    }
    auto* client = getClientInstance();
    return client ? client->getLocalPlayer() : nullptr;
}

} // namespace ll::service

