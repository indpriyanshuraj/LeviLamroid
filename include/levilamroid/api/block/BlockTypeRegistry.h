#pragma once
#include "ll/api/Macros.h"
#include "levilamroid/api/block/Block.h"
#include "levilamroid/api/block/BlockLegacy.h"
#include "levilamroid/api/block/BlockGroup.h"
#include "levilamroid/api/block/CustomBlockDefinition.h"
#include "levilamroid/api/math/HashedString.h"
#include <string>
#include <string_view>
#include <vector>

namespace levilamroid::block {

LL_API bool registerCustomBlock(const CustomBlockDefinition& def);
LL_API std::vector<CustomBlockDefinition> getRegisteredCustomBlocks();
LL_API const CustomBlockDefinition* getCustomBlock(std::string_view identifier);
LL_API bool isCustomBlock(std::string_view identifier);
LL_API size_t getCustomBlockCount();
LL_API void* getCustomBlockPermutation(std::string_view identifier);
LL_API void* getCustomBlockLegacy(std::string_view identifier);

class LL_API BlockRegistry {
public:
    static BlockRegistry& getInstance();

    bool registerBlock(const CustomBlockDefinition& def);
    const CustomBlockDefinition* getBlock(std::string_view identifier) const;
    std::vector<CustomBlockDefinition> getAllBlocks() const;
    bool hasBlock(std::string_view identifier) const;
    size_t getCount() const;
    void* getPermutation(std::string_view identifier) const;
    void* getLegacy(std::string_view identifier) const;
};

} // namespace levilamroid::block

namespace ll::block {
    using ::levilamroid::block::registerCustomBlock;
    using ::levilamroid::block::getRegisteredCustomBlocks;
    using ::levilamroid::block::getCustomBlock;
    using ::levilamroid::block::isCustomBlock;
    using ::levilamroid::block::getCustomBlockCount;
    using ::levilamroid::block::getCustomBlockPermutation;
    using ::levilamroid::block::getCustomBlockLegacy;
    using ::levilamroid::block::BlockRegistry;
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

LL_API bool ll_block_register_with_base(
    const char* identifier,
    const char* displayName,
    const char* textureName,
    const char* baseBlock,
    int category,
    const char* creativeGroup,
    float destroyTime,
    float explosionResistance,
    uint8_t lightEmission
);

LL_API bool ll_block_is_registered(const char* identifier);
LL_API size_t ll_block_get_count();
LL_API void* ll_block_get_permutation(const char* identifier);
LL_API void* ll_block_get_legacy(const char* identifier);

}
