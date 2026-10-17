#pragma once
#include <string_view>

namespace levilamroid::block::groups {

// Construction Block Groups
inline constexpr std::string_view ConstructionMisc    = "itemGroup.name.constructionMisc";
inline constexpr std::string_view Stone               = "itemGroup.name.stone";
inline constexpr std::string_view Cobblestone         = "itemGroup.name.cobblestone";
inline constexpr std::string_view StoneBrick          = "itemGroup.name.stoneBrick";
inline constexpr std::string_view Sandstone           = "itemGroup.name.sandstone";
inline constexpr std::string_view Planks              = "itemGroup.name.planks";
inline constexpr std::string_view Wood                = "itemGroup.name.wood";
inline constexpr std::string_view Brick               = "itemGroup.name.brick";
inline constexpr std::string_view Concrete            = "itemGroup.name.concrete";
inline constexpr std::string_view ConcretePowder      = "itemGroup.name.concretePowder";
inline constexpr std::string_view StainedClay         = "itemGroup.name.stainedClay";
inline constexpr std::string_view GlazedTerracotta    = "itemGroup.name.glazedTerracotta";
inline constexpr std::string_view Walls               = "itemGroup.name.walls";
inline constexpr std::string_view Stairs              = "itemGroup.name.stairs";
inline constexpr std::string_view Slab                = "itemGroup.name.slab";
inline constexpr std::string_view Fence               = "itemGroup.name.fence";
inline constexpr std::string_view FenceGate           = "itemGroup.name.fenceGate";
inline constexpr std::string_view Glass               = "itemGroup.name.glass";
inline constexpr std::string_view GlassPane           = "itemGroup.name.glassPane";
inline constexpr std::string_view Door                = "itemGroup.name.door";
inline constexpr std::string_view Trapdoor            = "itemGroup.name.trapdoor";
inline constexpr std::string_view Copper              = "itemGroup.name.copper";
inline constexpr std::string_view CopperLights        = "itemGroup.name.copper_lights";

// Nature Block Groups
inline constexpr std::string_view NatureMisc          = "itemGroup.name.natureMisc";
inline constexpr std::string_view NatureBuilding      = "itemGroup.name.natureBuildingBlocks";
inline constexpr std::string_view Dirt                = "itemGroup.name.dirt";
inline constexpr std::string_view Grass               = "itemGroup.name.grass";
inline constexpr std::string_view Sand                = "itemGroup.name.sand";
inline constexpr std::string_view Gravel              = "itemGroup.name.gravel";
inline constexpr std::string_view Clay                = "itemGroup.name.clay";
inline constexpr std::string_view Ice                 = "itemGroup.name.ice";
inline constexpr std::string_view Log                 = "itemGroup.name.log";
inline constexpr std::string_view Leaves              = "itemGroup.name.leaves";
inline constexpr std::string_view Sapling             = "itemGroup.name.sapling";
inline constexpr std::string_view Flower              = "itemGroup.name.flower";
inline constexpr std::string_view Mushroom            = "itemGroup.name.mushroom";
inline constexpr std::string_view Crop                = "itemGroup.name.crop";
inline constexpr std::string_view Greenery            = "itemGroup.name.greenery";
inline constexpr std::string_view Cactus              = "itemGroup.name.cactus";
inline constexpr std::string_view Coral               = "itemGroup.name.coral";
inline constexpr std::string_view CoralDecorations    = "itemGroup.name.coral_decorations";
inline constexpr std::string_view Ore                 = "itemGroup.name.ore";
inline constexpr std::string_view OreBlocks           = "itemGroup.name.oreBlocks";

// Functional & Utility Block Groups
inline constexpr std::string_view CraftingTables      = "itemGroup.name.craftingTables";
inline constexpr std::string_view Furnaces            = "itemGroup.name.furnaces";
inline constexpr std::string_view Chest               = "itemGroup.name.chest";
inline constexpr std::string_view ShulkerBox          = "itemGroup.name.shulkerBox";
inline constexpr std::string_view EnchantingTable     = "itemGroup.name.enchantingTable";
inline constexpr std::string_view Beacon              = "itemGroup.name.beacon";
inline constexpr std::string_view Bell                = "itemGroup.name.bell";
inline constexpr std::string_view Lectern             = "itemGroup.name.lectern";
inline constexpr std::string_view ChiseledBookshelf   = "itemGroup.name.chiseledBookshelf";
inline constexpr std::string_view Composter           = "itemGroup.name.composter";
inline constexpr std::string_view Grindstone          = "itemGroup.name.grindStone";
inline constexpr std::string_view Shelf               = "itemGroup.name.shelf";
inline constexpr std::string_view Sign                = "itemGroup.name.sign";
inline constexpr std::string_view Banner              = "itemGroup.name.banner";

// Redstone & Mechanism Block Groups
inline constexpr std::string_view Redstone            = "itemGroup.name.redstone";
inline constexpr std::string_view RedstoneProducers   = "itemGroup.name.redstoneProducers";
inline constexpr std::string_view RedstoneContainers  = "itemGroup.name.redstoneContainers";
inline constexpr std::string_view Buttons             = "itemGroup.name.buttons";
inline constexpr std::string_view PressurePlate       = "itemGroup.name.pressurePlate";
inline constexpr std::string_view Pistons             = "itemGroup.name.pistons";
inline constexpr std::string_view Rail                = "itemGroup.name.rail";
inline constexpr std::string_view TNT                 = "itemGroup.name.tnt";

// Lighting Block Groups
inline constexpr std::string_view Torch               = "itemGroup.name.torch";
inline constexpr std::string_view Lights              = "itemGroup.name.lights";
inline constexpr std::string_view Glowstone           = "itemGroup.name.glowstone";
inline constexpr std::string_view EndRod              = "itemGroup.name.endRod";

// Wool & Decorative Block Groups
inline constexpr std::string_view Wool                = "itemGroup.name.wool";
inline constexpr std::string_view WoolCarpet          = "itemGroup.name.woolCarpet";
inline constexpr std::string_view Bars                = "itemGroup.name.bars";
inline constexpr std::string_view Chains              = "itemGroup.name.chains";

} // namespace levilamroid::block::groups

namespace ll::block::groups {
    using namespace ::levilamroid::block::groups;
}
