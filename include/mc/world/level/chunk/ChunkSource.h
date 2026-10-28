#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include <functional>
#include "mc/world/level/chunk/LevelChunk.h"
#include "mc/_HeaderOutputPredefine.h"

class ChunkSource {
public:
    virtual ~ChunkSource() = default;

    virtual LevelChunk* getExistingChunk(int32_t x, int32_t z) { (void)x; (void)z; return nullptr; }
    virtual std::shared_ptr<LevelChunk> getAvailableChunk(int32_t x, int32_t z) { (void)x; (void)z; return nullptr; }
};
