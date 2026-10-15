#pragma once
#include <cstdint>
#include <string>

class ChunkSource;
class LevelChunk;

class LevelListener {
public:
    virtual ~LevelListener() = default;

    virtual void onSubChunkLoaded(
        ChunkSource& source,
        LevelChunk& chunk,
        short absoluteSubChunkIndex,
        bool generateMode
    ) {
        (void)source; (void)chunk; (void)absoluteSubChunkIndex; (void)generateMode;
    }

    virtual void onLevelDestruction(std::string const& levelId) { (void)levelId; }
};
