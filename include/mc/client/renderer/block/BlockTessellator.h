#pragma once
#include "mc/client/renderer/Tessellator.h"
#include "mc/world/level/BlockPos.h"
#include <cstdint>
#include <functional>

class Block;
class BlockSource;

class BlockTessellator {
public:
    struct CachedGetBlockWrapper {
        std::function<Block const&(BlockPos const&)> fn;
        std::function<Block const&(BlockPos const&)>& get() { return fn; }
    };

    CachedGetBlockWrapper mCachedGetBlock;
    int mRenderingLayer{0};

    BlockTessellator(BlockSource* region = nullptr) { (void)region; }
    virtual ~BlockTessellator() = default;

    void setRegion(BlockSource& region) { (void)region; }

    virtual bool tessellateBlockInWorld(
        BlockSource& region,
        Block const& block,
        BlockPos const& pos
    ) {
        (void)region; (void)block; (void)pos;
        return true;
    }

    virtual bool tessellateInWorld(
        Tessellator& tessellator,
        Block const& block,
        BlockPos const& pos,
        bool b = false,
        void* unk = nullptr
    ) {
        (void)tessellator; (void)block; (void)pos; (void)b; (void)unk;
        return true;
    }
};
