#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include "mc/world/level/block/Block.h"

class StructureBlockPalette {
public:
    std::vector<Block const*> mBlocks{};

    StructureBlockPalette() = default;
    virtual ~StructureBlockPalette() = default;

    [[nodiscard]] size_t getSize() const noexcept { return mBlocks.size(); }
    [[nodiscard]] bool empty() const noexcept { return mBlocks.empty(); }

    [[nodiscard]] Block const* tryGetBlock(uint64_t index, void* unknownRegistry = nullptr) const noexcept {
        (void)unknownRegistry;
        if (index < mBlocks.size()) {
            return mBlocks[index];
        }
        return nullptr;
    }

    [[nodiscard]] Block const* getBlock(size_t index) const noexcept {
        if (index < mBlocks.size()) return mBlocks[index];
        return nullptr;
    }

    void addBlock(Block const* block) {
        if (block) mBlocks.push_back(block);
    }
};
