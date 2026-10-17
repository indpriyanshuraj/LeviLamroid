#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/item/IconInfo.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <string>
#include <mutex>
#include <set>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace levilamroid::item {

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

// VTable storage for Custom BlockItems:
// Indices:
// -2: offset-to-top (0)
// -1: RTTI pointer (_ZTI9BlockItem)
//  0..132: virtual methods from original _ZTV9BlockItem
static uintptr_t sCustomBlockItemVtableStorage[160];
static uintptr_t* sCustomBlockItemVtable = &sCustomBlockItemVtableStorage[2];
static bool sCustomBlockItemVtableInit = false;

struct CustomBlockItemVT {
    // Slot 95: buildDescriptionId
    static std::string buildDescriptionId(const void* itemSelf, const void* itemStack) {
        if (!itemSelf) return "tile.unknown.name";
        const auto* descId = reinterpret_cast<const std::string*>(reinterpret_cast<const uint8_t*>(itemSelf) + 0x90);
        if (descId && !descId->empty()) {
            return *descId;
        }
        return "tile.unknown.name";
    }

    // Slot 118: getIconInfo
    static ::levilamroid::item::ItemIconInfo getIconInfo(const void* itemSelf, const void* itemStack, int unk) {
        if (!itemSelf) return ::levilamroid::item::ItemIconInfo();
        const auto* iconName = reinterpret_cast<const std::string*>(reinterpret_cast<const uint8_t*>(itemSelf) + 0x58);
        int frame = *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(itemSelf) + 0x48);
        return ::levilamroid::item::ItemIconInfo(*iconName, frame, 1);
    }

    // Slot 131: _useOn
    static bool useOn(const void* itemSelf, void* instance, void* entity, void* blockPos, unsigned char face, const void* clickPos) {
        if (!itemSelf) return false;
        auto* itemPtr = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(itemSelf));
        void** blockTypeSlot = reinterpret_cast<void**>(itemPtr + 0x138);

        uintptr_t mcBase = getMcBase();
        const auto& off = mcpe::getOffsets();

        // Lazily ensure backing BlockType is populated if not resolved at registration
        if (!*blockTypeSlot && off.blockTypeRegistryGetInstance && off.blockTypeRegistryLookupByName) {
            using GetInstFn = void* (*)();
            using LookupFn = void* (*)(void*, const HashedString&, int);
            auto getInst = reinterpret_cast<GetInstFn>(mcBase + off.blockTypeRegistryGetInstance);
            auto lookup = reinterpret_cast<LookupFn>(mcBase + off.blockTypeRegistryLookupByName);
            void* reg = getInst ? getInst() : nullptr;
            if (reg && lookup) {
                const auto* nameStr = reinterpret_cast<const std::string*>(itemPtr + 0x90);
                if (nameStr && !nameStr->empty()) {
                    HashedString customId(*nameStr);
                    *blockTypeSlot = lookup(reg, customId, 1);
                }
                if (!*blockTypeSlot) {
                    HashedString cryingObsidian("minecraft:crying_obsidian");
                    *blockTypeSlot = lookup(reg, cryingObsidian, 1);
                }
                if (!*blockTypeSlot) {
                    HashedString amethyst("minecraft:amethyst_block");
                    *blockTypeSlot = lookup(reg, amethyst, 1);
                }
                if (!*blockTypeSlot) {
                    HashedString stone("minecraft:stone");
                    *blockTypeSlot = lookup(reg, stone, 1);
                }
                if (*blockTypeSlot) {
                    ll::Logger::getInstance().info("BlockItem: Lazily resolved backing BlockType: ", *blockTypeSlot);
                }
            }
        }

        if (off.blockItemUseOn) {
            using UseOnFn = bool (*)(const void*, void*, void*, void*, unsigned char, const void*);
            auto origUseOn = reinterpret_cast<UseOnFn>(mcBase + off.blockItemUseOn);
            return origUseOn(itemSelf, instance, entity, blockPos, face, clickPos);
        }
        return false;
    }
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

    // 2. Invoke Item::Item(std::string const& nameId, short id) [RVA: 0x0FF7D35C]
    // Note: Item constructor automatically sets mRawNameId and mDescriptionId from identifier!
    using ItemCtorFn = void (*)(void*, const std::string&, short);
    auto itemCtor = reinterpret_cast<ItemCtorFn>(mcBase + off.itemConstructor);
    itemCtor(itemMem, def.identifier, def.id);

    // 3. Configure Item properties
    // Item::mMaxStackSize at offset +0x88 (136 bytes)
    uint8_t maxStack = (def.maxStackSize > 0) ? def.maxStackSize : (def.category == 3 ? 1 : 64);
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x88) = maxStack;

    // Item::mCreativeCategory at offset +0x140 (320 bytes)
    // 1=Construction, 2=Nature, 3=Equipment, 4=Items
    uint8_t category = (def.category > 0) ? static_cast<uint8_t>(def.category) : 4;
    *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x140) = category;

    // Item::mCreativeGroup at offset +0x148 (HashedString: 8-byte FNV1a hash + 24-byte SSO string)
    std::string groupName = def.creativeGroup;
    if (groupName.empty()) {
        if (category == 3) {
            groupName = std::string(groups::Sword);
        } else {
            groupName = std::string(groups::Misc);
        }
    }
    new (reinterpret_cast<uint8_t*>(itemMem) + 0x148) HashedString(groupName);

    // Item::mAllowOffhand at offset +0x1C8 (456 bytes)
    if (def.allowOffhand) {
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

    if (def.isBlock) {
        // Promote to BlockItem VTable so game treats it as 3D block geometry
        if (off.blockItemVtable) {
            if (!sCustomBlockItemVtableInit) {
                const uintptr_t* origVtable = reinterpret_cast<const uintptr_t*>(mcBase + off.blockItemVtable);
                // Header: offset-to-top (0) and RTTI
                sCustomBlockItemVtableStorage[0] = 0;
                sCustomBlockItemVtableStorage[1] = *(origVtable - 1);
                // Copy all 133 virtual functions
                std::memcpy(sCustomBlockItemVtable, origVtable, 133 * sizeof(uintptr_t));
                // Override Slot 95 (buildDescriptionId)
                sCustomBlockItemVtable[95] = reinterpret_cast<uintptr_t>(&CustomBlockItemVT::buildDescriptionId);
                // Override Slot 118 (getIconInfo)
                sCustomBlockItemVtable[118] = reinterpret_cast<uintptr_t>(&CustomBlockItemVT::getIconInfo);
                // Override Slot 131 (_useOn)
                sCustomBlockItemVtable[131] = reinterpret_cast<uintptr_t>(&CustomBlockItemVT::useOn);
                sCustomBlockItemVtableInit = true;
                ll::Logger::getInstance().info("ItemHooks: Initialized sCustomBlockItemVtable from _ZTV9BlockItem (0x",
                                               std::hex, off.blockItemVtable, ")");
            }
            *reinterpret_cast<uintptr_t*>(itemMem) = reinterpret_cast<uintptr_t>(sCustomBlockItemVtable);
        }

        // Set bit 0x4 at offset +0x112 (BlockItem flag checked by Item::isBlock)
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x112) |= 0x4;

        // Texture atlas: blocks reside on "atlas.terrain"
        *reinterpret_cast<std::string*>(reinterpret_cast<uint8_t*>(itemMem) + 0x10) = "atlas.terrain";

        // Description ID: "tile." + def.identifier + ".name"
        std::string descId = def.descriptionId.empty() ? ("tile." + def.identifier + ".name") : def.descriptionId;
        *reinterpret_cast<std::string*>(reinterpret_cast<uint8_t*>(itemMem) + 0x90) = descId;

        // Block item icon name: matches the terrain texture key
        if (!def.iconName.empty()) {
            *reinterpret_cast<std::string*>(reinterpret_cast<uint8_t*>(itemMem) + 0x58) = def.iconName;
        }

        // Eagerly resolve backing BlockType if registry is available
        if (off.blockTypeRegistryGetInstance && off.blockTypeRegistryLookupByName) {
            using GetInstFn = void* (*)();
            using LookupFn = void* (*)(void*, const HashedString&, int);
            auto getInst = reinterpret_cast<GetInstFn>(mcBase + off.blockTypeRegistryGetInstance);
            auto lookup = reinterpret_cast<LookupFn>(mcBase + off.blockTypeRegistryLookupByName);
            void* reg = getInst ? getInst() : nullptr;
            void* blockType = nullptr;
            if (reg && lookup) {
                HashedString customId(def.identifier);
                blockType = lookup(reg, customId, 1);
                if (!blockType) {
                    HashedString cryingObsidian("minecraft:crying_obsidian");
                    blockType = lookup(reg, cryingObsidian, 1);
                }
                if (!blockType) {
                    HashedString amethyst("minecraft:amethyst_block");
                    blockType = lookup(reg, amethyst, 1);
                }
                if (!blockType) {
                    HashedString stone("minecraft:stone");
                    blockType = lookup(reg, stone, 1);
                }
            }
            if (blockType) {
                *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(itemMem) + 0x138) = blockType;
                ll::Logger::getInstance().info("ItemHooks: Assigned backing BlockType (", blockType, ") to ", def.identifier);
            }
        }
    } else {
        // Item::mDescriptionId at offset +0x90 (144 bytes)
        if (!def.descriptionId.empty()) {
            auto* descStr = reinterpret_cast<std::string*>(reinterpret_cast<uint8_t*>(itemMem) + 0x90);
            *descStr = def.descriptionId;
        }

        // Item::mIconFrame at offset +0x48 (72 bytes) and +0x50
        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x48) = def.iconFrame;
        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x50) = def.iconFrame;

        // Item::mIconName (std::string) at offset +0x58 (88 bytes)
        if (!def.iconName.empty()) {
            auto* iconStr = reinterpret_cast<std::string*>(reinterpret_cast<uint8_t*>(itemMem) + 0x58);
            *iconStr = def.iconName;

            // Invoke native Item::setIcon(std::string const& name, int frame) [RVA: 0x0FF83DDC in 1.26.52.3]
            if (off.itemSetIconInfo) {
                using SetIconFn = void* (*)(void*, const std::string&, int);
                auto setIcon = reinterpret_cast<SetIconFn>(mcBase + off.itemSetIconInfo);
                setIcon(itemMem, def.iconName, def.iconFrame);
            }
        }
    }

    // 4. Allocate 16 bytes for Bedrock SharedPtrControlBlock
    auto* ctrlBlock = static_cast<BedrockControlBlock*>(::operator new(sizeof(BedrockControlBlock)));
    if (!ctrlBlock) return;
    ctrlBlock->mItem = static_cast<Item*>(itemMem);
    ctrlBlock->mWeakRef = 1;    // weak ref = 1
    ctrlBlock->mStrongRef = 1;  // strong ref = 1

    // 5. Build Bedrock SharedPtr<Item> composite struct on stack: { mControlBlock }
    BedrockSharedPtr sharedItem;
    sharedItem.mControlBlock = ctrlBlock;

    // 6. Invoke ItemRegistry::registerItem(ItemRegistry* this, SharedPtr<Item>* pShared) [RVA: 0x0FBB0064]
    using RegItemFn = void (*)(void*, BedrockSharedPtr*);
    auto regItemFn = reinterpret_cast<RegItemFn>(mcBase + off.itemRegistryRegisterItem);
    regItemFn(registry, &sharedItem);

    sRegisteredRegistryItems.insert({registry, def.identifier});
    def.nativeItemPtr = itemMem;
    def.nativeControlBlock = ctrlBlock;

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
    std::lock_guard<std::mutex> lock(sItemMutex);
    logger.info("§6[LeviLamroid] §eCreativeItemRegistry callback active. Injecting custom creative items...");

    uintptr_t mcBase = getMcBase();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    for (const auto& def : sCustomItems) {
        if (!def.nativeItemPtr) continue;

        // Build stack ItemInstance (128 bytes)
        alignas(16) uint8_t itemInst[128];
        std::memset(itemInst, 0, sizeof(itemInst));
        if (off.itemStackVtable) {
            *reinterpret_cast<uintptr_t*>(itemInst) = mcBase + off.itemStackVtable;
        }
        *reinterpret_cast<void**>(itemInst + 8) = def.nativeControlBlock;
        *reinterpret_cast<uint16_t*>(itemInst + 0x20) = 0; // aux = 0
        itemInst[0x22] = (def.category == 3) ? 1 : 64; // count
        itemInst[0x23] = 1; // valid = 1

        // 1. Try engine native CreativeItemInitializer::_addCreativeItemServer [RVA: 0x0FFFD1C0]
        if (off.creativeItemInitializerAddCreativeServer) {
            using AddCreativeFn = void (*)(void*, int, void*, void*, void*);
            auto addCreativeFn = reinterpret_cast<AddCreativeFn>(mcBase + off.creativeItemInitializerAddCreativeServer);
            addCreativeFn(itemRegistryRef, def.category, creativeItemRegistry, itemInst, nullptr);
            logger.info("§aAdded to Creative Inventory via _addCreativeItemServer: §b", def.identifier);
            continue;
        }

        // 2. Fallback: direct vector insertion at CreativeItemRegistry + 0xA0
        uint8_t* creativeBytes = reinterpret_cast<uint8_t*>(creativeItemRegistry);
        uint8_t* itemsBegin = *reinterpret_cast<uint8_t**>(creativeBytes + 0xA0);
        uint8_t* itemsEnd   = *reinterpret_cast<uint8_t**>(creativeBytes + 0xA8);
        uint8_t* itemsCap   = *reinterpret_cast<uint8_t**>(creativeBytes + 0xB0);

        if (itemsBegin && itemsEnd && itemsEnd + 128 <= itemsCap) {
            std::memset(itemsEnd, 0, 128);
            *reinterpret_cast<void**>(itemsEnd) = creativeItemRegistry;
            *reinterpret_cast<uint32_t*>(itemsEnd + 8) = static_cast<uint32_t>(def.groupIndex);
            uint32_t currentCount = static_cast<uint32_t>((itemsEnd - itemsBegin) / 128);
            *reinterpret_cast<uint32_t*>(itemsEnd + 12) = 10000 + currentCount;
            std::memcpy(itemsEnd + 16, itemInst, 112);

            itemsEnd += 128;
            *reinterpret_cast<uint8_t**>(creativeBytes + 0xA8) = itemsEnd;
            logger.info("§aAdded to Creative Inventory via vector push: §b", def.identifier);
        }
    }
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

} // extern "C"
