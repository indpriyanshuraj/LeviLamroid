#pragma once
#include <cstdint>
#include "mc/world/level/BlockPos.h"

class ActorBlockSyncMessage {
public:
    uint64_t mActorUniqueId{0};
    BlockPos mBlockPos{0, 0, 0};
    uint32_t mBlockRuntimeId{0};
    uint32_t mMessageId{0};

    ActorBlockSyncMessage() = default;
};
