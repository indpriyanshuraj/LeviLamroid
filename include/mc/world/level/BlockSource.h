#pragma once
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/block/Block.h"
#include "mc/world/level/chunk/ChunkSource.h"

class BlockActor;
class Dimension;
class Level;
class ActorBlockSyncMessage;
class BlockChangeContext;

class BlockSource {
public:
    BlockSource() = default;
    BlockSource(Level& level, Dimension& dim, ChunkSource& chunkSource, bool a=false, bool b=true, bool c=false, bool d=false) {
        (void)level; (void)dim; (void)chunkSource; (void)a; (void)b; (void)c; (void)d;
    }
    ~BlockSource() = default;

    Block const& getBlock(BlockPos const& pos) const {
        auto vtable = *reinterpret_cast<uintptr_t const* const*>(this);
        using Fn = Block const& (*)(BlockSource const*, BlockPos const&);
        return reinterpret_cast<Fn>(vtable[0])(this, pos);
    }
    Block const& getBlock(int32_t x, int32_t y, int32_t z) const {
        return getBlock(BlockPos{x, y, z});
    }
    Block const& getExtraBlock(BlockPos const& pos) const {
        auto vtable = *reinterpret_cast<uintptr_t const* const*>(this);
        using Fn = Block const& (*)(BlockSource const*, BlockPos const&);
        return reinterpret_cast<Fn>(vtable[1])(this, pos);
    }
    Block const& getLiquidBlock(BlockPos const& pos) const {
        auto vtable = *reinterpret_cast<uintptr_t const* const*>(this);
        using Fn = Block const& (*)(BlockSource const*, BlockPos const&);
        return reinterpret_cast<Fn>(vtable[2])(this, pos);
    }
    BlockActor* getBlockEntity(BlockPos const& pos) const { (void)pos; return nullptr; }
    bool areChunksFullyLoaded(BlockPos const& min, BlockPos const& max) const { (void)min; (void)max; return true; }
    bool hasChunksAt(BlockPos const& min, BlockPos const& max) const { (void)min; (void)max; return true; }
    ChunkSource& getChunkSource() const { static ChunkSource sDummy; return sDummy; }
    void addListener(class BlockSourceListener& listener) { (void)listener; }
    void removeListener(class BlockSourceListener& listener) { (void)listener; }
    Block const& getBlock(BlockPos const& pos, unsigned int layer) const {
        if (layer == 1) return getExtraBlock(pos);
        return getBlock(pos);
    }

    bool setBlock(BlockPos const& pos, Block const& block, int updateFlags = 3, void* actor = nullptr);
    bool setBlock(
        BlockPos const& position,
        Block const& block,
        int updateFlags,
        ActorBlockSyncMessage const* syncMsg,
        BlockChangeContext const& changeSourceContext
    );
    bool setBlock(
        BlockPos const& position,
        Block const& block,
        int updateFlags,
        std::shared_ptr<BlockActor> blockEntity,
        ActorBlockSyncMessage const* syncMsg,
        BlockChangeContext const& changeSourceContext
    );
};
