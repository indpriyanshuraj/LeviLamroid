#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "levilamroid/api/block/BlockPos.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/item/ItemStack.h"
#include "levilamroid/api/geometry/GeometryRegistry.h"
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

static inline bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t untagged = v & 0x00ffffffffffffffULL;
    if (untagged < 0x10000 || untagged > 0x00007fffffffffffULL) return false;
    if (untagged + size < untagged) return false;
    return true;
}

static std::vector<CustomBlockDefinition> sCustomBlocks;
static std::mutex sBlockMutex;

bool registerCustomBlock(const CustomBlockDefinition& def) {
    {
        std::lock_guard<std::mutex> lock(sBlockMutex);
        bool found = false;
        for (auto& b : sCustomBlocks) {
            if (b.identifier == def.identifier) {
                std::string prevGeo = b.geometry;
                std::string prevBase = b.baseBlock;
                void* prevLegacy = b.nativeBlockLegacyPtr;
                b = def;
                if (b.geometry.empty()) b.geometry = prevGeo;
                if ((b.baseBlock.empty() || b.baseBlock == "minecraft:stone") && !prevBase.empty()) {
                    b.baseBlock = prevBase;
                }
                if (!b.nativeBlockLegacyPtr) b.nativeBlockLegacyPtr = prevLegacy;
                found = true;
                break;
            }
        }
        if (!found) {
            sCustomBlocks.push_back(def);
        }
    }

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[LeviLamroid] §aRegistered custom block definition: §b", def.identifier,
                " §7(Texture: ", def.textureName, 
                ", Category: ", def.category,
                ", Group: ", def.creativeGroup, 
                ", Geo: ", def.geometry, ")");

    // Synchronize corresponding 3D BlockItem in ItemRegistry
    ::levilamroid::item::CustomItemDefinition itemDef;
    itemDef.identifier = def.identifier;
    const auto* existing = ::levilamroid::item::getCustomItem(def.identifier);
    if (existing && existing->id > 0) {
        itemDef = *existing;
    } else {
        static std::atomic<short> sAutoBlockId{10150};
        itemDef.id = sAutoBlockId.fetch_add(1);
    }
    itemDef.displayName = def.displayName;
    itemDef.descriptionId = std::string("tile.") + def.identifier + ".name";
    itemDef.iconName = ""; // Empty 2D icon forces 3D block rendering
    itemDef.category = def.category;
    itemDef.creativeGroup = def.creativeGroup;
    itemDef.allowOffhand = true;
    itemDef.maxStackSize = 64;
    itemDef.isBlock = true;
    itemDef.isHandEquipped = false;
    itemDef.isFood = false;
    itemDef.nutrition = 0;
    itemDef.useAnimation = 0;
    itemDef.baseBlock = def.baseBlock;
    itemDef.geometry = def.geometry;
    itemDef.nativeBlockLegacyPtr = def.nativeBlockLegacyPtr;
    ::levilamroid::item::registerCustomItem(itemDef);

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
    return ::ll::memory::getMinecraftBase();
}

extern "C" void* ll_service_get_local_player();

extern "C" LL_API const void* ll_block_get_direct_access_blocks();

void* getCustomBlockPermutation(std::string_view identifier) {
    auto& items = ::levilamroid::item::getAllCustomItems();
    for (auto& item : items) {
        if (item.identifier == identifier && item.isBlock) {
            void* perm = ::levilamroid::item::getBlockPermutationForDef(item);
            if (perm) return perm;
        }
    }

    // Resolve base block permutation from engine direct access blocks
    const void* dab = ll_block_get_direct_access_blocks();
    if (dab) {
        const void* const* blockArray = reinterpret_cast<const void* const*>(dab);
        int blockId = 1; // Default to stone (ID 1)
        std::string baseBlock = getFunctionalBlockForCustomItem(std::string(identifier));
        if (baseBlock == "minecraft:dirt") blockId = 3;
        else if (baseBlock == "minecraft:cobblestone") blockId = 4;
        else if (baseBlock == "minecraft:wooden_slab" || baseBlock == "minecraft:planks") blockId = 5;
        else if (baseBlock == "minecraft:chest") blockId = 54;
        else if (baseBlock == "minecraft:crafting_table") blockId = 58;
        else if (baseBlock == "minecraft:furnace") blockId = 61;
        else if (baseBlock == "minecraft:white_carpet") blockId = 171;
        else if (baseBlock == "minecraft:flower_pot") blockId = 140;
        else if (baseBlock == "minecraft:wheat") blockId = 59;
        else if (baseBlock == "minecraft:allium") blockId = 38;
        else if (baseBlock == "minecraft:chain") blockId = 549;
        else if (identifier == "minecraft:air") blockId = 0;

        if (isPointerReadable(&blockArray[blockId], sizeof(void*))) {
            const void* p = blockArray[blockId];
            if (p && isPointerReadable(p, 32)) {
                return const_cast<void*>(p);
            }
        }
        if (isPointerReadable(&blockArray[1], sizeof(void*))) {
            const void* p = blockArray[1];
            if (p && isPointerReadable(p, 32)) {
                return const_cast<void*>(p);
            }
        }
    }
    return nullptr;
}

void* getCustomBlockLegacy(std::string_view identifier) {
    auto& items = ::levilamroid::item::getAllCustomItems();
    for (auto& item : items) {
        if (item.identifier == identifier && item.isBlock && item.nativeItemPtr) {
            auto* blkCtrl = *reinterpret_cast<item::BedrockBlockControlBlock**>(reinterpret_cast<uint8_t*>(item.nativeItemPtr) + 312);
            return blkCtrl ? blkCtrl->mBlockLegacy : nullptr;
        }
    }
    return nullptr;
}

// ── Phase 2: Real Block Registration via Native Engine APIs ──

static void* sOrigBlockTypeRegistryInit = nullptr;
static void* sEngineBlockTypeRegistry = nullptr;

// Provided by CoreHooks
extern "C" void* ll_service_get_level();
extern "C" void* ll_service_get_block_source();
extern "C" bool ll_player_get_held_item_identifier(void* player, char* outIdent, size_t maxLen);
extern "C" bool ll_command_dispatch_client(const char* command);
extern "C" void ll_ui_send_toast(const char* title, const char* msg, const char* icon, int durationMs);
extern "C" bool ll_player_consume_held_item(void* player, int count);
extern "C" bool ll_block_source_set_block(void* blockSource, int x, int y, int z, void* blockPermutation, int updateFlags);
extern "C" bool ll_handle_food_consumption(void* player, void* itemStack, const ::levilamroid::item::CustomItemDefinition* def);

LL_API bool isPlaceableItem(const std::string& id) {
    if (id.find("knife") != std::string::npos ||
        id.find("dough") != std::string::npos ||
        id.find("crust") != std::string::npos ||
        id.find("pasta") != std::string::npos ||
        id.find("slice") != std::string::npos ||
        id.find("straw") != std::string::npos ||
        id.find("canvas") != std::string::npos) {
        return false;
    }
    if (isCustomBlock(id)) return true;
    const auto* def = ::levilamroid::item::getCustomItem(id);
    if (def && def->isBlock) return true;
    return false;
}

LL_API std::string getFunctionalBlockForCustomItem(const std::string& id) {
    if (auto blockDef = getCustomBlock(id)) {
        if (!blockDef->baseBlock.empty() && blockDef->baseBlock != "minecraft:stone") return blockDef->baseBlock;
    }
    if (auto itemDef = ::levilamroid::item::getCustomItem(id)) {
        if (!itemDef->baseBlock.empty() && itemDef->baseBlock != "minecraft:stone") return itemDef->baseBlock;
    }
    if (id.find("wild_") != std::string::npos || id.find("flower") != std::string::npos ||
        id.find("shrub") != std::string::npos || id.find("allium") != std::string::npos ||
        id.find("poppy") != std::string::npos) return "minecraft:allium";
    if (id.find("seed") != std::string::npos || id.find("rice") != std::string::npos ||
        id.find("cabbage") != std::string::npos || id.find("tomato") != std::string::npos ||
        id.find("onion") != std::string::npos || id.find("crop") != std::string::npos) return "minecraft:wheat";
    if (id.find("cutting_board") != std::string::npos || id.find("board") != std::string::npos || id.find("table") != std::string::npos) return "minecraft:crafting_table";
    if (id.find("stove") != std::string::npos || id.find("cooker") != std::string::npos || id.find("furnace") != std::string::npos) return "minecraft:furnace";
    if (id.find("pot") != std::string::npos || id.find("skillet") != std::string::npos) return "minecraft:flower_pot";
    if (id.find("pie") != std::string::npos || id.find("cake") != std::string::npos || id.find("feast") != std::string::npos) return "minecraft:cake";
    if (id.find("cabinet") != std::string::npos || id.find("crate") != std::string::npos || id.find("basket") != std::string::npos) return "minecraft:barrel";
    if (id.find("rug") != std::string::npos || id.find("carpet") != std::string::npos || id.find("canvas") != std::string::npos) return "minecraft:white_carpet";
    if (id.find("tatami") != std::string::npos || id.find("mat") != std::string::npos || id.find("slab") != std::string::npos) return "minecraft:wooden_slab";
    if (id.find("soil") != std::string::npos || id.find("farmland") != std::string::npos) return "minecraft:farmland";
    if (id.find("compost") != std::string::npos || id.find("dirt") != std::string::npos) return "minecraft:dirt";
    if (id.find("rope") != std::string::npos || id.find("net") != std::string::npos) return "minecraft:chain";
    return "minecraft:stone";
}

LL_API const char* getPlacementSoundForBlock(const std::string& id) {
    if (auto blockDef = getCustomBlock(id)) {
        if (!blockDef->creativeGroup.empty()) {
            if (blockDef->creativeGroup.find("wood") != std::string::npos) return "dig.wood";
            if (blockDef->creativeGroup.find("cloth") != std::string::npos || blockDef->creativeGroup.find("wool") != std::string::npos) return "dig.cloth";
            if (blockDef->creativeGroup.find("dirt") != std::string::npos || blockDef->creativeGroup.find("gravel") != std::string::npos) return "dig.gravel";
            if (blockDef->creativeGroup.find("plant") != std::string::npos || blockDef->creativeGroup.find("nature") != std::string::npos) return "dig.grass";
        }
    }
    if (id.find("wood") != std::string::npos || id.find("plank") != std::string::npos || id.find("board") != std::string::npos) return "dig.wood";
    if (id.find("cloth") != std::string::npos || id.find("rug") != std::string::npos || id.find("wool") != std::string::npos) return "dig.cloth";
    if (id.find("soil") != std::string::npos || id.find("dirt") != std::string::npos || id.find("gravel") != std::string::npos) return "dig.gravel";
    if (id.find("grass") != std::string::npos || id.find("plant") != std::string::npos || id.find("crop") != std::string::npos) return "dig.grass";
    return "dig.stone";
}

void* getEngineBlockTypeRegistry() {
    if (sEngineBlockTypeRegistry) return sEngineBlockTypeRegistry;
    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (mcBase && off.blockTypeRegistryGetInstance) {
        using GetRegistryFn = void* (*)();
        auto getRegistry = reinterpret_cast<GetRegistryFn>(mcBase + off.blockTypeRegistryGetInstance);
        sEngineBlockTypeRegistry = getRegistry();
    }
    return sEngineBlockTypeRegistry;
}

// ── Block Placement Hook (GameMode::buildBlock) ──────────────────────────
struct BlockPlaceListener {
    LLBlockPlaceCallback cb;
    void* udata;
};
static std::vector<BlockPlaceListener> sBlockPlaceListeners;
static std::mutex sBlockPlaceMutex;

static bool dispatchBlockPlaceListeners(void* player, int x, int y, int z, uint8_t face, bool isTouch, const char* blockIdent) {
    std::vector<BlockPlaceListener> copies;
    {
        std::lock_guard<std::mutex> lock(sBlockPlaceMutex);
        copies = sBlockPlaceListeners;
    }
    for (const auto& l : copies) {
        if (l.cb && l.cb(player, x, y, z, face, isTouch, blockIdent, l.udata)) return true;
    }
    return false;
}

static void* sOrigGameModeBuildBlock = nullptr;

static bool hooked_gameModeBuildBlock(void* self, const void* pos, uint8_t face, bool isTouch, bool flag) {
    void* player = self ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(self) + 0x8) : nullptr;
    if (!player) player = ll_service_get_local_player();

    if (pos) {
        const int* p = reinterpret_cast<const int*>(pos);
        int x = p[0], y = p[1], z = p[2];

        char heldBuf[128] = {0};
        const char* heldIdent = "";
        if (ll_player_get_held_item_identifier(player, heldBuf, sizeof(heldBuf)) && heldBuf[0]) {
            heldIdent = heldBuf;
        }

        // If a listener returns true, it consumed/cancelled the placement event
        if (dispatchBlockPlaceListeners(player, x, y, z, face, isTouch, heldIdent)) {
            return false;
        }

        bool placed = false;
        if (sOrigGameModeBuildBlock) {
            using BuildFn = bool (*)(void*, const void*, uint8_t, bool, bool);
            placed = reinterpret_cast<BuildFn>(sOrigGameModeBuildBlock)(self, pos, face, isTouch, flag);
        }

        std::string heldStr(heldIdent);
        const auto* itemDef = ::levilamroid::item::getCustomItem(heldStr);

        // Fallback for custom blocks / placeable items / seeds / food when vanilla buildBlock doesn't handle them
        if (!placed && !heldStr.empty()) {
            // 1. Seeds planting
            if (heldStr.find("seeds") != std::string::npos && face == 1) {
                int px = x, py = y + 1, pz = z;
                void* bs = ll_service_get_block_source();
                void* perm = getCustomBlockPermutation(heldStr);
                if (bs && perm) {
                    placed = ll_block_source_set_block(bs, px, py, pz, perm, 3);
                }
                if (!placed) {
                    std::string cropBlock = (itemDef && !itemDef->baseBlock.empty()) ? itemDef->baseBlock : "minecraft:wheat";
                    char cmd[128];
                    std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d %s [] replace", px, py, pz, cropBlock.c_str());
                    placed = ll_command_dispatch_client(cmd);
                }
                if (placed) {
                    ll_command_dispatch_client("/playsound dig.grass @a ~ ~ ~ 0.8 1.4");
                    trackPlacedCustomBlock(px, py, pz, heldStr);
                    ll_player_consume_held_item(player, 1);
                    ll_ui_send_toast(heldStr.c_str(), "Crops Planted!", heldStr.c_str(), 1800);
                    return true;
                }
            }

            // 2. Custom blocks & placeable items
            if (isCustomBlock(heldStr) || isPlaceableItem(heldStr)) {
                int tx = x, ty = y, tz = z;
                switch (face) {
                    case 0: ty--; break;
                    case 1: ty++; break;
                    case 2: tz--; break;
                    case 3: tz++; break;
                    case 4: tx--; break;
                    case 5: tx++; break;
                    default: ty++; break;
                }

                void* bs = ll_service_get_block_source();
                void* perm = getCustomBlockPermutation(heldStr);
                if (bs && perm) {
                    placed = ll_block_source_set_block(bs, tx, ty, tz, perm, 3);
                }
                if (!placed) {
                    std::string placeBlock = getFunctionalBlockForCustomItem(heldStr);
                    char cmd[128];
                    std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d %s [] replace", tx, ty, tz, placeBlock.c_str());
                    placed = ll_command_dispatch_client(cmd);
                }
                if (placed) {
                    const char* sound = getPlacementSoundForBlock(heldStr);
                    if (sound && sound[0]) {
                        char scmd[128];
                        std::snprintf(scmd, sizeof(scmd), "/playsound %s @a ~ ~ ~ 0.8 1.0", sound);
                        ll_command_dispatch_client(scmd);
                    }
                    trackPlacedCustomBlock(tx, ty, tz, heldStr);
                    ll_player_consume_held_item(player, 1);
                    std::string shortName = heldStr;
                    auto colon = shortName.find(':');
                    if (colon != std::string::npos) shortName = shortName.substr(colon + 1);
                    std::string toast = "Placed " + shortName + "!";
                    ll_ui_send_toast(heldStr.c_str(), toast.c_str(), shortName.c_str(), 1800);
                    return true;
                }
            }
        }

        // Only track placed custom block if placement actually succeeded in the engine
        if (placed && isCustomBlock(heldIdent)) {
            int tx = x, ty = y, tz = z;
            switch (face) {
                case 0: ty--; break;
                case 1: ty++; break;
                case 2: tz--; break;
                case 3: tz++; break;
                case 4: tx--; break;
                case 5: tx++; break;
                default: ty++; break;
            }
            trackPlacedCustomBlock(tx, ty, tz, heldIdent);
        }
        return placed;
    }

    if (sOrigGameModeBuildBlock) {
        using BuildFn = bool (*)(void*, const void*, uint8_t, bool, bool);
        return reinterpret_cast<BuildFn>(sOrigGameModeBuildBlock)(self, pos, face, isTouch, flag);
    }
    return false;
}

// ── Fake Block / Ghost Block Management (Levimatica / Schematic Preview) ─
struct FakeBlockRecord {
    int x, y, z;
    std::string ident;
    bool clientOnly;
};
static std::vector<FakeBlockRecord> sFakeBlocks;
static std::mutex sFakeBlockMutex;

bool fakeBlockSet(int x, int y, int z, const char* blockIdent, bool clientOnly) {
    if (!blockIdent) return false;
    void* bs = ll_service_get_block_source();
    void* perm = getCustomBlockPermutation(blockIdent);
    bool ok = false;
    if (bs && perm) {
        // Flag 4 = client-only / no-sync flag (Levimatica ghost block technique)
        ok = ll_block_source_set_block(bs, x, y, z, perm, clientOnly ? 4 : 3);
    }
    if (!ok) {
        char cmd[128];
        std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d %s", x, y, z, blockIdent);
        ok = ll_command_dispatch_client(cmd);
    }
    if (ok) {
        std::lock_guard<std::mutex> lk(sFakeBlockMutex);
        sFakeBlocks.push_back({x, y, z, blockIdent, clientOnly});
    }
    return ok;
}

bool fakeBlockClear(int x, int y, int z) {
    void* bs = ll_service_get_block_source();
    bool ok = false;
    if (bs) {
        void* airPerm = getCustomBlockPermutation("minecraft:air");
        if (airPerm) {
            ok = ll_block_source_set_block(bs, x, y, z, airPerm, 4);
        }
    }
    if (!ok) {
        char cmd[128];
        std::snprintf(cmd, sizeof(cmd), "/setblock %d %d %d air", x, y, z);
        ok = ll_command_dispatch_client(cmd);
    }
    {
        std::lock_guard<std::mutex> lk(sFakeBlockMutex);
        sFakeBlocks.erase(std::remove_if(sFakeBlocks.begin(), sFakeBlocks.end(),
            [x, y, z](const FakeBlockRecord& r) { return r.x == x && r.y == y && r.z == z; }),
            sFakeBlocks.end());
    }
    return ok;
}

void fakeBlockClearAll() {
    std::vector<FakeBlockRecord> copy;
    {
        std::lock_guard<std::mutex> lk(sFakeBlockMutex);
        copy = sFakeBlocks;
        sFakeBlocks.clear();
    }
    for (const auto& r : copy) {
        fakeBlockClear(r.x, r.y, r.z);
    }
}

// Hooked into right after vanilla BlockTypeRegistry::init() runs (if non-zero).
static void hooked_blockTypeRegistryInit() {
    if (sOrigBlockTypeRegistryInit) {
        reinterpret_cast<void(*)()>(sOrigBlockTypeRegistryInit)();
    }
    sEngineBlockTypeRegistry = getEngineBlockTypeRegistry();
    ::levilamroid::item::refreshCustomBlockBindings();
}

bool installBlockHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = getMcBase();

    if (mcBase && off.blockTypeRegistryInit) {
        using HookFn = void(*)();
        bool hooked = ll::memory::hookRva(off.blockTypeRegistryInit,
            reinterpret_cast<void*>(static_cast<HookFn>(&hooked_blockTypeRegistryInit)),
            &sOrigBlockTypeRegistryInit);
        if (hooked) {
            logger.info("[BlockHooks] Hooked BlockTypeRegistry::init [RVA: 0x", std::hex, off.blockTypeRegistryInit, "]");
        } else {
            logger.warn("[BlockHooks] Failed to hook blockTypeRegistryInit");
        }
    } else {
        logger.info("[BlockHooks] Native block bindings will resolve via blockTypeRegistryGetInstance.");
    }

    if (off.gameModeBuildBlock) {
        bool hooked = ll::memory::hookRva(off.gameModeBuildBlock,
            reinterpret_cast<void*>(&hooked_gameModeBuildBlock),
            &sOrigGameModeBuildBlock);
        if (hooked) {
            logger.info("§6[BlockHooks] §aHooked GameMode::buildBlock [RVA: 0x", std::hex, off.gameModeBuildBlock, "]");
        } else {
            logger.warn("[BlockHooks] Failed to hook GameMode::buildBlock");
        }
    }

    logger.info("[BlockHooks] Native block subsystem ready.");
    return true;
}

struct BlockPosKey {
    int x, y, z;
    bool operator==(const BlockPosKey& o) const { return x == o.x && y == o.y && z == o.z; }
};
struct BlockPosKeyHash {
    size_t operator()(const BlockPosKey& k) const {
        return (static_cast<size_t>(k.x) * 73856093) ^ 
               (static_cast<size_t>(k.y) * 19349663) ^ 
               (static_cast<size_t>(k.z) * 83492791);
    }
};

static std::unordered_map<BlockPosKey, std::string, BlockPosKeyHash> sPlacedCustomBlocks;
static std::mutex sPlacedBlockMutex;

void trackPlacedCustomBlock(int x, int y, int z, const std::string& identifier) {
    std::lock_guard<std::mutex> lock(sPlacedBlockMutex);
    sPlacedCustomBlocks[{x, y, z}] = identifier;
    ll::Logger::getInstance().info("§6[BlockHooks] §aTracked placed custom block at (", x, ", ", y, ", ", z, "): ", identifier);
}

bool getTrackedCustomBlock(int x, int y, int z, std::string* outIdentifier) {
    std::lock_guard<std::mutex> lock(sPlacedBlockMutex);
    auto it = sPlacedCustomBlocks.find({x, y, z});
    if (it != sPlacedCustomBlocks.end()) {
        if (outIdentifier) *outIdentifier = it->second;
        return true;
    }
    return false;
}

bool untrackPlacedCustomBlock(int x, int y, int z, std::string* outIdentifier) {
    std::lock_guard<std::mutex> lock(sPlacedBlockMutex);
    auto it = sPlacedCustomBlocks.find({x, y, z});
    if (it != sPlacedCustomBlocks.end()) {
        if (outIdentifier) *outIdentifier = it->second;
        sPlacedCustomBlocks.erase(it);
        return true;
    }
    return false;
}

} // namespace levilamroid::block

extern "C" {

LL_API bool ll_block_register_with_id(
    const char* identifier,
    short id,
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

    std::string identStr = identifier;
    std::string shortName = identStr;
    auto colon = shortName.find(':');
    if (colon != std::string::npos) shortName = shortName.substr(colon + 1);

    // Resolve custom 3D geometry if already ingested or registered in GeometryRegistry
    const auto* existingBlock = ::levilamroid::block::getCustomBlock(identifier);
    if (existingBlock && !existingBlock->geometry.empty()) {
        def.geometry = existingBlock->geometry;
    }
    if (def.geometry.empty()) {
        std::string candidateGeo = "geometry." + shortName;
        if (::levilamroid::geometry::hasGeometry(candidateGeo)) {
            def.geometry = candidateGeo;
        }
    }

    // Infer authentic vanilla base block archetype for proper collision and placement
    if (existingBlock && !existingBlock->baseBlock.empty() && existingBlock->baseBlock != "minecraft:stone") {
        def.baseBlock = existingBlock->baseBlock;
    } else if (identStr.find("table") != std::string::npos || identStr.find("crafting_table") != std::string::npos ||
               identStr.find("cutting_board") != std::string::npos || identStr.find("board") != std::string::npos) {
        def.baseBlock = "minecraft:crafting_table";
    } else if (identStr.find("rug") != std::string::npos || identStr.find("carpet") != std::string::npos) {
        def.baseBlock = "minecraft:white_carpet";
    } else if (identStr.find("tatami") != std::string::npos || identStr.find("mat") != std::string::npos || identStr.find("slab") != std::string::npos) {
        def.baseBlock = "minecraft:wooden_slab";
    } else if (identStr.find("pot") != std::string::npos) {
        def.baseBlock = "minecraft:flower_pot";
    } else if (identStr.find("stove") != std::string::npos || identStr.find("furnace") != std::string::npos || identStr.find("cooker") != std::string::npos) {
        def.baseBlock = "minecraft:furnace";
    } else if (identStr.find("cabinet") != std::string::npos || identStr.find("crate") != std::string::npos || identStr.find("basket") != std::string::npos || identStr.find("bag") != std::string::npos || identStr.find("bale") != std::string::npos) {
        def.baseBlock = "minecraft:barrel";
    } else if (identStr.find("soil") != std::string::npos || identStr.find("farmland") != std::string::npos) {
        def.baseBlock = "minecraft:farmland";
    } else if (identStr.find("compost") != std::string::npos || identStr.find("dirt") != std::string::npos) {
        def.baseBlock = "minecraft:dirt";
    } else if (identStr.find("rope") != std::string::npos || identStr.find("net") != std::string::npos) {
        def.baseBlock = "minecraft:chain";
    } else {
        def.baseBlock = "minecraft:stone";
    }

    def.category = (category > 0) ? category : 1;
    def.creativeGroup = (creativeGroup && creativeGroup[0] != '\0') ? creativeGroup : std::string(::levilamroid::block::groups::Stone);
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;

    // Register corresponding 3D BlockItem with unique numerical ID
    ::levilamroid::item::CustomItemDefinition itemDef;
    itemDef.identifier = identifier;
    itemDef.displayName = def.displayName;
    if (id <= 0) {
        static std::atomic<short> sAutoBlockId{10092};
        const auto* existing = ::levilamroid::item::getCustomItem(identifier);
        id = (existing && existing->id > 0) ? existing->id : sAutoBlockId.fetch_add(1);
    }
    itemDef.id = id;
    itemDef.descriptionId = std::string("tile.") + identifier + ".name";
    itemDef.iconName = ""; // No 2D icon override -> renders 3D block geometry
    itemDef.category = def.category;
    itemDef.creativeGroup = def.creativeGroup;
    itemDef.allowOffhand = true;
    itemDef.maxStackSize = 64;
    itemDef.isBlock = true;
    itemDef.isHandEquipped = false;
    itemDef.isFood = false;
    itemDef.nutrition = 0;
    itemDef.useAnimation = 0;
    itemDef.baseBlock = def.baseBlock;
    itemDef.geometry = def.geometry;
    itemDef.nativeBlockLegacyPtr = def.nativeBlockLegacyPtr;
    ::levilamroid::item::registerCustomItem(itemDef);

    return ::levilamroid::block::registerCustomBlock(def);
}

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
    return ll_block_register_with_id(identifier, 0, displayName, textureName, category, creativeGroup, destroyTime, explosionResistance, lightEmission);
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
    def.baseBlock = (baseBlock && baseBlock[0] != '\0') ? baseBlock : "minecraft:stone";

    std::string identStr = identifier;
    std::string shortName = identStr;
    auto colon = shortName.find(':');
    if (colon != std::string::npos) shortName = shortName.substr(colon + 1);

    const auto* existingBlock = ::levilamroid::block::getCustomBlock(identifier);
    if (existingBlock && !existingBlock->geometry.empty()) {
        def.geometry = existingBlock->geometry;
    }
    if (def.geometry.empty()) {
        std::string candidateGeo = "geometry." + shortName;
        if (::levilamroid::geometry::hasGeometry(candidateGeo)) {
            def.geometry = candidateGeo;
        }
    }

    def.category = (category > 0) ? category : 1;
    def.creativeGroup = (creativeGroup && creativeGroup[0] != '\0') ? creativeGroup : std::string(::levilamroid::block::groups::Stone);
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;

    // Register corresponding 3D BlockItem
    ::levilamroid::item::CustomItemDefinition itemDef;
    itemDef.identifier = identifier;
    itemDef.displayName = def.displayName;
    static std::atomic<short> sAutoBlockBaseId{10150};
    const auto* existing = ::levilamroid::item::getCustomItem(identifier);
    itemDef.id = (existing && existing->id > 0) ? existing->id : sAutoBlockBaseId.fetch_add(1);
    itemDef.descriptionId = std::string("tile.") + identifier + ".name";
    itemDef.iconName = ""; // No 2D icon override -> renders 3D block geometry
    itemDef.category = def.category;
    itemDef.creativeGroup = def.creativeGroup;
    itemDef.allowOffhand = true;
    itemDef.maxStackSize = 64;
    itemDef.isBlock = true;
    itemDef.isHandEquipped = false;
    itemDef.isFood = false;
    itemDef.nutrition = 0;
    itemDef.useAnimation = 0;
    itemDef.baseBlock = def.baseBlock;
    itemDef.geometry = def.geometry;
    itemDef.nativeBlockLegacyPtr = def.nativeBlockLegacyPtr;
    ::levilamroid::item::registerCustomItem(itemDef);

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
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (!off.blockTypeRegistryGetDirectAccessBlocks) return nullptr;
    using Fn = const void* (*)(void*);
    auto fn = reinterpret_cast<Fn>(mcBase + off.blockTypeRegistryGetDirectAccessBlocks);
    void* reg = ::levilamroid::block::getEngineBlockTypeRegistry();
    if (reg) {
        return fn(reg);
    }
    return nullptr;
}

LL_API void ll_block_track_placed(int x, int y, int z, const char* identifier) {
    if (identifier) ::levilamroid::block::trackPlacedCustomBlock(x, y, z, identifier);
}

LL_API bool ll_block_get_tracked(int x, int y, int z, char* outIdentifier, size_t maxLen) {
    if (!outIdentifier || maxLen == 0) return false;
    std::string ident;
    if (::levilamroid::block::getTrackedCustomBlock(x, y, z, &ident)) {
        std::strncpy(outIdentifier, ident.c_str(), maxLen - 1);
        outIdentifier[maxLen - 1] = '\0';
        return true;
    }
    return false;
}

LL_API bool ll_block_untrack(int x, int y, int z) {
    return ::levilamroid::block::untrackPlacedCustomBlock(x, y, z, nullptr);
}

LL_API bool ll_block_register_place_listener(LLBlockPlaceCallback callback, void* udata) {
    if (!callback) return false;
    std::lock_guard<std::mutex> lock(::levilamroid::block::sBlockPlaceMutex);
    ::levilamroid::block::sBlockPlaceListeners.push_back({callback, udata});
    return true;
}

LL_API bool ll_fake_block_set(int x, int y, int z, const char* blockIdent, bool clientOnly) {
    return ::levilamroid::block::fakeBlockSet(x, y, z, blockIdent, clientOnly);
}

LL_API bool ll_fake_block_clear(int x, int y, int z) {
    return ::levilamroid::block::fakeBlockClear(x, y, z);
}

LL_API void ll_fake_block_clear_all() {
    ::levilamroid::block::fakeBlockClearAll();
}

} // extern "C"
