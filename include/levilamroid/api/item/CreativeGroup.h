#pragma once
#include <string_view>
#include <string>

namespace levilamroid::item {

namespace groups {
    // Equipment & Weapons
    inline constexpr std::string_view Sword          = "itemGroup.name.sword";
    inline constexpr std::string_view Bow            = "itemGroup.name.bow";
    inline constexpr std::string_view Crossbow       = "itemGroup.name.crossbow";
    inline constexpr std::string_view Spear          = "itemGroup.name.spear";
    inline constexpr std::string_view Shield         = "itemGroup.name.shield";
    inline constexpr std::string_view Helmet         = "itemGroup.name.helmet";
    inline constexpr std::string_view ArmorHead      = "itemGroup.name.armor_head";
    inline constexpr std::string_view Chestplate     = "itemGroup.name.chestplate";
    inline constexpr std::string_view ArmorTorso     = "itemGroup.name.armor_torso";
    inline constexpr std::string_view Leggings       = "itemGroup.name.leggings";
    inline constexpr std::string_view ArmorLegs      = "itemGroup.name.armor_legs";
    inline constexpr std::string_view Boots          = "itemGroup.name.boots";
    inline constexpr std::string_view ArmorFeet      = "itemGroup.name.armor_feet";
    inline constexpr std::string_view Elytra         = "itemGroup.name.elytra";

    // Tools
    inline constexpr std::string_view Axe            = "itemGroup.name.axe";
    inline constexpr std::string_view Pickaxe        = "itemGroup.name.pickaxe";
    inline constexpr std::string_view Shovel         = "itemGroup.name.shovel";
    inline constexpr std::string_view Hoe            = "itemGroup.name.hoe";
    inline constexpr std::string_view Shears         = "itemGroup.name.shears";
    inline constexpr std::string_view FlintAndSteel  = "itemGroup.name.flintsteel";
    inline constexpr std::string_view FishingRod     = "itemGroup.name.fishing_rod";
    inline constexpr std::string_view Compass        = "itemGroup.name.compass";
    inline constexpr std::string_view Clock          = "itemGroup.name.clock";

    // Items & Consumables
    inline constexpr std::string_view Misc           = "itemGroup.name.misc";
    inline constexpr std::string_view Potion         = "itemGroup.name.potion";
    inline constexpr std::string_view SplashPotion   = "itemGroup.name.splashPotion";
    inline constexpr std::string_view LingeringPotion= "itemGroup.name.lingeringPotion";
    inline constexpr std::string_view Arrow          = "itemGroup.name.arrow";
    inline constexpr std::string_view Firework       = "itemGroup.name.firework";
    inline constexpr std::string_view MobEgg         = "itemGroup.name.mobEgg";
    inline constexpr std::string_view Dye            = "itemGroup.name.dye";
    inline constexpr std::string_view EnchantedBook  = "itemGroup.name.enchantedBook";
    inline constexpr std::string_view GoatHorn       = "itemGroup.name.goatHorn";
    inline constexpr std::string_view Banner         = "itemGroup.name.banner";
    inline constexpr std::string_view MusicDisc      = "itemGroup.name.record";

    // Construction & Blocks
    inline constexpr std::string_view Planks         = "itemGroup.name.planks";
    inline constexpr std::string_view Walls          = "itemGroup.name.walls";
    inline constexpr std::string_view Stairs         = "itemGroup.name.stairs";
    inline constexpr std::string_view Slab           = "itemGroup.name.slab";
    inline constexpr std::string_view Door           = "itemGroup.name.door";
    inline constexpr std::string_view Trapdoor       = "itemGroup.name.trapdoor";
    inline constexpr std::string_view Fence          = "itemGroup.name.fence";
    inline constexpr std::string_view FenceGate      = "itemGroup.name.fenceGate";
    inline constexpr std::string_view Glass          = "itemGroup.name.glass";
    inline constexpr std::string_view Bed            = "itemGroup.name.bed";
    inline constexpr std::string_view Chest          = "itemGroup.name.chest";
    inline constexpr std::string_view ShulkerBox     = "itemGroup.name.shulkerBox";
    inline constexpr std::string_view CraftingTables = "itemGroup.name.craftingTables";
    inline constexpr std::string_view Anvil          = "itemGroup.name.anvil";
    inline constexpr std::string_view Ore            = "itemGroup.name.ore";
    inline constexpr std::string_view Food           = "itemGroup.name.cookedFood";
    inline constexpr std::string_view RawFood        = "itemGroup.name.rawFood";
    inline constexpr std::string_view Crop           = "itemGroup.name.crop";
    inline constexpr std::string_view Wool           = "itemGroup.name.wool";
    inline constexpr std::string_view Concrete       = "itemGroup.name.concrete";
    inline constexpr std::string_view StainedClay    = "itemGroup.name.stainedClay";
    inline constexpr std::string_view Candles        = "itemGroup.name.candles";

    // Nature
    inline constexpr std::string_view Flower         = "itemGroup.name.flower";
    inline constexpr std::string_view Sapling        = "itemGroup.name.sapling";
    inline constexpr std::string_view Seed           = "itemGroup.name.seed";
    inline constexpr std::string_view Leaves         = "itemGroup.name.leaves";
    inline constexpr std::string_view Log            = "itemGroup.name.log";
    inline constexpr std::string_view Dirt           = "itemGroup.name.dirt";
    inline constexpr std::string_view Stone          = "itemGroup.name.stone";
    inline constexpr std::string_view Sand           = "itemGroup.name.sand";
    inline constexpr std::string_view Coral          = "itemGroup.name.coral";

    // Redstone
    inline constexpr std::string_view Buttons        = "itemGroup.name.buttons";
    inline constexpr std::string_view PressurePlate  = "itemGroup.name.pressurePlate";
    inline constexpr std::string_view Rail           = "itemGroup.name.rail";
    inline constexpr std::string_view Minecart       = "itemGroup.name.minecart";
    inline constexpr std::string_view CommandBlock   = "itemGroup.name.commandBlock";
} // namespace groups

enum class CreativeGroup : int {
    None = 0,
    // Weapons & Armor
    Sword,
    Bow,
    Crossbow,
    Spear,
    Shield,
    Helmet,
    Chestplate,
    Leggings,
    Boots,
    Elytra,

    // Tools
    Axe,
    Pickaxe,
    Shovel,
    Hoe,
    Shears,
    FlintAndSteel,
    FishingRod,
    Compass,
    Clock,

    // Items
    Misc,
    Potion,
    SplashPotion,
    LingeringPotion,
    Arrow,
    Firework,
    MobEgg,
    Dye,
    EnchantedBook,
    GoatHorn,
    Banner,
    MusicDisc,

    // Construction
    Planks,
    Walls,
    Stairs,
    Slab,
    Door,
    Trapdoor,
    Fence,
    FenceGate,
    Glass,
    Bed,
    Chest,
    ShulkerBox,
    CraftingTables,
    Anvil,
    Ore,
    Food,
    RawFood,
    Crop,
    Wool,
    Concrete,
    StainedClay,
    Candles,

    // Nature
    Flower,
    Sapling,
    Seed,
    Leaves,
    Log,
    Dirt,
    Stone,
    Sand,
    Coral,

    // Redstone
    Buttons,
    PressurePlate,
    Rail,
    Minecart,
    CommandBlock,

    Custom
};

inline constexpr std::string_view getGroupName(CreativeGroup group) noexcept {
    switch (group) {
        case CreativeGroup::Sword:          return groups::Sword;
        case CreativeGroup::Bow:            return groups::Bow;
        case CreativeGroup::Crossbow:       return groups::Crossbow;
        case CreativeGroup::Spear:          return groups::Spear;
        case CreativeGroup::Shield:         return groups::Shield;
        case CreativeGroup::Helmet:         return groups::Helmet;
        case CreativeGroup::Chestplate:     return groups::Chestplate;
        case CreativeGroup::Leggings:       return groups::Leggings;
        case CreativeGroup::Boots:          return groups::Boots;
        case CreativeGroup::Elytra:         return groups::Elytra;
        case CreativeGroup::Axe:            return groups::Axe;
        case CreativeGroup::Pickaxe:        return groups::Pickaxe;
        case CreativeGroup::Shovel:         return groups::Shovel;
        case CreativeGroup::Hoe:            return groups::Hoe;
        case CreativeGroup::Shears:         return groups::Shears;
        case CreativeGroup::FlintAndSteel:  return groups::FlintAndSteel;
        case CreativeGroup::FishingRod:     return groups::FishingRod;
        case CreativeGroup::Compass:        return groups::Compass;
        case CreativeGroup::Clock:          return groups::Clock;
        case CreativeGroup::Misc:           return groups::Misc;
        case CreativeGroup::Potion:         return groups::Potion;
        case CreativeGroup::SplashPotion:   return groups::SplashPotion;
        case CreativeGroup::LingeringPotion:return groups::LingeringPotion;
        case CreativeGroup::Arrow:          return groups::Arrow;
        case CreativeGroup::Firework:       return groups::Firework;
        case CreativeGroup::MobEgg:         return groups::MobEgg;
        case CreativeGroup::Dye:            return groups::Dye;
        case CreativeGroup::EnchantedBook:  return groups::EnchantedBook;
        case CreativeGroup::GoatHorn:       return groups::GoatHorn;
        case CreativeGroup::Banner:         return groups::Banner;
        case CreativeGroup::MusicDisc:      return groups::MusicDisc;
        case CreativeGroup::Planks:         return groups::Planks;
        case CreativeGroup::Walls:          return groups::Walls;
        case CreativeGroup::Stairs:         return groups::Stairs;
        case CreativeGroup::Slab:           return groups::Slab;
        case CreativeGroup::Door:           return groups::Door;
        case CreativeGroup::Trapdoor:       return groups::Trapdoor;
        case CreativeGroup::Fence:          return groups::Fence;
        case CreativeGroup::FenceGate:      return groups::FenceGate;
        case CreativeGroup::Glass:          return groups::Glass;
        case CreativeGroup::Bed:            return groups::Bed;
        case CreativeGroup::Chest:          return groups::Chest;
        case CreativeGroup::ShulkerBox:     return groups::ShulkerBox;
        case CreativeGroup::CraftingTables: return groups::CraftingTables;
        case CreativeGroup::Anvil:          return groups::Anvil;
        case CreativeGroup::Ore:            return groups::Ore;
        case CreativeGroup::Food:           return groups::Food;
        case CreativeGroup::RawFood:        return groups::RawFood;
        case CreativeGroup::Crop:           return groups::Crop;
        case CreativeGroup::Wool:           return groups::Wool;
        case CreativeGroup::Concrete:       return groups::Concrete;
        case CreativeGroup::StainedClay:    return groups::StainedClay;
        case CreativeGroup::Candles:        return groups::Candles;
        case CreativeGroup::Flower:         return groups::Flower;
        case CreativeGroup::Sapling:        return groups::Sapling;
        case CreativeGroup::Seed:           return groups::Seed;
        case CreativeGroup::Leaves:         return groups::Leaves;
        case CreativeGroup::Log:            return groups::Log;
        case CreativeGroup::Dirt:           return groups::Dirt;
        case CreativeGroup::Stone:          return groups::Stone;
        case CreativeGroup::Sand:           return groups::Sand;
        case CreativeGroup::Coral:          return groups::Coral;
        case CreativeGroup::Buttons:        return groups::Buttons;
        case CreativeGroup::PressurePlate:  return groups::PressurePlate;
        case CreativeGroup::Rail:           return groups::Rail;
        case CreativeGroup::Minecart:       return groups::Minecart;
        case CreativeGroup::CommandBlock:   return groups::CommandBlock;
        default:                            return groups::Misc;
    }
}

} // namespace levilamroid::item

namespace ll::item {
    using CreativeGroup = ::levilamroid::item::CreativeGroup;
    namespace groups = ::levilamroid::item::groups;
    using ::levilamroid::item::getGroupName;
}
