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
    std::string identifier;                            // e.g. "levitest:cosmic_blade"
    short id{0};                                       // e.g. 10500
    std::string descriptionId;                         // e.g. "item.levitest:cosmic_blade"
    std::string iconName;                              // e.g. "cosmic_blade"
    int iconFrame{0};                                  // e.g. 0
    int category{static_cast<int>(CreativeItemCategory::Items)}; // Equipment=3, Items=4
    std::string creativeGroup;                         // e.g. "itemGroup.name.sword"
    int groupIndex{0};                                 // Legacy group index
    bool allowOffhand{true};
    uint8_t maxStackSize{64};                          // 1 for weapons, 64 for materials
    bool isBlock{false};                               // True for blocks (uses BlockItem vtable & atlas.terrain)
    bool isHandEquipped{false};
    short maxDamage{0};
    void* nativeItemPtr{nullptr};
    void* nativeControlBlock{nullptr};
};

LL_API bool registerCustomItem(const CustomItemDefinition& def);
LL_API Item* getNativeItem(const std::string& identifier);
LL_API Item* getNativeItem(short id);
LL_API bool giveItemToPlayer(void* player, const std::string& identifier, int count = 1, int slot = -1);
LL_API void forEachRegisteredItem(void (*callback)(const CustomItemDefinition& def, void* udata), void* udata);

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
LL_API void* ll_item_get_native(const char* identifier);
LL_API void* ll_item_get_by_id(short id);
LL_API bool ll_item_give_player(void* player, const char* identifier, int count, int slot);
LL_API void ll_item_set_category(void* item, int category);
LL_API void ll_item_set_group(void* item, const char* group);
LL_API void ll_item_set_max_stack(void* item, uint8_t maxStack);
LL_API void ll_item_set_offhand_allowed(void* item, bool allowed);
LL_API void ll_item_set_icon(void* item, const char* iconName, int iconFrame);
LL_API const char* ll_item_group_get_name(int groupEnum);
}
