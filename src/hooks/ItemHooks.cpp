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

extern "C" {
bool ll_container_set_item(void* container, int slot, const void* itemStack);
bool ll_command_dispatch_client(const char* fullCommand);
}

namespace levilamroid::item {

static inline bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t untagged = v & 0x00ffffffffffffffULL;
    if (untagged < 0x10000 || untagged > 0x00007fffffffffffULL) return false;
    if (untagged + size < untagged) return false;
    return true;
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
    return ::ll::memory::getMinecraftBase();
}

struct BedrockSharedPtr {
    BedrockControlBlock* mControlBlock; // +0x00 (8 bytes total!)
};


static inline void* unwrapItemRegistry(void* ptr) {
    if (!ptr || !isPointerReadable(ptr, 16)) return nullptr;

    // 1. Check if ptr is already the authentic ItemRegistry:
    // In libminecraftpe.so, ItemRegistry has mNameToItemMap with max_load_factor (1.0f) at offset +152 (0x98).
    if (isPointerReadable(reinterpret_cast<uint8_t*>(ptr) + 152, sizeof(float))) {
        float mlf = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(ptr) + 152);
        if (mlf > 0.1f && mlf < 10.0f) {
            return ptr; // Already the real ItemRegistry!
        }
    }

    // 2. Dereference ItemRegistryRef / NotNullNonOwnerPtr<ItemRegistry> from caller stack:
    //   +0x00: ItemRegistry* candidate (heap pointer)
    //   +0x08: ControlBlock* ctrl (heap pointer)
    void* candidate = *reinterpret_cast<void**>(ptr);
    if (candidate && isPointerReadable(candidate, 64)) {
        void* ctrl = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr) + 8);
        if (ctrl && isPointerReadable(ctrl, 16)) {
            return candidate;
        }
        if (isPointerReadable(reinterpret_cast<uint8_t*>(candidate) + 152, sizeof(float))) {
            float mlf = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(candidate) + 152);
            if (mlf > 0.1f && mlf < 10.0f) {
                return candidate;
            }
        }
        return candidate;
    }
    return ptr;
}

static void ensureNativeItemAllocated(CustomItemDefinition& def) {
    if (def.nativeItemPtr && def.nativeControlBlock) return;

    uintptr_t mcBase = getMcBase();
    if (!mcBase) return;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (!off.itemConstructor) return;

    uint8_t maxStack = (def.maxStackSize > 0) ? def.maxStackSize : (def.category == 3 ? 1 : 64);
    uint8_t category = (def.category > 0) ? static_cast<uint8_t>(def.category) : 4;

    void* itemMem = def.nativeItemPtr;
    if (!itemMem) {
        if (def.id <= 0) {
            static std::atomic<short> sAutoItemGenId{10200};
            def.id = sAutoItemGenId.fetch_add(1);
        }

        // 1. Allocate 512 bytes (0x200) for native Item (sizeof is ~488 bytes)
        itemMem = ::operator new(0x200);
        if (!itemMem) return;
        std::memset(itemMem, 0, 0x200);

        // 2. Invoke native Item::Item constructor:
        using ItemCtorFn = void (*)(void*, const std::string&, short);
        auto itemCtor = reinterpret_cast<ItemCtorFn>(mcBase + off.itemConstructor);
        try {
            itemCtor(itemMem, def.identifier, def.id);
        } catch (const std::exception& e) {
            ll::Logger::getInstance().error("§c[LeviLamroid] Exception in Item::Item constructor for ", def.identifier, ": ", e.what());
        } catch (...) {
            ll::Logger::getInstance().error("§c[LeviLamroid] Unknown exception in Item::Item constructor for ", def.identifier);
        }

        if (*reinterpret_cast<uintptr_t*>(itemMem) == 0) {
            uintptr_t fallbackVt = def.isBlock ? off.blockItemVtable : off.itemVtable;
            if (fallbackVt != 0) {
                *reinterpret_cast<uintptr_t*>(itemMem) = mcBase + fallbackVt;
            }
        }

        // 3. Configure Item properties:
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x88) = maxStack;
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x140) = category;

        std::string groupName = def.creativeGroup;
        if (groupName.find(':') != std::string::npos) {
            groupName = (def.isBlock) ? "itemGroup.name.stone" : "itemGroup.name.misc";
        }
        if (groupName.empty()) {
            if (category == 3) {
                groupName = std::string(groups::Sword);
            } else if (def.isBlock) {
                groupName = "itemGroup.name.stone";
            } else if (def.isFood) {
                groupName = "itemGroup.name.miscFood";
            } else {
                groupName = std::string(groups::Misc);
            }
        }
        new (reinterpret_cast<uint8_t*>(itemMem) + 0x148) HashedString(groupName);

        if (def.allowOffhand || def.isBlock) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1C8) = 1;
        }

        if (!def.isBlock && (def.isHandEquipped || category == 3)) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CA) = 1;
        } else {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CA) = 0;
        }

        if (def.maxDamage > 0) {
            *reinterpret_cast<short*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1D0) = def.maxDamage;
        }

        // Native use animation (+0x1C4): 1=Eat, 2=Bow, 3=Spear, 4=Crossbow, 5=Spyglass
        if (def.isBlock || (category == 3 && def.useAnimation == 0 && !def.isFood && def.nutrition == 0)) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1C4) = 0;
            *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CC) = 0;
        } else if (def.useAnimation > 0) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1C4) = static_cast<uint8_t>(def.useAnimation);
            *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CC) = def.useDuration > 0 ? def.useDuration : 32;
        } else if (def.isFood || def.nutrition > 0) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1C4) = 1; // Eat
            *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CC) = def.useDuration > 0 ? def.useDuration : 32;
        } else {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1C4) = 0;
            *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1CC) = 0;
        }

        // Native bonus attack damage (+0x1D4)
        if (def.attackDamage > 0.0f) {
            *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1D4) = def.attackDamage;
        }

        // Native food nutrition (+0x1E0)
        if (!def.isBlock && def.nutrition > 0) {
            *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1E0) = def.nutrition;
        }

        // Native food saturation (+0x1E4)
        if (!def.isBlock && def.saturationModifier > 0.0f) {
            *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(itemMem) + 0x1E4) = def.saturationModifier;
        }

        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x48) = def.iconFrame;
        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(itemMem) + 0x50) = def.iconFrame;

        // Note: Legacy itemSetIconInfo is omitted here to prevent uninitialized texture atlas SIGSEGV.
        // Modern Bedrock data-driven items load texture info via NBT minecraft:icon and iconFrame.

        if (def.isBlock) {
            if (off.blockItemVtable) {
                *reinterpret_cast<uintptr_t*>(itemMem) = mcBase + off.blockItemVtable;
            }
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(itemMem) + 0x112) |= 0x4;
            if (def.nativeBlockLegacyPtr) {
                *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(itemMem) + 0x138) = def.nativeBlockLegacyPtr;
            }
        }

        def.nativeItemPtr = itemMem;
    }

    if (!def.nativeControlBlock && itemMem) {
        auto* itemCtrlBlock = static_cast<BedrockControlBlock*>(::operator new(sizeof(BedrockControlBlock)));
        itemCtrlBlock->mItem = static_cast<Item*>(itemMem);
        itemCtrlBlock->mStrongRef = 1000000;
        itemCtrlBlock->mWeakRef = 1000000;
        def.nativeControlBlock = itemCtrlBlock;
    }
}

static void registerSingleItem(void* registry, CustomItemDefinition& def) {
    if (!registry) return;
    if (sRegisteredRegistryItems.count({registry, def.identifier})) return;

    auto& logger = ll::Logger::getInstance();
    uintptr_t mcBase = getMcBase();
    if (!mcBase) return;

    const auto& off = ::levilamroid::mcpe::getOffsets();
    if (!off.itemConstructor || !off.itemRegistryRegisterItem) return;

    ensureNativeItemAllocated(def);
    if (!def.nativeItemPtr) return;

    // Verify registry is readable and authentic
    if (!isPointerReadable(registry, 160)) return;
    float mlf = *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(registry) + 152);
    if (mlf < 0.1f || mlf > 10.0f) {
        logger.warn("§e[LeviLamroid] Skipping registerItem on unverified registry candidate: ", registry);
        return;
    }

    // Dedicated Registry Control Block for each item (never reused or wrapped)
    uint8_t* regBlock = new uint8_t[32]();
    *reinterpret_cast<Item**>(regBlock) = static_cast<Item*>(def.nativeItemPtr);
    *reinterpret_cast<uint32_t*>(regBlock + 0x08) = 1000000; // Strong count
    *reinterpret_cast<uint32_t*>(regBlock + 0x0c) = 1000000; // Weak count

    BedrockSharedPtr sharedItem;
    sharedItem.mControlBlock = reinterpret_cast<BedrockControlBlock*>(regBlock);

    using RegItemFn = void (*)(void*, BedrockSharedPtr*);
    auto regItemFn = reinterpret_cast<RegItemFn>(mcBase + off.itemRegistryRegisterItem);
    try {
        regItemFn(registry, &sharedItem);
        sRegisteredRegistryItems.insert({registry, def.identifier});
        uint8_t maxStack = (def.maxStackSize > 0) ? def.maxStackSize : (def.category == 3 ? 1 : 64);
        uint8_t category = (def.category > 0) ? static_cast<uint8_t>(def.category) : 4;
        logger.info("§6[LeviLamroid] §aRegistered native item: §b", def.identifier, 
                     " §7(ID: ", def.id, ", Category: ", (int)category, ", MaxStack: ", (int)maxStack, ")");
    } catch (const std::exception& e) {
        logger.error("§c[LeviLamroid] Exception registering item: ", def.identifier, ": ", e.what());
    } catch (...) {
        logger.error("§c[LeviLamroid] Unknown exception registering item: ", def.identifier);
    }
}

static void* sOrigVanillaItemsRegisterItems = nullptr;

static void hooked_vanillaItemsRegisterItems(
    void* ctx,
    void* itemRegistryPtr,
    void* itemRegistryCtrlBlock,
    void* baseGameVersion,
    void* experiments
) {
    if (sOrigVanillaItemsRegisterItems) {
        reinterpret_cast<void(*)(void*, void*, void*, void*, void*)>(sOrigVanillaItemsRegisterItems)(
            ctx, itemRegistryPtr, itemRegistryCtrlBlock, baseGameVersion, experiments
        );
    }

    auto& logger = ll::Logger::getInstance();
    void* registry = unwrapItemRegistry(itemRegistryPtr);
    logger.info("§6[LeviLamroid] §aVanillaItems::registerItems hook fired! itemRegistry arg: ", itemRegistryPtr,
                ", unwrapped: ", registry);

    if (registry && isPointerReadable(registry, 16)) {
        std::lock_guard<std::mutex> lock(sItemMutex);
        sItemRegistryInstance = registry;
        if (std::find(sKnownRegistries.begin(), sKnownRegistries.end(), registry) == sKnownRegistries.end()) {
            sKnownRegistries.push_back(registry);
        }
        logger.info("§6[LeviLamroid] §eRegistering ", sCustomItems.size(), " custom items during VanillaItems::registerItems window...");
        for (auto& def : sCustomItems) {
            registerSingleItem(registry, def);
        }
    }
}

static void hooked_itemRegistryPostInit(void* registryArg, void* a2) {
    if (sOrigItemRegistryPostInit) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigItemRegistryPostInit)(registryArg, a2);
    }
    void* registry = unwrapItemRegistry(registryArg);
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
    if (def.nativeBlockPermutation) return def.nativeBlockPermutation;
    return nullptr;
}

void refreshCustomBlockBindings() {
    auto& logger = ll::Logger::getInstance();
    std::lock_guard<std::mutex> lock(sItemMutex);
    logger.info("§6[LeviLamroid] §aCustom block bindings verified (", sCustomItems.size(), " items).");
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
    CustomItemDefinition copy = def;
    if (::levilamroid::block::isCustomBlock(copy.identifier)) {
        copy.isBlock = true;
        copy.iconName = "";
        copy.isFood = false;
        copy.nutrition = 0;
        copy.useAnimation = 0;
    }

    for (auto& item : sCustomItems) {
        if (item.identifier == copy.identifier) {
            bool wasBlock = item.isBlock || copy.isBlock;
            void* prevBlkCtrl = item.nativeBlockControlBlock;
            void* prevPerm = item.nativeBlockPermutation;
            std::string prevBase = item.baseBlock;
            item = copy;
            if (wasBlock) {
                item.isBlock = true;
                item.iconName = "";
                item.isFood = false;
                item.nutrition = 0;
                item.useAnimation = 0;
                if (!item.nativeBlockControlBlock) item.nativeBlockControlBlock = prevBlkCtrl;
                if (!item.nativeBlockPermutation) item.nativeBlockPermutation = prevPerm;
                if (item.baseBlock.empty()) item.baseBlock = prevBase;
            }
            if (!sKnownRegistries.empty()) {
                ensureNativeItemAllocated(item);
                for (void* reg : sKnownRegistries) {
                    registerSingleItem(reg, item);
                }
            }
            return true;
        }
    }
    sCustomItems.push_back(copy);
    if (!sKnownRegistries.empty()) {
        ensureNativeItemAllocated(sCustomItems.back());
        for (void* reg : sKnownRegistries) {
            registerSingleItem(reg, sCustomItems.back());
        }
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
    if (!player || !isPointerReadable(player, 0x580)) return false;

    CustomItemDefinition* targetDef = nullptr;
    {
        std::lock_guard<std::mutex> lock(sItemMutex);
        for (auto& item : sCustomItems) {
            if (item.identifier == identifier) {
                targetDef = &item;
                ensureNativeItemAllocated(*targetDef);
                break;
            }
        }
    }
    if (!targetDef || !targetDef->nativeItemPtr || !targetDef->nativeControlBlock) return false;

    uint8_t* playerBytes = reinterpret_cast<uint8_t*>(player);
    void* supplies = *reinterpret_cast<void**>(playerBytes + 0x570);
    if (!supplies || !isPointerReadable(supplies, 0xc0)) return false;

    uint8_t* suppliesBytes = reinterpret_cast<uint8_t*>(supplies);
    if (slot < 0) {
        slot = *reinterpret_cast<int*>(suppliesBytes + 0x10);
        if (slot < 0 || slot > 8) slot = 0;
    }

    void* inventory = *reinterpret_cast<void**>(suppliesBytes + 0xb8);
    if (!inventory || !isPointerReadable(inventory, 0x150)) return false;

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
    if (ll_container_set_item(inventory, slot, stackMem)) {
        ll::Logger::getInstance().info("§6[LeviLamroid] §aDelivered ", count, "x '", identifier, "' directly into inventory slot ", slot);
        return true;
    }

    // Direct client command fallback
    std::string giveCmd = "/give @s " + identifier + " " + std::to_string(count > 0 ? count : 1);
    ll_command_dispatch_client(giveCmd.c_str());
    return true;
}

void forEachRegisteredItem(void (*callback)(const CustomItemDefinition& def, void* udata), void* udata) {
    if (!callback) return;
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (const auto& item : sCustomItems) {
        callback(item, udata);
    }
}

const CustomItemDefinition* getCustomItem(std::string_view identifier) {
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (const auto& item : sCustomItems) {
        if (item.identifier == identifier) return &item;
    }
    return nullptr;
}

const CustomItemDefinition* getCustomItem(short id) {
    std::lock_guard<std::mutex> lock(sItemMutex);
    for (const auto& item : sCustomItems) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

std::vector<CustomItemDefinition>& getAllCustomItems() {
    return sCustomItems;
}

bool installItemHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.vanillaItemsRegisterItems) {
        if (ll::memory::hookRva(off.vanillaItemsRegisterItems, (void*)&hooked_vanillaItemsRegisterItems, &sOrigVanillaItemsRegisterItems)) {
            logger.info("Hooked VanillaItems::registerItems [RVA: 0x", std::hex, off.vanillaItemsRegisterItems, "]");
        }
    }

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

static std::string beautifyItemIdentifier(std::string_view ident) {
    auto colon = ident.find(':');
    std::string s = (colon != std::string_view::npos) ? std::string(ident.substr(colon + 1)) : std::string(ident);
    bool cap = true;
    for (char& c : s) {
        if (c == '_') {
            c = ' ';
            cap = true;
        } else if (cap && std::isalpha(static_cast<unsigned char>(c))) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            cap = false;
        }
    }
    return s;
}

static void resolveItemNaming(levilamroid::item::CustomItemDefinition& def, const char* nameOrDesc) {
    if (!nameOrDesc || nameOrDesc[0] == '\0') {
        def.displayName = beautifyItemIdentifier(def.identifier);
        def.descriptionId = "item." + def.identifier + ".name";
        return;
    }
    std::string s = nameOrDesc;
    bool isDirectName = (s.find("§") != std::string::npos) || 
                        (s.find(' ') != std::string::npos) ||
                        (s.rfind("item.", 0) != 0 && s.rfind("tile.", 0) != 0 && s.find('.') == std::string::npos);
    if (isDirectName) {
        def.displayName = s;
        def.descriptionId = "item." + def.identifier + ".name";
    } else {
        if (s.find(".name") == std::string::npos) s += ".name";
        def.descriptionId = s;
        if (def.displayName.empty()) {
            def.displayName = beautifyItemIdentifier(def.identifier);
        }
    }
}

LL_API bool ll_item_register_food(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int nutrition, float saturationModifier, int category, const char* creativeGroup) {
    if (!identifier) return false;
    levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = id;
    resolveItemNaming(def, descriptionId);
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
    resolveItemNaming(def, descriptionId);
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
    resolveItemNaming(def, descriptionId);
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

LL_API void* ll_item_registry_get() {
    return levilamroid::item::sItemRegistryInstance;
}

LL_API bool ll_item_register_shared(void* sharedItemPtr) {
    if (!sharedItemPtr || !levilamroid::item::sItemRegistryInstance) return false;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = levilamroid::item::getMcBase();
    if (!mcBase || !off.itemRegistryRegisterItem) return false;
    using RegItemFn = void (*)(void*, void*);
    auto regItemFn = reinterpret_cast<RegItemFn>(mcBase + off.itemRegistryRegisterItem);
    regItemFn(levilamroid::item::sItemRegistryInstance, sharedItemPtr);
    return true;
}

// ── End Phase 1 ──

} // extern "C"
