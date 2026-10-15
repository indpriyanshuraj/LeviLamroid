#pragma once
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/chunk/ChunkSourceViewGenerateMode.h"

class ChunkViewSource : public ChunkSource {
public:
    ChunkViewSource() = default;
    ~ChunkViewSource() override = default;

    virtual void move(
        BlockPos const& min,
        BlockPos const& max,
        bool b,
        ChunkSourceViewGenerateMode mode,
        std::function<void(gsl::span<std::shared_ptr<LevelChunk>>)> cb,
        void* unk = nullptr
    ) {
        (void)min; (void)max; (void)b; (void)mode; (void)cb; (void)unk;
    }
};
