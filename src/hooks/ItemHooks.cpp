#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/item/IconInfo.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <string>
#include <mutex>
#include <set>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>

namespace levilamroid::item {

static bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || reinterpret_cast<uintptr_t>(ptr) < 0x10000 || size == 0) return false;
    static int sPipe[2] = {-1, -1};
    static std::once_flag sPipeOnce;
    std::call_once(sPipeOnce, []() {
        if (pipe(sPipe) == 0) {
            fcntl(sPipe[0], F_SETFL, O_NONBLOCK);
            fcntl(sPipe[1], F_SETFL, O_NONBLOCK);
        }
    });
    if (sPipe[1] < 0) return false;
    ssize_t written = write(sPipe[1], ptr, size);
    if (written > 0) {
        char dummy[128];
        while (written > 0) {
            ssize_t r = read(sPipe[0], dummy, std::min<size_t>(written, sizeof(dummy)));
            if (r <= 0) break;
            written -= r;
        }
        return true;
    }
    return false;
}

static std::vector<CustomItemDefinition> sCustomItems;
static std::mutex sItemMutex;

static void* sItemRegistryInstance = nullptr;
static void* sCreativeRegistryInstance = nullptr;
static void* sOrigItemRegistryPostInit = nullptr;
static void* sOrigCreativeCallback = nullptr;
static std::vector<void*> sKnownRegistries;
static std::set<std::pair<void*, std::string>> sRegisteredRegistryItems;

static uintptr_t getMcBase() {
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

struct BedrockSharedPtr {
    BedrockControlBlock* mControlBlock; // +0x00 (8 bytes total!)
};

static void registerSingleItem(void* registry, CustomItemDefinition& def) {
    if (!registry) return;
    if (sRegisteredRegistryItems.count({registry, def.identifier})) return;

    auto& logger = ll::Logger::getInstance();
    uintptr_t mcBase = getMcBase();
    if (!mcBase) return;

    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (!off.itemConstructor || !off.itemRegistryRegisterItem) return;

    // 1. Allocate 512 bytes (0x200) for native Item (sizeof is ~488 bytes)
    void* itemMem = ::operator new(0x200);
    if (!itemMem) return;
    std::memset(itemMem, 0, 0x200);

    // 2. Invoke native Item::Item constructor:
    using ItemCtorFn = void (*)(void*, const std::string&, short);
    auto itemCtor = reinterpret_cast<ItemCtorFn>(mcBase + off.itemConstructor);
    itemCtor(itemMem, def.identifier, def.id);

    // 3. Configure Item properties:
    // Item::mMaxStackSize at offset +0x88 (136 bytes)
    uint8_t maxStack = (def.maxStackSize > 0) ? def.maxStackSize : (def.category == 3 ? 1 : 64);
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x88) = maxStack;

    // Item::mCreativeCategory at offset +0x140 (320 bytes)
    // 1=Construction, 2=Nature, 3=Equipment, 4=Items
    uint8_t category = (def.category > 0) ? static_cast<uint8_t>(def.category) : 4;
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x140) = category;

    // Item::mCreativeGroup at offset +0x148 (HashedString)
    std::string groupName = def.creativeGroup;
    if (groupName.empty()) {
        if (category == 3) {
            groupName = std::string(groups::Sword);
        } else if (def.isBlock) {
            groupName = "itemGroup.name.stone";
        } else {
            groupName = std::string(groups::Misc);
        }
    }
    new (reinterpret_cast<uint8_t*>(itemMem) + 0x148) HashedString(groupName);

    // Item::mAllowOffhand at offset +0x1C8 (456 bytes)
    if (def.allowOffhand || def.isBlock) {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1C8) = 1;
    }

    // Item::mIsHandEquipped at offset +0x1CA (458 bytes)
    if (def.isHandEquipped || category == 3) {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CA) = 1;
    }

    // Item::mMaxDamage at offset +0x1D0 (464 bytes)
    if (def.maxDamage > 0) {
        *reinterpret_cast<short*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1D0) = def.maxDamage;
    }

    // ── Phase 1: Safe Item Configuration ──
    // Note: Do not overwrite internal offsets +0x1C4/+0x1CC/+0x1E0/+0x1E4 as they corrupt
    // native C++ Item layout and trigger SIGSEGV when held or queried by the renderer/engine.
    // Food eating and egg summoning actions are safely intercepted in CoreHooks GameMode handlers.
    if (def.isFood || def.nutrition > 0) {
        logger.info("  → Registered food item: ", def.identifier, " (Nutrition: ", def.nutrition, ")");
    }
    // ── End Phase 1 ──

    // Item::mIconFrame at offset +0x48 (72 bytes) and +0x50
    *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x48) = def.iconFrame;
    *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x50) = def.iconFrame;

    // Invoke authentic Item::setIcon(std::string const& name, int frame) [RVA: 0x0FF83DDC in 1.26.52.3]
    if (!def.iconName.empty() && off.itemSetIconInfo) {
        using SetIconFn = void* (*)(void*, const std::string&, int);
        auto setIcon = reinterpret_cast<SetIconFn>(mcBase + off.itemSetIconInfo);
        setIcon(itemMem, def.iconName, def.iconFrame);
    }

    // 3b. Configure BlockItem properties if this item represents a 3D block
    if (def.isBlock) {
        // Resolve BlockLegacy*: prefer Phase 2 resolved ptr, then live lookup, then stone fallback
        void* legacyBlock = nullptr;

        // A. Check if Phase 2 hook already resolved a real BlockLegacy* for this identifier
        {
            auto customBlocks = ::levilamroid::block::getRegisteredCustomBlocks();
            for (const auto& cb : customBlocks) {
                if (cb.identifier == def.identifier && cb.nativeBlockLegacyPtr) {
                    legacyBlock = cb.nativeBlockLegacyPtr;
                    break;
                }
            }
        }

        // B. Live registry lookup (works after blockTypeRegistryInit has run)
        if (!legacyBlock && off.blockTypeRegistryGetInstance && off.blockTypeRegistryLookupByName) {
            using GetRegistryFn = void* (*)();
            auto getRegistry = reinterpret_cast<GetRegistryFn>(mcBase + off.blockTypeRegistryGetInstance);
            void* blockRegistry = getRegistry();
            if (blockRegistry) {
                using LookupFn = void* (*)(void*, const HashedString&, bool);
                auto lookup = reinterpret_cast<LookupFn>(mcBase + off.blockTypeRegistryLookupByName);
                legacyBlock = lookup(blockRegistry, HashedString(def.identifier), true);
                if (!legacyBlock && !def.baseBlock.empty() && def.baseBlock != def.identifier) {
                    legacyBlock = lookup(blockRegistry, HashedString(def.baseBlock), true);
                }
                // C. Final fallback to stone only as last resort
                if (!legacyBlock) {
                    legacyBlock = lookup(blockRegistry, HashedString("minecraft:stone"), true);
                }
            }
        }

        // Assign authentic BlockItem vtable so Bedrock recognizes it as a true block in inventory
        if (off.blockItemVtable) {
            *reinterpret_cast<uintptr_t*>(itemMem) = mcBase + off.blockItemVtable;
        }

        *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(itemMem) + 312) = legacyBlock;
        if (legacyBlock) {
            def.nativeBlockPermutation = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(legacyBlock) + 0x200);
        }
        logger.info("[LeviLamroid] Block item binding for: ", def.identifier, " -> BlockLegacy: ",
                    legacyBlock ? "REAL" : "STONE_FALLBACK", " (BlockItem vtable assigned)");
    }

    // 4. Registry Control Block (for ItemRegistry::registerItem atomic add at offset +0x0c)
    alignas(16) static uint8_t sRegBlocks[64][32];
    static size_t sRegBlockIdx = 0;
    uint8_t* regBlock = sRegBlocks[sRegBlockIdx % 64];
    sRegBlockIdx++;
    std::memset(regBlock, 0, 32);
    *reinterpret_cast<Item**>(regBlock) = static_cast<Item*>(itemMem);
    *reinterpret_cast<uint32_t*>(regBlock + 0x0c) = 1;

    BedrockSharedPtr sharedItem;
    sharedItem.mControlBlock = reinterpret_cast<BedrockControlBlock*>(regBlock);

    // Invoke ItemRegistry::registerItem(ItemRegistry* this, SharedPtr<Item>* pShared) [RVA: 0x0FBB0064]
    using RegItemFn = void (*)(void*, BedrockSharedPtr*);
    auto regItemFn = reinterpret_cast<RegItemFn>(mcBase + off.itemRegistryRegisterItem);
    regItemFn(registry, &sharedItem);

    sRegisteredRegistryItems.insert({registry, def.identifier});
    def.nativeItemPtr = itemMem;

    // 5. Permanent ItemStack BedrockControlBlock
    // Layout:
    // +0x00: Item*
    // +0x08: mControlBlock = nullptr (CRITICAL: 0x127a9e78 cbz x20 takes branch and skips atomic refcounting!)
    // +0x10: strong = 1
    // +0x14: weak = 1
    auto* itemCtrlBlock = static_cast<BedrockControlBlock*>(::operator new(sizeof(BedrockControlBlock)));
    itemCtrlBlock->mItem = static_cast<Item*>(itemMem);
    itemCtrlBlock->mControlBlock = nullptr;
    itemCtrlBlock->mStrongRef = 1;
    itemCtrlBlock->mWeakRef = 1;
    def.nativeControlBlock = itemCtrlBlock;

    logger.info("§6[LeviLamroid] §aRegistered native item: §b", def.identifier, 
                 " §7(ID: ", def.id, ", Category: ", (int)category, ", MaxStack: ", (int)maxStack, ")");
}

static void hooked_itemRegistryPostInit(void* registry, void* a2) {
    if (sOrigItemRegistryPostInit) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigItemRegistryPostInit)(registry, a2);
    }
    if (!registry) return;

    {
        std::lock_guard<std::mutex> lock(sItemMutex);
        sItemRegistryInstance = registry;
        if (std::find(sKnownRegistries.begin(), sKnownRegistries.end(), registry) == sKnownRegistries.end()) {
            sKnownRegistries.push_back(registry);
        }

        auto& logger = ll::Logger::getInstance();
        logger.info("§6[LeviLamroid] §eItemRegistry (", registry, ") initialized. Injecting ", sCustomItems.size(), " custom items...");

        for (auto& def : sCustomItems) {
            registerSingleItem(registry, def);
        }
    }
}

void* getBlockPermutationForDef(CustomItemDefinition& def) {
    if (!def.isBlock) return nullptr;

    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.blockTypeRegistryGetInstance && off.blockTypeRegistryLookupByName) {
        using GetRegistryFn = void* (*)();
        auto getRegistry = reinterpret_cast<GetRegistryFn>(mcBase + off.blockTypeRegistryGetInstance);
        void* blockRegistry = getRegistry();
        if (blockRegistry && isPointerReadable(blockRegistry, 16)) {
            using LookupFn = void* (*)(void*, const HashedString&, bool);
            auto lookup = reinterpret_cast<LookupFn>(mcBase + off.blockTypeRegistryLookupByName);

            void* realLegacy = lookup(blockRegistry, HashedString(def.identifier), true);
            if (!realLegacy) {
                auto colon = def.identifier.find(':');
                if (colon != std::string::npos) {
                    realLegacy = lookup(blockRegistry, HashedString(def.identifier.substr(colon + 1)), true);
                }
            }

            if (realLegacy) {
                if (def.nativeItemPtr && isPointerReadable(reinterpret_cast<uint8_t*>(def.nativeItemPtr) + 312, sizeof(void*))) {
                    *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(def.nativeItemPtr) + 312) = realLegacy;
                }
                if (isPointerReadable(reinterpret_cast<uint8_t*>(realLegacy) + 0x200, sizeof(void*))) {
                    def.nativeBlockPermutation = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(realLegacy) + 0x200);
                }
                return def.nativeBlockPermutation;
            }
        }
    }

    if (def.nativeBlockPermutation) return def.nativeBlockPermutation;
    return nullptr;
}

void refreshCustomBlockBindings() {
    uintptr_t mcBase = getMcBase();
    if (!mcBase) return;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (!off.blockTypeRegistryGetInstance || !off.blockTypeRegistryLookupByName) return;

    using GetRegistryFn = void* (*)();
    auto getRegistry = reinterpret_cast<GetRegistryFn>(mcBase + off.blockTypeRegistryGetInstance);
    void* blockRegistry = getRegistry();
    if (!blockRegistry || !isPointerReadable(blockRegistry, 16)) return;

    using LookupFn = void* (*)(void*, const HashedString&, bool);
    auto lookup = reinterpret_cast<LookupFn>(mcBase + off.blockTypeRegistryLookupByName);

    auto& logger = ll::Logger::getInstance();
    std::lock_guard<std::mutex> lock(sItemMutex);

    for (auto& item : sCustomItems) {
        if (!item.isBlock || !item.nativeItemPtr) continue;

        void* realLegacy = lookup(blockRegistry, HashedString(item.identifier), true);
        if (!realLegacy) {
            auto colon = item.identifier.find(':');
            if (colon != std::string::npos) {
                realLegacy = lookup(blockRegistry, HashedString(item.identifier.substr(colon + 1)), true);
            }
        }

        if (realLegacy) {
            void* oldLegacy = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(item.nativeItemPtr) + 312);
            if (oldLegacy != realLegacy) {
                *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(item.nativeItemPtr) + 312) = realLegacy;
                if (isPointerReadable(reinterpret_cast<uint8_t*>(realLegacy) + 0x200, sizeof(void*))) {
                    item.nativeBlockPermutation = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(realLegacy) + 0x200);
                }
                logger.info("§6[LeviLamroid] §aBound custom BlockItem '", item.identifier,
                            "' to authentic BlockLegacy @ ", realLegacy);
            }
        }
    }
}

static void hooked_serverInitCreativeItemsCallback(
    void* itemRegistryRef,
    void* blockDefinitionGroup,
    void* creativeItemRegistry,
    void* worldVersion,
    void* experiments,
    void* resourcePackManager,
    void* ctx,
    void* validator,
    void* eventing
) {
    sCreativeRegistryInstance = creativeItemRegistry;

    if (sOrigCreativeCallback) {
        reinterpret_cast<void(*)(void*, void*, void*, void*, void*, void*, void*, void*, void*)>(
            sOrigCreativeCallback)(
            itemRegistryRef, blockDefinitionGroup, creativeItemRegistry,
            worldVersion, experiments, resourcePackManager, ctx, validator, eventing
        );
    }

    if (!creativeItemRegistry) return;

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[LeviLamroid] §eCreativeItemRegistry callback active. Custom items ready (", sCustomItems.size(), " items registered).");
    refreshCustomBlockBindings();
}

bool registerCustomItem(const CustomItemDefinition& def) {
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (auto& item : sCustomItems) {
        if (item.identifier == def.identifier) {
            item = def;
            for (void* reg : sKnownRegistries) {
                registerSingleItem(reg, item);
            }
            return true;
        }
    }
    sCustomItems.push_back(def);
    for (void* reg : sKnownRegistries) {
        registerSingleItem(reg, sCustomItems.back());
    }
    return true;
}

Item* getNativeItem(const std::string& identifier) {
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (const auto& item : sCustomItems) {
        if (item.identifier == identifier) {
            return static_cast<Item*>(item.nativeItemPtr);
        }
    }
    return nullptr;
}

Item* getNativeItem(short id) {
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (const auto& item : sCustomItems) {
        if (item.id == id) {
            return static_cast<Item*>(item.nativeItemPtr);
        }
    }
    return nullptr;
}

Item& Item::setIconInfo(const std::string& name, int frame) {
    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (mcBase && off.itemSetIconInfo) {
        using SetIconFn = void* (*)(void*, const std::string&, int);
        auto setIcon = reinterpret_cast<SetIconFn>(mcBase + off.itemSetIconInfo);
        setIcon(this, name, frame);
    } else {
        setIconFrame(frame);
    }
    return *this;
}

ItemRegistry& ItemRegistry::getInstance() {
    static ItemRegistry instance;
    return instance;
}

Item* ItemRegistry::getItem(short id) {
    return getNativeItem(id);
}

Item* ItemRegistry::getItem(std::string_view identifier) {
    return getNativeItem(std::string(identifier));
}

bool ItemRegistry::registerItem(const CustomItemDefinition& def) {
    return registerCustomItem(def);
}

bool ItemRegistry::registerItem(
    std::string_view identifier, 
    short id, 
    std::string_view name, 
    std::string_view icon, 
    CreativeItemCategory category, 
    std::string_view group, 
    uint8_t maxStack, 
    bool allowOffhand
) {
    CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = name.empty() ? ("item." + def.identifier) : std::string(name);
    def.iconName = icon;
    def.iconFrame = 0;
    def.category = static_cast<int>(category);
    def.creativeGroup = group;
    def.allowOffhand = allowOffhand;
    def.maxStackSize = maxStack;
    return registerCustomItem(def);
}

bool giveItemToPlayer(void* player, const std::string& identifier, int count, int slot) {
    if (!player) return false;

    CustomItemDefinition* targetDef = nullptr;
    {
        std::lock_guard<std::mutex> lock(sItemMutex);
        for (auto& item : sCustomItems) {
            if (item.identifier == identifier) {
                targetDef = &item;
                break;
            }
        }
    }
    if (!targetDef || !targetDef->nativeItemPtr || !targetDef->nativeControlBlock) return false;

    uint8_t* playerBytes = reinterpret_cast<uint8_t*>(player);
    void* supplies = *reinterpret_cast<void**>(playerBytes + 0x570);
    if (!supplies) return false;

    uint8_t* suppliesBytes = reinterpret_cast<uint8_t*>(supplies);
    if (slot < 0) {
        slot = *reinterpret_cast<int*>(suppliesBytes + 0x10);
        if (slot < 0 || slot > 8) slot = 0;
    }

    void* inventory = *reinterpret_cast<void**>(suppliesBytes + 0xb8);
    if (!inventory) return false;

    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    // Prepare 152-byte ItemStack on stack
    alignas(16) uint8_t stackMem[160];
    std::memset(stackMem, 0, sizeof(stackMem));
    if (off.itemStackVtable) {
        *reinterpret_cast<uintptr_t*>(stackMem) = mcBase + off.itemStackVtable;
    }
    *reinterpret_cast<void**>(stackMem + 0x08) = targetDef->nativeControlBlock;
    if (targetDef->isBlock) {
        void* blockState = getBlockPermutationForDef(*targetDef);
        *reinterpret_cast<void**>(stackMem + 0x18) = blockState;
    }
    *reinterpret_cast<uint16_t*>(stackMem + 0x20) = 0; // aux = 0
    stackMem[0x22] = static_cast<uint8_t>(count > 0 ? count : 1);
    stackMem[0x23] = 1; // valid = 1

    // Call Container::setItem [RVA: off.containerSetItem (0x0F6EFEB8)]
    if (off.containerSetItem) {
        using SetItemFn = void (*)(void*, int, const void*);
        auto setItemFn = reinterpret_cast<SetItemFn>(mcBase + off.containerSetItem);
        setItemFn(inventory, slot, stackMem);
    } else {
        // Fallback: direct slot write
        uint8_t* invBytes = reinterpret_cast<uint8_t*>(inventory);
        uint8_t* slotsBegin = *reinterpret_cast<uint8_t**>(invBytes + 0x140);
        if (slotsBegin) {
            uint8_t* itemAddr = slotsBegin + (slot * 152);
            std::memcpy(itemAddr, stackMem, 152);
        }
    }

    // Trigger UI container slot refresh (vtable[35])
    void** invVtable = *reinterpret_cast<void***>(inventory);
    if (invVtable && invVtable[35]) {
        using TriggerFn = void (*)(void*, int);
        auto triggerFn = reinterpret_cast<TriggerFn>(invVtable[35]);
        triggerFn(inventory, slot);
    }
    return true;
}

void forEachRegisteredItem(void (*callback)(const CustomItemDefinition& def, void* udata), void* udata) {
    if (!callback) return;
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (const auto& item : sCustomItems) {
        callback(item, udata);
    }
}

std::vector<CustomItemDefinition>& getAllCustomItems() {
    return sCustomItems;
}

bool installItemHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.itemRegistryPostInitHook) {
        if (ll::memory::hookRva(off.itemRegistryPostInitHook, (void*)&hooked_itemRegistryPostInit, &sOrigItemRegistryPostInit)) {
            logger.info("Hooked ItemRegistry Post-Init [RVA: 0x", std::hex, off.itemRegistryPostInitHook, "]");
        }
    }

    if (off.vanillaItemsServerInitCreativeCallback) {
        if (ll::memory::hookRva(off.vanillaItemsServerInitCreativeCallback, (void*)&hooked_serverInitCreativeItemsCallback, &sOrigCreativeCallback)) {
            logger.info("Hooked VanillaItems::serverInitCreativeItemsCallback [RVA: 0x", std::hex, off.vanillaItemsServerInitCreativeCallback, "]");
        }
    }

    logger.info("LeviLamroid item subsystem active.");
    return true;
}

} // namespace levilamroid::item

extern "C" {

LL_API bool ll_item_register(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, bool allowOffhand) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = descriptionId ? descriptionId : ("item." + def.identifier);
    def.iconName = iconName ? iconName : "";
    def.iconFrame = iconFrame;
    def.category = category;
    def.allowOffhand = allowOffhand;
    def.maxStackSize = (category == 3) ? 1 : 64; // Swords/Equipment = 1, otherwise 64
    return levilamroid::item::registerCustomItem(def);
}

LL_API bool ll_item_register_ex(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, bool allowOffhand, uint8_t maxStackSize) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = descriptionId ? descriptionId : ("item." + def.identifier);
    def.iconName = iconName ? iconName : "";
    def.iconFrame = iconFrame;
    def.category = category;
    def.allowOffhand = allowOffhand;
    def.maxStackSize = maxStackSize;
    return levilamroid::item::registerCustomItem(def);
}

LL_API bool ll_item_register_full(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, const char* creativeGroup, bool allowOffhand, uint8_t maxStackSize) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = descriptionId ? descriptionId : ("item." + def.identifier);
    def.iconName = iconName ? iconName : "";
    def.iconFrame = iconFrame;
    def.category = category;
    def.creativeGroup = creativeGroup ? creativeGroup : "";
    def.allowOffhand = allowOffhand;
    def.maxStackSize = maxStackSize;
    return levilamroid::item::registerCustomItem(def);
}

LL_API void* ll_item_get_native(const char* identifier) {
    if (!identifier) return nullptr;
    return levilamroid::item::getNativeItem(std::string(identifier));
}

LL_API void* ll_item_get_by_id(short id) {
    return levilamroid::item::getNativeItem(id);
}

LL_API bool ll_item_give_player(void* player, const char* identifier, int count, int slot) {
    if (!identifier) return false;
    return levilamroid::item::giveItemToPlayer(player, identifier, count, slot);
}

LL_API void ll_item_set_category(void* item, int category) {
    if (item) {
        static_cast<levilamroid::item::Item*>(item)->setCreativeCategory(static_cast<levilamroid::item::CreativeItemCategory>(category));
    }
}

LL_API void ll_item_set_group(void* item, const char* group) {
    if (item && group) {
        static_cast<levilamroid::item::Item*>(item)->setCreativeGroup(group);
    }
}

LL_API void ll_item_set_max_stack(void* item, uint8_t maxStack) {
    if (item) {
        static_cast<levilamroid::item::Item*>(item)->setMaxStackSize(maxStack);
    }
}

LL_API void ll_item_set_offhand_allowed(void* item, bool allowed) {
    if (item) {
        static_cast<levilamroid::item::Item*>(item)->setAllowOffhand(allowed);
    }
}

LL_API void ll_item_set_icon(void* item, const char* iconName, int iconFrame) {
    if (item && iconName) {
        static_cast<levilamroid::item::Item*>(item)->setIconInfo(iconName, iconFrame);
    }
}

LL_API const char* ll_item_group_get_name(int groupEnum) {
    auto view = levilamroid::item::getGroupName(static_cast<levilamroid::item::CreativeGroup>(groupEnum));
    return view.data();
}

// ── Phase 1: Food / Tool / Weapon convenience registration ──

LL_API bool ll_item_register_food(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int nutrition, float saturationModifier, int category, const char* creativeGroup) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = descriptionId ? descriptionId : ("item." + def.identifier);
    def.iconName = iconName ? iconName : "";
    def.iconFrame = iconFrame;
    def.category = (category > 0) ? category : 2; // Nature by default for food
    def.creativeGroup = creativeGroup ? creativeGroup : "itemGroup.name.miscFood";
    def.allowOffhand = true;
    def.maxStackSize = 64;
    def.isFood = true;
    def.nutrition = nutrition;
    def.saturationModifier = saturationModifier;
    def.useAnimation = 1;  // EAT
    def.useDuration = 32;  // 1.6 seconds
    return levilamroid::item::registerCustomItem(def);
}

LL_API bool ll_item_register_tool(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, float attackDamage, short durability, float miningSpeed, int category, const char* creativeGroup) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = descriptionId ? descriptionId : ("item." + def.identifier);
    def.iconName = iconName ? iconName : "";
    def.iconFrame = iconFrame;
    def.category = (category > 0) ? category : 3; // Equipment by default for tools
    def.creativeGroup = creativeGroup ? creativeGroup : "itemGroup.name.pickaxe";
    def.allowOffhand = false;
    def.maxStackSize = 1;
    def.isHandEquipped = true;
    def.maxDamage = durability;
    def.attackDamage = attackDamage;
    def.miningSpeed = miningSpeed;
    return levilamroid::item::registerCustomItem(def);
}

LL_API bool ll_item_register_weapon(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, float attackDamage, short durability, int category, const char* creativeGroup) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    def.descriptionId = descriptionId ? descriptionId : ("item." + def.identifier);
    def.iconName = iconName ? iconName : "";
    def.iconFrame = iconFrame;
    def.category = (category > 0) ? category : 3; // Equipment by default for weapons
    def.creativeGroup = creativeGroup ? creativeGroup : "itemGroup.name.sword";
    def.allowOffhand = false;
    def.maxStackSize = 1;
    def.isHandEquipped = true;
    def.maxDamage = durability;
    def.attackDamage = attackDamage;
    return levilamroid::item::registerCustomItem(def);
}

// ── End Phase 1 ──

} // extern "C"
