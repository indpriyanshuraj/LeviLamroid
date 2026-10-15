#pragma once
#include "mc/world/level/block/actor/BlockActor.h"

class ChestBlockActor : public BlockActor {
public:
    ChestBlockActor* mLargeChestPaired{nullptr};
    bool             mIsLarge{false};
    bool             mIsLead{false};

    ChestBlockActor() : BlockActor(BlockActorType::Chest) {}
    explicit ChestBlockActor(BlockPos const& pos) : BlockActor(BlockActorType::Chest, pos) {}
    ~ChestBlockActor() override = default;

    virtual void _tryToPairWith(BlockSource& region, BlockPos const& neighbor) { (void)region; (void)neighbor; }
    virtual void unpair() {}
};
