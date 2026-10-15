#pragma once
#include <cstdint>
#include "mc/world/level/BlockPos.h"

class BlockSource;
class Block;
class ActorBlockSyncMessage;
class Actor;

enum class BlockChangedEventTarget : int32_t {
    All = 0,
    AllExceptOriginatingActor = 1,
    OnlyOriginatingActor = 2
};

class BlockSourceListener {
public:
    virtual ~BlockSourceListener() = default;

    virtual void onSourceCreated(BlockSource& source) { (void)source; }
    virtual void onSourceDestroyed(BlockSource& source) { (void)source; }

    virtual void onBlockChanged(
        BlockSource& source,
        BlockPos const& pos,
        uint32_t layer,
        Block const& block,
        Block const& oldBlock,
        int updateFlags,
        ActorBlockSyncMessage const* syncMsg,
        BlockChangedEventTarget target,
        Actor* blockEntity
    ) {
        (void)source; (void)pos; (void)layer; (void)block; (void)oldBlock;
        (void)updateFlags; (void)syncMsg; (void)target; (void)blockEntity;
    }
};
