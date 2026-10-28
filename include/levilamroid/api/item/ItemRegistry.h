#pragma once
#include "ll/api/Macros.h"
#include "levilamroid/api/item/Item.h"
#include "levilamroid/api/item/ItemStack.h"
#include "levilamroid/api/item/CreativeItemCategory.h"
#include "levilamroid/api/item/CreativeGroup.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace levilamroid::item {

// Backward compatible alias
using CreativeCategory = CreativeItemCategory;

struct CustomItemDefinition {
    std::string identifier;                            // e.g. "custom:ruby_sword"
    short id{0};                                       // e.g. 10500
    std::string displayName;                           // e.g. "Ruby Sword"
    std::string descriptionId;                         // e.g. "item.custom:ruby_sword.name"
    std::string iconName;                              // e.g. "ruby_sword"
    int iconFrame{0};                                  // e.g. 0
    int category{static_cast<int>(CreativeItemCategory::Items)}; // Equipment=3, Items=4
    std::string creativeGroup;                         // e.g. "itemGroup.name.sword"
    int groupIndex{0};                                 // Legacy group index
    bool allowOffhand{true};
    uint8_t maxStackSize{64};                          // 1 for weapons, 64 for materials
    bool isBlock{false};                               // True for blocks (uses BlockItem vtable & atlas.terrain)
    bool isHandEquipped{false};
    bool isFood{false};
    bool canAlwaysEat{false};                          // True if consumable when hunger bar is full
    short maxDamage{0};

    // ── Phase 1: Food / Tool / Weapon properties ──
    int nutrition{0};                                  // Hunger points restored (0 = not food)
    float saturationModifier{0.6f};                    // Saturation modifier (0.6 = normal food)
    int useAnimation{0};                               // 0=none, 1=eat, 2=bow, 3=spear, 4=crossbow, 5=spyglass
    int useDuration{0};                                // Use duration in ticks (32=food, 72000=bow)
    float attackDamage{0.0f};                          // Bonus attack damage for weapons/tools
    int enchantSlot{0};                                // Enchantment slot bitmask
    float miningSpeed{1.0f};                           // Mining speed multiplier
    std::string cooldownCategory{""};                  // Cooldown category/group identifier
    float cooldownDuration{0.0f};                      // Cooldown duration in seconds
    // ── End Phase 1 ──

    void* nativeItemPtr{nullptr};
    void* nativeControlBlock{nullptr};
    void* nativeBlockPermutation{nullptr};
    void* nativeBlockControlBlock{nullptr};
    void* nativeBlockLegacyPtr{nullptr};
    std::string baseBlock{""};
    std::string geometry{""};
};

LL_API bool registerCustomItem(const CustomItemDefinition& def);
LL_API Item* getNativeItem(const std::string& identifier);
LL_API Item* getNativeItem(short id);
LL_API bool giveItemToPlayer(void* player, const std::string& identifier, int count = 1, int slot = -1);
LL_API void forEachRegisteredItem(void (*callback)(const CustomItemDefinition& def, void* udata), void* udata);
LL_API void* getBlockPermutationForDef(CustomItemDefinition& def);
LL_API const CustomItemDefinition* getCustomItem(std::string_view identifier);
LL_API const CustomItemDefinition* getCustomItem(short id);
LL_API std::vector<CustomItemDefinition>& getAllCustomItems();
LL_API void refreshCustomBlockBindings();

class LL_API ItemRegistry {
public:
    static ItemRegistry& getInstance();

    Item* getItem(short id);
    Item* getItem(std::string_view identifier);

    bool registerItem(const CustomItemDefinition& def);
    bool registerItem(
        std::string_view identifier, 
        short id, 
        std::string_view name, 
        std::string_view icon, 
        CreativeItemCategory category = CreativeItemCategory::Items, 
        std::string_view group = "", 
        uint8_t maxStack = 64, 
        bool allowOffhand = true
    );
};

} // namespace levilamroid::item

namespace ll::item {
    using CustomItemDefinition = ::levilamroid::item::CustomItemDefinition;
    using CreativeCategory = ::levilamroid::item::CreativeCategory;
    using ItemRegistry = ::levilamroid::item::ItemRegistry;
    using ::levilamroid::item::registerCustomItem;
    using ::levilamroid::item::getNativeItem;
    using ::levilamroid::item::giveItemToPlayer;
    using ::levilamroid::item::forEachRegisteredItem;
}

extern "C" {
LL_API bool ll_item_register(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, bool allowOffhand);
LL_API bool ll_item_register_ex(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, bool allowOffhand, uint8_t maxStackSize);
LL_API bool ll_item_register_full(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int category, const char* creativeGroup, bool allowOffhand, uint8_t maxStackSize);

// ── Phase 1: Food / Tool / Weapon registration helpers ──
// Register a food item. Sets useAnimation=EAT, useDuration=32, maxStackSize=64 automatically.
LL_API bool ll_item_register_food(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, int nutrition, float saturationModifier, int category, const char* creativeGroup);
// Register a tool item. Sets isHandEquipped=true, maxStackSize=1 automatically.
LL_API bool ll_item_register_tool(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, float attackDamage, short durability, float miningSpeed, int category, const char* creativeGroup);
// Register a weapon item. Sets isHandEquipped=true, maxStackSize=1, useAnimation=none automatically.
LL_API bool ll_item_register_weapon(const char* identifier, short id, const char* descriptionId, const char* iconName, int iconFrame, float attackDamage, short durability, int category, const char* creativeGroup);
// ── End Phase 1 ──

LL_API void* ll_item_get_native(const char* identifier);
LL_API void* ll_item_get_by_id(short id);
LL_API bool ll_item_give_player(void* player, const char* identifier, int count, int slot);
LL_API void ll_item_set_category(void* item, int category);
LL_API void ll_item_set_group(void* item, const char* group);
LL_API void ll_item_set_max_stack(void* item, uint8_t maxStack);
LL_API void ll_item_set_offhand_allowed(void* item, bool allowed);
LL_API void ll_item_set_icon(void* item, const char* iconName, int iconFrame);
LL_API const char* ll_item_group_get_name(int groupEnum);

// Custom Item Event Listeners
typedef bool (*LLPlayerAttackCallback)(void* player, void* targetActor, const char* heldItemIdent, void* udata);
typedef bool (*LLItemUseCallback)(void* player, const char* itemIdent, void* itemStack, void* udata);

LL_API bool ll_player_register_attack_listener(LLPlayerAttackCallback callback, void* udata);
LL_API bool ll_item_register_use_listener(LLItemUseCallback callback, void* udata);

// Player Inventory & Held Item Queries
LL_API void* ll_player_get_selected_item_stack(void* player);
LL_API bool ll_player_get_held_item_identifier(void* player, char* outIdent, size_t maxLen);
LL_API void* ll_player_get_supplies(void* player);
LL_API void* ll_player_get_inventory(void* player);

// ItemStack Inspection & Mutation APIs
LL_API int ll_item_stack_get_count(const void* itemStack);
LL_API bool ll_item_stack_set_count(void* itemStack, uint8_t count);
LL_API short ll_item_stack_get_aux(const void* itemStack);
LL_API bool ll_item_stack_is_valid(const void* itemStack);
LL_API void* ll_item_stack_get_item(const void* itemStack);
}

