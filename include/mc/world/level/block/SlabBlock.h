#pragma once
#include "mc/world/level/block/BlockType.h"

class SlabBlock : public BlockType {
public:
    bool mIsDouble{false};

    SlabBlock() {
        mIsSlab = true;
    }

    explicit SlabBlock(bool isDouble) : mIsDouble(isDouble) {
        mIsSlab = true;
    }

    ~SlabBlock() override = default;
};
