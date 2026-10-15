#pragma once
#include <memory>
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/BlockType.h"
#include "mc/world/level/block/actor/BlockActor.h"
#include "mc/world/level/block/actor/ChestBlockActor.h"

class Block;

class VanillaBlockActorFactory {
public:
    static std::shared_ptr<BlockActor> createBlockActor(
        BlockActorType type,
        BlockPos const& pos,
        Block const& block
    ) {
        (void)block;
        if (type == BlockActorType::Chest) {
            return std::make_shared<ChestBlockActor>(pos);
        }
        return std::make_shared<BlockActor>(type, pos);
    }

    static std::shared_ptr<BlockActor> createBlockActor(
        BlockPos const& pos,
        BlockType const& blockType
    ) {
        auto type = blockType.getBlockEntityType();
        if (type == BlockActorType::Chest) {
            return std::make_shared<ChestBlockActor>(pos);
        }
        return std::make_shared<BlockActor>(type, pos);
    }
};

