#pragma once
#include <cstdint>
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockPos.h"

class Block;
class BlockSource;

enum class TintMethod : int32_t {
    None = 0,
    DefaultFoliage = 1,
    BirchFoliage = 2,
    EvergreenFoliage = 3,
    DryFoliage = 4,
    Water = 5,
    Grass = 6
};

class BiomeColorSampling {
public:
    class TessellationPolicy {
    public:
        virtual ~TessellationPolicy() = default;

        virtual mce::Color get(
            Block const& block,
            BlockSource const& region,
            BlockPos const& pos,
            void* cache = nullptr
        ) const {
            (void)block;
            (void)region;
            (void)pos;
            (void)cache;
            return mce::Color{1.0f, 1.0f, 1.0f, 1.0f};
        }
    };

    static TessellationPolicy const& getTessellationPolicy(TintMethod method) {
        static TessellationPolicy const sDefaultPolicy{};
        (void)method;
        return sDefaultPolicy;
    }
};
