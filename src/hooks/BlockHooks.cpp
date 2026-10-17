#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <mutex>
#include <algorithm>

namespace levilamroid::block {

static std::vector<CustomBlockDefinition> sCustomBlocks;
static std::mutex sBlockMutex;

bool registerCustomBlock(const CustomBlockDefinition& def) {
    std::lock_guard<std::mutex> lock(sBlockMutex);
    for (auto& b : sCustomBlocks) {
        if (b.identifier == def.identifier) {
            b = def;
            return true;
        }
    }
    sCustomBlocks.push_back(def);

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[LeviLamroid] §aQueued custom block: §b", def.identifier,
                " §7(Texture: ", def.textureName, 
                ", Category: ", def.category,
                ", Group: ", def.creativeGroup, ")");

    // Every block has an associated BlockItem for inventory & creative tabs
    item::CustomItemDefinition itemDef;
    itemDef.identifier = def.identifier;
    itemDef.isBlock = true;
    // Derive numeric ID deterministically or dynamically (10600+)
    itemDef.id = static_cast<short>(10600 + sCustomBlocks.size());
    itemDef.descriptionId = "tile." + def.identifier + ".name";
    itemDef.iconName = def.textureName;
    itemDef.iconFrame = 0;
    itemDef.category = def.category;
    itemDef.creativeGroup = def.creativeGroup;
    itemDef.allowOffhand = true;
    itemDef.maxStackSize = 64; // Blocks always stack to 64
    item::registerCustomItem(itemDef);

    return true;
}

std::vector<CustomBlockDefinition> getRegisteredCustomBlocks() {
    std::lock_guard<std::mutex> lock(sBlockMutex);
    return sCustomBlocks;
}

} // namespace levilamroid::block

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
) {
    if (!identifier) return false;
    ::levilamroid::block::CustomBlockDefinition def;
    def.identifier = identifier;
    def.displayName = displayName ? displayName : identifier;
    def.textureName = textureName ? textureName : identifier;
    def.category = (category > 0) ? category : 1;
    def.creativeGroup = creativeGroup ? creativeGroup : std::string(::levilamroid::block::groups::Stone);
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;
    return ::levilamroid::block::registerCustomBlock(def);
}

} // extern "C"
