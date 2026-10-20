#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "levilamroid/api/block/BlockPos.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <mutex>
#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <cctype>
#include <cstring>
#include <cstdio>

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
    logger.info("§6[LeviLamroid] §aRegistered custom block definition: §b", def.identifier,
                " §7(Texture: ", def.textureName, 
                ", Category: ", def.category,
                ", Group: ", def.creativeGroup, ")");

    // Every block has an associated BlockItem for inventory & creative tabs
    item::CustomItemDefinition itemDef;
    itemDef.identifier = def.identifier;
    itemDef.isBlock = true;
    itemDef.baseBlock = def.baseBlock;
    itemDef.id = static_cast<short>(20600 + sCustomBlocks.size());
    itemDef.descriptionId = "tile." + def.identifier;
    // Blocks render 3D cube mesh from atlas.terrain, with fallback 2D icon from item_texture
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

const CustomBlockDefinition* getCustomBlock(std::string_view identifier) {
    std::lock_guard<std::mutex> lock(sBlockMutex);
    for (const auto& b : sCustomBlocks) {
        if (b.identifier == identifier) {
            return &b;
        }
        auto colon = b.identifier.find(':');
        if (colon != std::string_view::npos && b.identifier.substr(colon + 1) == identifier) {
            return &b;
        }
        auto qcolon = identifier.find(':');
        if (qcolon != std::string_view::npos && identifier.substr(qcolon + 1) == b.identifier) {
            return &b;
        }
    }
    return nullptr;
}

bool isCustomBlock(std::string_view identifier) {
    return getCustomBlock(identifier) != nullptr;
}

size_t getCustomBlockCount() {
    std::lock_guard<std::mutex> lock(sBlockMutex);
    return sCustomBlocks.size();
}

BlockRegistry& BlockRegistry::getInstance() {
    static BlockRegistry instance;
    return instance;
}

bool BlockRegistry::registerBlock(const CustomBlockDefinition& def) {
    return registerCustomBlock(def);
}

const CustomBlockDefinition* BlockRegistry::getBlock(std::string_view identifier) const {
    return getCustomBlock(identifier);
}

std::vector<CustomBlockDefinition> BlockRegistry::getAllBlocks() const {
    return getRegisteredCustomBlocks();
}

bool BlockRegistry::hasBlock(std::string_view identifier) const {
    return isCustomBlock(identifier);
}

size_t BlockRegistry::getCount() const {
    return getCustomBlockCount();
}

void* BlockRegistry::getPermutation(std::string_view identifier) const {
    return getCustomBlockPermutation(identifier);
}

void* BlockRegistry::getLegacy(std::string_view identifier) const {
    return getCustomBlockLegacy(identifier);
}

[[maybe_unused]] static uintptr_t getMcBase() {
    static uintptr_t sBase = 0;
    if (sBase != 0) return sBase;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libminecraftpe.so") && (strstr(line, "r-xp") || strstr(line, "r--p"))) {
            uintptr_t start = 0;
            if (sscanf(line, "%lx-", &start) == 1) {
                sBase = start;
                break;
            }
        }
    }
    fclose(fp);
    return sBase;
}

extern "C" void* ll_service_get_local_player();

void* getCustomBlockPermutation(std::string_view identifier) {
    auto& items = ::levilamroid::item::getAllCustomItems();
    for (auto& item : items) {
        if (item.identifier == identifier && item.isBlock) {
            return ::levilamroid::item::getBlockPermutationForDef(item);
        }
    }
    return nullptr;
}

void* getCustomBlockLegacy(std::string_view identifier) {
    auto& items = ::levilamroid::item::getAllCustomItems();
    for (auto& item : items) {
        if (item.identifier == identifier && item.isBlock && item.nativeItemPtr) {
            return *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(item.nativeItemPtr) + 312);
        }
    }
    return nullptr;
}

// ── Phase 2: Real Block Registration via Native Engine APIs ──

static void* sOrigBlockTypeRegistryInit = nullptr;
static void* sEngineBlockTypeRegistry = nullptr;

// Provided by CoreHooks
extern "C" void* ll_service_get_level();

// Hooked into right after vanilla BlockTypeRegistry::init() runs.
static void hooked_blockTypeRegistryInit(void* self) {
    // Always call original first so vanilla blocks are all registered
    if (sOrigBlockTypeRegistryInit) {
        reinterpret_cast<void(*)(void*)>(sOrigBlockTypeRegistryInit)(self);
    }
    sEngineBlockTypeRegistry = self;
    ::levilamroid::item::refreshCustomBlockBindings();
}

bool installBlockHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = getMcBase();

    if (mcBase && off.blockTypeRegistryInit) {
        using HookFn = void(*)(void*);
        bool hooked = ll::memory::hookRva(off.blockTypeRegistryInit,
            reinterpret_cast<void*>(static_cast<HookFn>(&hooked_blockTypeRegistryInit)),
            &sOrigBlockTypeRegistryInit);
        if (hooked) {
            logger.info("[BlockHooks] Hooked BlockTypeRegistry::init [RVA: 0x", std::hex, off.blockTypeRegistryInit, "]");
        } else {
            logger.warn("[BlockHooks] Failed to hook blockTypeRegistryInit — blocks will fall back to stone");
        }
    } else {
        logger.warn("[BlockHooks] blockTypeRegistryInit RVA not set — Phase 2 blocks inactive");
    }

    logger.info("[BlockHooks] Native block subsystem ready.");
    return true;
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
    if (strstr(identifier, "crafting_table") || strstr(identifier, "table")) {
        def.baseBlock = "minecraft:crafting_table";
    } else {
        def.baseBlock = "minecraft:stone";
    }
    def.category = (category > 0) ? category : 1;
    def.creativeGroup = (creativeGroup && creativeGroup[0] != '\0') ? creativeGroup : std::string(::levilamroid::block::groups::Stone);
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;
    return ::levilamroid::block::registerCustomBlock(def);
}

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
) {
    if (!identifier) return false;
    ::levilamroid::block::CustomBlockDefinition def;
    def.identifier = identifier;
    def.displayName = displayName ? displayName : identifier;
    def.textureName = textureName ? textureName : identifier;
    def.baseBlock = (baseBlock && baseBlock[0] != '\0') ? baseBlock : "minecraft:info_update";
    def.category = (category > 0) ? category : 1;
    def.creativeGroup = (creativeGroup && creativeGroup[0] != '\0') ? creativeGroup : std::string(::levilamroid::block::groups::Stone);
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;
    return ::levilamroid::block::registerCustomBlock(def);
}

LL_API bool ll_block_is_registered(const char* identifier) {
    if (!identifier) return false;
    return ::levilamroid::block::isCustomBlock(identifier);
}

LL_API size_t ll_block_get_count() {
    return ::levilamroid::block::getCustomBlockCount();
}

LL_API void* ll_block_get_permutation(const char* identifier) {
    if (!identifier) return nullptr;
    return ::levilamroid::block::getCustomBlockPermutation(identifier);
}

LL_API void* ll_block_get_legacy(const char* identifier) {
    if (!identifier) return nullptr;
    return ::levilamroid::block::getCustomBlockLegacy(identifier);
}

LL_API const void* ll_block_get_direct_access_blocks() {
    uintptr_t mcBase = ::levilamroid::block::getMcBase();
    if (!mcBase) return nullptr;
    using Fn = const void* (*)(void*);
    auto fn = reinterpret_cast<Fn>(mcBase + 0x103E7828);
    if (::levilamroid::block::sEngineBlockTypeRegistry) {
        return fn(::levilamroid::block::sEngineBlockTypeRegistry);
    }
    return nullptr;
}

} // extern "C"
