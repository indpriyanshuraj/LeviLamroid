#pragma once
#include <cstdint>

enum class BlockRenderLayer : int32_t {
    RenderlayerBlend = 0,
    RenderlayerBlendToOpaque = 1,
    RenderlayerOpaque = 2,
    RenderlayerSeasonsOpaque = 3,
    RenderlayerShiftOpaqueInternalOnly = 4,
    RenderlayerAlphatest = 5,
    RenderlayerAlphatestSingleSide = 6,
    RenderlayerAlphatestSingleSideToOpaque = 7,
    RenderlayerShiftAlphatestSingleSideInternalOnly = 8,
    RenderlayerShiftAlphatestSingleSideToOpaqueInternalOnly = 9,
    RenderlayerAlpha = 10,
    RenderlayerDoubleSided = 11,
    RenderlayerSeasonsDoubleSided = 12,
    RenderlayerWater = 13,
    RenderlayerMax = 14
};
