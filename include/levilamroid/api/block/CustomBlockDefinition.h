#pragma once
#include "levilamroid/api/item/CreativeItemCategory.h"
#include "levilamroid/api/block/BlockGroup.h"
#include <string>
#include <cstdint>

namespace levilamroid::block {

struct CustomBlockDefinition {
    std::string identifier;                             // e.g. "custom:ruby_block"
    std::string displayName;                            // e.g. "Ruby Block"
    std::string textureName;                            // e.g. "ruby_block" (in textures/blocks/)
    int category{static_cast<int>(item::CreativeItemCategory::Construction)};
    std::string creativeGroup{std::string(groups::Stone)};
    float destroyTime{1.5f};
    float explosionResistance{6.0f};
    uint8_t lightEmission{0};
    bool isSolid{true};
    float friction{0.6f};
    void* nativeBlockLegacyPtr{nullptr};
    void* nativeBlockPermutation{nullptr};
    uint16_t blockId{0};
    std::string baseBlock{""};
};

} // namespace levilamroid::block

namespace ll::block {
    using CustomBlockDefinition = ::levilamroid::block::CustomBlockDefinition;
}
