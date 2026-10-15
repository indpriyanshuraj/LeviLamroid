#pragma once
#include <cstdint>
#include <memory>
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/dataloadhelper/NewUniqueIdsDataLoadHelper.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/component/IVanillaRenderBlockActorComponent.h"

class BlockSource;

class BlockActor {
public:
    BlockActorType mType{BlockActorType::Undefined};
    BlockPos       mPosition{0, 0, 0};

    BlockActor() = default;
    explicit BlockActor(BlockActorType type, BlockPos const& pos = {})
        : mType(type), mPosition(pos) {}

    virtual ~BlockActor() = default;

    virtual void load(CompoundTag const& tag, DataLoadHelper& helper) { (void)tag; (void)helper; }
    virtual void load(class Level& level, CompoundTag const& tag, DataLoadHelper& helper) { (void)level; (void)tag; (void)helper; }
    virtual bool save(CompoundTag& tag) const { (void)tag; return true; }
    virtual IVanillaRenderBlockActorComponent* getVanillaRenderComponent() { return nullptr; }
    virtual IVanillaRenderBlockActorComponent* _getRenderComponent() { return nullptr; }
    virtual void onChanged(BlockSource& region) { (void)region; }
    [[nodiscard]] virtual BlockPos const& getPosition() const noexcept { return mPosition; }
    [[nodiscard]] virtual BlockActorType getType() const noexcept { return mType; }
};
