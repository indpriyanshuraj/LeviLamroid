#pragma once
#include "mc/client/ClientInstance.h"
#include "mc/world/level/Level.h"
#include "mc/world/actor/player/LocalPlayer.h"

namespace ll::service {

inline ClientInstance* getClientInstance() {
    static ClientInstance sInstance{};
    return &sInstance;
}

inline Level* getLevel() {
    static Level sLevel{};
    return &sLevel;
}

inline Level* getMultiPlayerLevel() {
    static Level sMultiPlayerLevel{};
    return &sMultiPlayerLevel;
}

inline LocalPlayer* getLocalPlayer() {
    auto* client = getClientInstance();
    return client ? client->getLocalPlayer() : nullptr;
}

} // namespace ll::service
