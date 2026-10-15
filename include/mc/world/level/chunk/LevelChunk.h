#pragma once
#include <cstdint>
#include "mc/world/level/BlockPos.h"

class Dimension;

class LevelChunk {
public:
    struct ChunkPos2D {
        int32_t x{0};
        int32_t z{0};
    };

    struct PositionWrapper {
        ChunkPos2D mPos{};
        [[nodiscard]] ChunkPos2D const& get() const noexcept { return mPos; }
        [[nodiscard]] ChunkPos2D& get() noexcept { return mPos; }
    };

    PositionWrapper mPosition{};
    int32_t         mChunkX{0};
    int32_t         mChunkZ{0};

    LevelChunk() = default;
    LevelChunk(int32_t cx, int32_t cz)
        : mPosition{{cx, cz}}, mChunkX(cx), mChunkZ(cz) {}
    virtual ~LevelChunk() = default;

    [[nodiscard]] int32_t getX() const noexcept { return mChunkX; }
    [[nodiscard]] int32_t getZ() const noexcept { return mChunkZ; }
    [[nodiscard]] BlockPos getMin() const noexcept { return {mChunkX * 16, -64, mChunkZ * 16}; }
    [[nodiscard]] BlockPos getMax() const noexcept { return {mChunkX * 16 + 15, 319, mChunkZ * 16 + 15}; }
};
