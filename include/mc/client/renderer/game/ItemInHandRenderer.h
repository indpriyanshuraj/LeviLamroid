#pragma once
#include "mc/deps/minecraft_renderer/renderer/MaterialPtr.h"

class ItemInHandRenderer {
public:
    struct MaterialWrapper {
        mce::MaterialPtr mPtr;
        mce::MaterialPtr const& get() const { return mPtr; }
    };

    MaterialWrapper mMatOpaqueBlockColor;
    MaterialWrapper mMatOpaqueBlock;
    MaterialWrapper mMatAlphaColoredBlock;
    MaterialWrapper mMatAlphaBlock;
    MaterialWrapper mMatAlphaOneSidedColoredBlock;
    MaterialWrapper mMatAlphaOneSidedBlock;
    MaterialWrapper mMatBlendBlock;
    MaterialWrapper mMatBlendBlockNoColor;
    MaterialWrapper mMatItemInHand;
    bool mIsDeferredEnabled{false};
};
