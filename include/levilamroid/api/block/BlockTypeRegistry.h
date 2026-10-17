#pragma once
#include "ll/api/Macros.h"
#include "levilamroid/api/block/Block.h"
#include "levilamroid/api/block/BlockLegacy.h"
#include "levilamroid/api/block/BlockGroup.h"
#include "levilamroid/api/block/CustomBlockDefinition.h"
#include "levilamroid/api/math/HashedString.h"
#include <string>
#include <vector>

namespace levilamroid::block {

LL_API bool registerCustomBlock(const CustomBlockDefinition& def);
LL_API std::vector<CustomBlockDefinition> getRegisteredCustomBlocks();

} // namespace levilamroid::block

namespace ll::block {
    using ::levilamroid::block::registerCustomBlock;
    using ::levilamroid::block::getRegisteredCustomBlocks;
}

extern "C" {
LL_API bool ll_block_register(
    const char* identifier,
    const char* displayName,
    const char* textureName,
    int category,
    const char* creativeGroup,
    float destroyTime,
    float explosionResistance,
    uint8_t lightEmission
);
}
