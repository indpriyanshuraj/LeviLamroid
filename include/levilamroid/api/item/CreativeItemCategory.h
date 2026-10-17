#pragma once
#include <cstdint>
#include <string_view>
#include <string>

namespace levilamroid::item {

enum class CreativeItemCategory : uint8_t {
    All          = 0,
    Construction = 1,
    Nature       = 2,
    Equipment    = 3,
    Items        = 4,
    Commands     = 5,
    None         = 6,
    Count        = 7
};

inline constexpr std::string_view getCategoryName(CreativeItemCategory category) noexcept {
    switch (category) {
        case CreativeItemCategory::All:          return "All";
        case CreativeItemCategory::Construction: return "Construction";
        case CreativeItemCategory::Nature:       return "Nature";
        case CreativeItemCategory::Equipment:    return "Equipment";
        case CreativeItemCategory::Items:        return "Items";
        case CreativeItemCategory::Commands:     return "Commands";
        case CreativeItemCategory::None:         return "None";
        default:                                 return "Unknown";
    }
}

inline constexpr CreativeItemCategory parseCategory(std::string_view name) noexcept {
    if (name == "Construction" || name == "construction" || name == "1") return CreativeItemCategory::Construction;
    if (name == "Nature" || name == "nature" || name == "2")             return CreativeItemCategory::Nature;
    if (name == "Equipment" || name == "equipment" || name == "3")       return CreativeItemCategory::Equipment;
    if (name == "Items" || name == "items" || name == "4")               return CreativeItemCategory::Items;
    if (name == "Commands" || name == "commands" || name == "5")         return CreativeItemCategory::Commands;
    if (name == "None" || name == "none" || name == "6")                 return CreativeItemCategory::None;
    return CreativeItemCategory::Items;
}

} // namespace levilamroid::item

namespace ll::item {
    using CreativeItemCategory = ::levilamroid::item::CreativeItemCategory;
    using ::levilamroid::item::getCategoryName;
    using ::levilamroid::item::parseCategory;
}
