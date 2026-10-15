#pragma once
#include "mc/world/level/block/Block.h"
#include "mc/world/level/block/BlockRenderLayer.h"
#include "mc/client/renderer/texture/TextureUVCoordinateSet.h"

class BlockSource;
struct BlockPos;

class BlockGraphics {
public:
    static BlockGraphics const* getForBlock(Block const& block) { (void)block; return nullptr; }
    BlockRenderLayer getRenderLayer(BlockSource const& region, BlockPos const& pos) const {
        (void)region; (void)pos;
        return BlockRenderLayer::RenderlayerAlpha;
    }
    TextureUVCoordinateSet const& getTexture(int face, int data) const {
        (void)face; (void)data;
        static TextureUVCoordinateSet sSet{};
        return sSet;
    }
};
