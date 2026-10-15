#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/level/material/Material.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/biome/biome_color_sampling/BiomeColorSampling.h"

class Block;
class BlockSource;
class Actor;
class ItemInstance;

class BlockType {
public:
    struct PermutationWrapper {
        Block const* mBlock{nullptr};
        explicit operator bool() const noexcept { return mBlock != nullptr; }
        Block const& operator*() const noexcept { return *mBlock; }
        Block const* get() const noexcept { return mBlock; }
    };
    struct PermutationsContainer {
        std::vector<PermutationWrapper> mList;
        [[nodiscard]] std::vector<PermutationWrapper> const& get() const noexcept { return mList; }
        [[nodiscard]] std::vector<PermutationWrapper>& get() noexcept { return mList; }
    };

    HashedString mTypeName{};
    Material     mMaterial{};
    bool         mIsAir{false};
    bool         mIsSlab{false};
    bool         mIsFence{false};
    bool         mIsThinFence{false};
    bool         mIsInteractive{false};
    bool         mIsOpaqueFullBlock{true};
    TintMethod   mTintMethod{TintMethod::None};
    PermutationsContainer mBlockPermutations{};

    BlockType() = default;
    virtual ~BlockType() = default;

    BlockActorType mBlockEntityType{BlockActorType::Undefined};

    [[nodiscard]] HashedString const& getTypeName() const noexcept { return mTypeName; }
    [[nodiscard]] Material const& getMaterial() const noexcept { return mMaterial; }
    [[nodiscard]] bool isAir() const noexcept { return mIsAir; }
    [[nodiscard]] bool isSlabBlock() const noexcept { return mIsSlab; }
    [[nodiscard]] bool isFenceBlock() const noexcept { return mIsFence; }
    [[nodiscard]] bool isThinFenceBlock() const noexcept { return mIsThinFence; }
    [[nodiscard]] bool isInteractiveBlock() const noexcept { return mIsInteractive; }
    [[nodiscard]] BlockActorType getBlockEntityType() const noexcept { return mBlockEntityType; }

    virtual ItemInstance asItemInstance(Block const& block, void* actor) const;

    virtual bool allowStateMismatchOnPlacement(Block const& predicted, Block const& ghost) const {
        (void)predicted; (void)ghost;
        return true;
    }
    virtual bool mayPlace(BlockSource& region, BlockPos const& pos) const {
        (void)region; (void)pos;
        return true;
    }
    virtual Block const& connectionUpdate(
        BlockSource& region,
        Block const& block,
        BlockPos const& pos,
        void const* directions = nullptr
    ) const {
        (void)region; (void)pos; (void)directions;
        return block;
    }
    virtual Block const& connectionUpdate(
        BlockSource& region,
        Block const& block,
        BlockPos const& pos,
        class NeighborBlockDirections const& directions
    ) const {
        (void)region; (void)pos; (void)directions;
        return block;
    }

    virtual Block const& getPlacementBlock(
        Actor const& by,
        BlockPos const& pos,
        unsigned char face,
        Vec3 const& clickPos,
        int itemAux
    ) const;
};
