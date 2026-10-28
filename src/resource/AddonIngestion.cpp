#include "AddonIngestion.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "levilamroid/api/recipe/RecipeRegistry.h"
#include "levilamroid/api/geometry/GeometryRegistry.h"
#include "levilamroid/api/entity/EntityRegistry.h"
#include "levilamroid/api/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <nlohmann/json.hpp>

namespace levilamroid::addon {

static std::string stripJsonComments(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    bool inString = false;
    bool inSingleLineComment = false;
    bool inMultiLineComment = false;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        char next = (i + 1 < input.size()) ? input[i + 1] : '\0';

        if (inSingleLineComment) {
            if (c == '\n' || c == '\r') {
                inSingleLineComment = false;
                result += c;
            }
            continue;
        }

        if (inMultiLineComment) {
            if (c == '*' && next == '/') {
                inMultiLineComment = false;
                ++i;
            }
            continue;
        }

        if (inString) {
            result += c;
            if (c == '\\' && i + 1 < input.size()) {
                result += input[++i];
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (c == '"') {
            inString = true;
            result += c;
        } else if (c == '/' && next == '/') {
            inSingleLineComment = true;
            ++i;
        } else if (c == '/' && next == '*') {
            inMultiLineComment = true;
            ++i;
        } else {
            result += c;
        }
    }
    return result;
}

static short sNextAutoItemId = 10500;
static std::set<std::string> sIngestedItems;
static std::set<std::string> sIngestedBlocks;
static std::set<std::string> sIngestedRecipes;
static std::set<std::string> sIngestedGeometries;
static std::set<std::string> sIngestedEntities;

static void ingestItemJson(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string clean = stripJsonComments(rawContent);

    nlohmann::json root = nlohmann::json::parse(clean, nullptr, false, true);
    if (root.is_discarded() || !root.contains("minecraft:item")) return;

    const auto& itemObj = root["minecraft:item"];
    if (!itemObj.contains("description") || !itemObj["description"].contains("identifier")) return;

    std::string identifier = itemObj["description"]["identifier"].get<std::string>();
    if (identifier.empty()) return;
    if (sIngestedItems.count(identifier)) return;
    sIngestedItems.insert(identifier);

    auto& logger = ll::Logger::getInstance();

    int category = 4; // Default: Items (4)
    std::string group = "";
    bool isCategoryNone = false;

    if (itemObj["description"].contains("menu_category") && itemObj["description"]["menu_category"].is_object()) {
        const auto& mc = itemObj["description"]["menu_category"];
        if (mc.contains("category") && mc["category"].is_string()) {
            std::string c = mc["category"].get<std::string>();
            std::transform(c.begin(), c.end(), c.begin(), [](unsigned char ch){ return std::tolower(ch); });
            if (c == "none") { isCategoryNone = true; category = 0; }
            else if (c == "construction") category = 1;
            else if (c == "nature") category = 2;
            else if (c == "equipment") category = 3;
            else if (c == "items") category = 4;
        }
        if (mc.contains("group") && mc["group"].is_string()) {
            group = mc["group"].get<std::string>();
        }
    } else if (itemObj["description"].contains("category") && itemObj["description"]["category"].is_string()) {
        std::string c = itemObj["description"]["category"].get<std::string>();
        std::transform(c.begin(), c.end(), c.begin(), [](unsigned char ch){ return std::tolower(ch); });
        if (c == "none") { isCategoryNone = true; category = 0; }
        else if (c == "construction") category = 1;
        else if (c == "nature") category = 2;
        else if (c == "equipment") category = 3;
        else if (c == "items") category = 4;
    }

    // Skip internal animation dummies, progress indicators, and category:none items
    if (isCategoryNone || category == 0 ||
        identifier.find("fire_") != std::string::npos ||
        identifier.find("arrow") != std::string::npos ||
        identifier.find("progress") != std::string::npos) {
        return;
    }

    // Guard: Never use itemGroup.name.mobEgg for generic Item instances, as Bedrock downcasts
    // items in this group to SpawnEggItem and crashes calling pure virtuals.
    if (group == "itemGroup.name.mobEgg" || identifier.find("spawn_egg") != std::string::npos) {
        group = "itemGroup.name.misc";
        if (category == 2) category = 4; // Place under Items tab safely
    } else if (group.find("cooker") != std::string::npos || group.find("kitchen") != std::string::npos) {
        group = "itemGroup.name.misc";
    } else if (group.find("crop") != std::string::npos) {
        group = "itemGroup.name.crop";
    } else if (group.find(':') != std::string::npos) {
        group = (category == 3) ? "itemGroup.name.sword" : "itemGroup.name.misc";
    }

    std::string iconName = "";
    int iconFrame = 0;
    float attackDamage = 0.0f;
    short maxDamage = 0;
    bool isFood = false;
    bool canAlwaysEat = false;
    int nutrition = 0;
    float saturationModifier = 0.6f;
    int useAnimation = 0;
    int useDuration = 0;
    uint8_t maxStackSize = 0;
    bool isHandEquipped = false;
    bool allowOffhand = true;
    std::string baseBlock = "";
    bool hasBlockComponent = false;
    std::string cooldownCategory = "";
    float cooldownDuration = 0.0f;

    if (itemObj.contains("components") && itemObj["components"].is_object()) {
        const auto& comps = itemObj["components"];

        // Icon
        if (comps.contains("minecraft:icon")) {
            const auto& iconComp = comps["minecraft:icon"];
            if (iconComp.is_string()) {
                iconName = iconComp.get<std::string>();
            } else if (iconComp.is_object()) {
                if (iconComp.contains("textures")) {
                    const auto& tex = iconComp["textures"];
                    if (tex.is_string()) {
                        iconName = tex.get<std::string>();
                    } else if (tex.is_object()) {
                        if (tex.contains("default") && tex["default"].is_string()) {
                            iconName = tex["default"].get<std::string>();
                        } else if (!tex.empty() && tex.begin().value().is_string()) {
                            iconName = tex.begin().value().get<std::string>();
                        }
                    }
                } else if (iconComp.contains("texture") && iconComp["texture"].is_string()) {
                    iconName = iconComp["texture"].get<std::string>();
                }
                if (iconComp.contains("frame") && iconComp["frame"].is_number()) {
                    iconFrame = iconComp["frame"].get<int>();
                }
            }
        }

        // Damage (Bonus attack damage for sword/weapon/tool)
        if (comps.contains("minecraft:damage") && comps["minecraft:damage"].is_number()) {
            attackDamage = comps["minecraft:damage"].get<float>();
        }

        // Durability
        if (comps.contains("minecraft:durability")) {
            if (comps["minecraft:durability"].is_number()) {
                maxDamage = static_cast<short>(comps["minecraft:durability"].get<int>());
            } else if (comps["minecraft:durability"].is_object() && comps["minecraft:durability"].contains("max_durability")) {
                maxDamage = static_cast<short>(comps["minecraft:durability"]["max_durability"].get<int>());
            }
        }

        // Food
        if (comps.contains("minecraft:food") && comps["minecraft:food"].is_object()) {
            isFood = true;
            const auto& food = comps["minecraft:food"];
            if (food.contains("nutrition") && food["nutrition"].is_number()) {
                nutrition = food["nutrition"].get<int>();
            }
            if (food.contains("can_always_eat") && food["can_always_eat"].is_boolean()) {
                canAlwaysEat = food["can_always_eat"].get<bool>();
            }
            if (food.contains("saturation_modifier")) {
                if (food["saturation_modifier"].is_number()) {
                    saturationModifier = food["saturation_modifier"].get<float>();
                } else if (food["saturation_modifier"].is_string()) {
                    std::string sm = food["saturation_modifier"].get<std::string>();
                    if (sm == "supernatural") saturationModifier = 1.2f;
                    else if (sm == "good") saturationModifier = 0.8f;
                    else if (sm == "normal") saturationModifier = 0.6f;
                    else if (sm == "low") saturationModifier = 0.2f;
                    else if (sm == "poor") saturationModifier = 0.1f;
                }
            }
            useAnimation = 1; // EAT
            useDuration = 32;
        }

        // Use modifiers
        if (comps.contains("minecraft:use_modifiers") && comps["minecraft:use_modifiers"].is_object()) {
            const auto& um = comps["minecraft:use_modifiers"];
            if (um.contains("use_duration") && um["use_duration"].is_number()) {
                float sec = um["use_duration"].get<float>();
                useDuration = static_cast<int>(sec * 20.0f);
            }
        }

        // Use animation
        if (comps.contains("minecraft:use_animation")) {
            if (comps["minecraft:use_animation"].is_number()) {
                useAnimation = comps["minecraft:use_animation"].get<int>();
            } else if (comps["minecraft:use_animation"].is_string()) {
                std::string anim = comps["minecraft:use_animation"].get<std::string>();
                if (anim == "eat") useAnimation = 1;
                else if (anim == "drink") useAnimation = 2;
                else if (anim == "bow") useAnimation = 4;
                else if (anim == "camera") useAnimation = 5;
            }
        }

        // Cooldown
        if (comps.contains("minecraft:cooldown") && comps["minecraft:cooldown"].is_object()) {
            const auto& cd = comps["minecraft:cooldown"];
            if (cd.contains("duration") && cd["duration"].is_number()) {
                cooldownDuration = cd["duration"].get<float>();
            }
            if (cd.contains("category") && cd["category"].is_string()) {
                cooldownCategory = cd["category"].get<std::string>();
            }
        }

        // Max stack size
        if (comps.contains("minecraft:max_stack_size") && comps["minecraft:max_stack_size"].is_number()) {
            maxStackSize = static_cast<uint8_t>(comps["minecraft:max_stack_size"].get<int>());
        }

        // Hand equipped
        if (comps.contains("minecraft:hand_equipped") && comps["minecraft:hand_equipped"].is_boolean()) {
            isHandEquipped = comps["minecraft:hand_equipped"].get<bool>();
        }

        // Offhand
        if (comps.contains("minecraft:allow_off_hand") && comps["minecraft:allow_off_hand"].is_boolean()) {
            allowOffhand = comps["minecraft:allow_off_hand"].get<bool>();
        }

        // Block Placer component
        if (comps.contains("minecraft:block_placer") && comps["minecraft:block_placer"].is_object()) {
            if (comps["minecraft:block_placer"].contains("block") && comps["minecraft:block_placer"]["block"].is_string()) {
                baseBlock = comps["minecraft:block_placer"]["block"].get<std::string>();
            }
        }
        if (comps.contains("minecraft:block_placer") || comps.contains("minecraft:block")) {
            hasBlockComponent = true;
        }
    }

    std::string displayName = "";
    if (itemObj.contains("components") && itemObj["components"].is_object()) {
        const auto& comps = itemObj["components"];
        if (comps.contains("minecraft:display_name")) {
            if (comps["minecraft:display_name"].is_object() && comps["minecraft:display_name"].contains("value") && comps["minecraft:display_name"]["value"].is_string()) {
                displayName = comps["minecraft:display_name"]["value"].get<std::string>();
            } else if (comps["minecraft:display_name"].is_string()) {
                displayName = comps["minecraft:display_name"].get<std::string>();
            }
        }
    }
    if (displayName.empty()) {
        auto colon = identifier.find(':');
        std::string s = (colon != std::string::npos) ? identifier.substr(colon + 1) : identifier;
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
        displayName = s;
    }

    if (maxStackSize == 0) {
        maxStackSize = (category == 3 || maxDamage > 0 || attackDamage > 0) ? 1 : 64;
    }

    if (category == 3 || attackDamage > 0) {
        isHandEquipped = true;
    }

    // Determine ID
    short itemId = 0;
    auto& allItems = ::levilamroid::item::getAllCustomItems();
    for (const auto& it : allItems) {
        if (it.identifier == identifier) {
            itemId = it.id;
            break;
        }
    }
    if (itemId == 0) {
        itemId = sNextAutoItemId++;
    }

    if (identifier.find("arrow") != std::string::npos || identifier.find("progress") != std::string::npos ||
        identifier.find("fire_0") != std::string::npos || identifier.find("fire_1") != std::string::npos) {
        return;
    }

    bool isBlock = false;
    if (!baseBlock.empty() || hasBlockComponent) {
        isBlock = true;
    } else if (sIngestedBlocks.count(identifier) || ::levilamroid::block::isCustomBlock(identifier) ||
               identifier.find("_block") != std::string::npos) {
        isBlock = true;
    }
    if (isBlock && baseBlock.empty()) {
        if (identifier.find("table") != std::string::npos || identifier.find("crafting_table") != std::string::npos ||
            identifier.find("cutting_board") != std::string::npos || identifier.find("board") != std::string::npos) {
            baseBlock = "minecraft:crafting_table";
        } else if (identifier.find("rug") != std::string::npos || identifier.find("carpet") != std::string::npos) {
            baseBlock = "minecraft:white_carpet";
        } else if (identifier.find("tatami") != std::string::npos || identifier.find("mat") != std::string::npos || identifier.find("slab") != std::string::npos) {
            baseBlock = "minecraft:wooden_slab";
        } else if (identifier.find("pot") != std::string::npos) {
            baseBlock = "minecraft:flower_pot";
        } else if (identifier.find("stove") != std::string::npos || identifier.find("furnace") != std::string::npos) {
            baseBlock = "minecraft:furnace";
        } else if (identifier.find("cabinet") != std::string::npos || identifier.find("crate") != std::string::npos || identifier.find("basket") != std::string::npos) {
            baseBlock = "minecraft:barrel";
        } else if (identifier.find("soil") != std::string::npos || identifier.find("farmland") != std::string::npos) {
            baseBlock = "minecraft:farmland";
        } else {
            baseBlock = "minecraft:stone";
        }
    }

    if (isBlock) {
        isHandEquipped = false;
        isFood = false;
        nutrition = 0;
        useAnimation = 0;
        iconName = ""; // Empty icon allows Bedrock to render authentic 3D block cube in hand & inventory
    } else if (iconName.empty()) {
        auto colon = identifier.find(':');
        iconName = (colon != std::string::npos) ? identifier.substr(colon + 1) : identifier;
    }

    ::levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = itemId;
    def.displayName = displayName;
    def.descriptionId = (isBlock ? "tile." : "item.") + identifier + ".name";
    def.iconName = iconName;
    def.iconFrame = iconFrame;
    def.category = category;
    def.creativeGroup = group;
    def.allowOffhand = allowOffhand;
    def.maxStackSize = maxStackSize;
    def.isHandEquipped = isHandEquipped;
    def.isFood = isFood;
    def.canAlwaysEat = canAlwaysEat;
    def.nutrition = nutrition;
    def.saturationModifier = saturationModifier;
    def.useAnimation = useAnimation;
    def.useDuration = useDuration;
    def.attackDamage = attackDamage;
    def.maxDamage = maxDamage;
    def.isBlock = isBlock;
    def.baseBlock = baseBlock;
    def.cooldownCategory = cooldownCategory;
    def.cooldownDuration = cooldownDuration;

    sIngestedItems.insert(identifier);
    ::levilamroid::item::registerCustomItem(def);
    logger.info("§6[AddonIngest] §aIndexed item JSON for native engine loading: §b", identifier,
                " §7(Cat: ", category, ", Name: '", displayName, "', Icon: '", iconName, "', Stack: ", (int)maxStackSize, ")");
}

static void ingestBlockJson(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string clean = stripJsonComments(rawContent);

    nlohmann::json root = nlohmann::json::parse(clean, nullptr, false, true);
    if (root.is_discarded() || !root.contains("minecraft:block")) return;

    const auto& blockObj = root["minecraft:block"];
    if (!blockObj.contains("description") || !blockObj["description"].contains("identifier")) return;

    std::string identifier = blockObj["description"]["identifier"].get<std::string>();
    if (identifier.empty()) return;
    if (sIngestedBlocks.count(identifier)) return;
    sIngestedBlocks.insert(identifier);

    auto& logger = ll::Logger::getInstance();

    int category = 1; // Default: Construction (1)
    std::string group = "itemGroup.name.stone";

    if (blockObj["description"].contains("menu_category") && blockObj["description"]["menu_category"].is_object()) {
        const auto& mc = blockObj["description"]["menu_category"];
        if (mc.contains("category") && mc["category"].is_string()) {
            std::string c = mc["category"].get<std::string>();
            std::transform(c.begin(), c.end(), c.begin(), [](unsigned char ch){ return std::tolower(ch); });
            if (c == "construction") category = 1;
            else if (c == "nature") category = 2;
            else if (c == "equipment") category = 3;
            else if (c == "items") category = 4;
        }
        if (mc.contains("group") && mc["group"].is_string()) {
            group = mc["group"].get<std::string>();
        }
    }

    // Sanitize creative group to standard vanilla groups so Bedrock creative tabs accept them
    if (group.find("cooker") != std::string::npos || group.find("kitchen") != std::string::npos) {
        group = "itemGroup.name.misc";
    } else if (group.find("crate") != std::string::npos || group.find("cabinet") != std::string::npos || group.find("basket") != std::string::npos) {
        group = "itemGroup.name.stone";
    } else if (group.find("crop") != std::string::npos) {
        group = "itemGroup.name.crop";
    } else if (group.find(':') != std::string::npos || group == "itemGroup.name.craftingTable" || group.empty()) {
        group = "itemGroup.name.stone";
    }

    float destroyTime = 1.5f;
    float explosionResistance = 6.0f;
    uint8_t lightEmission = 0;
    std::string textureName = "";
    std::string geometry = "";

    if (blockObj.contains("components") && blockObj["components"].is_object()) {
        const auto& comps = blockObj["components"];

        if (comps.contains("minecraft:destructible_by_mining") && comps["minecraft:destructible_by_mining"].is_object()) {
            const auto& dbm = comps["minecraft:destructible_by_mining"];
            if (dbm.contains("seconds_to_destroy") && dbm["seconds_to_destroy"].is_number()) {
                destroyTime = dbm["seconds_to_destroy"].get<float>();
            }
        }

        if (comps.contains("minecraft:destructible_by_explosion") && comps["minecraft:destructible_by_explosion"].is_object()) {
            const auto& dbe = comps["minecraft:destructible_by_explosion"];
            if (dbe.contains("explosion_resistance") && dbe["explosion_resistance"].is_number()) {
                explosionResistance = dbe["explosion_resistance"].get<float>();
            }
        }

        if (comps.contains("minecraft:light_emission") && comps["minecraft:light_emission"].is_number()) {
            lightEmission = static_cast<uint8_t>(comps["minecraft:light_emission"].get<int>());
        }

        if (comps.contains("minecraft:geometry")) {
            if (comps["minecraft:geometry"].is_string()) {
                geometry = comps["minecraft:geometry"].get<std::string>();
            } else if (comps["minecraft:geometry"].is_object() && comps["minecraft:geometry"].contains("identifier") && comps["minecraft:geometry"]["identifier"].is_string()) {
                geometry = comps["minecraft:geometry"]["identifier"].get<std::string>();
            }
        }

        if (comps.contains("minecraft:material_instances") && comps["minecraft:material_instances"].is_object()) {
            const auto& mi = comps["minecraft:material_instances"];
            if (mi.contains("*") && mi["*"].is_object() && mi["*"].contains("texture") && mi["*"]["texture"].is_string()) {
                textureName = mi["*"]["texture"].get<std::string>();
            } else {
                for (auto& [key, val] : mi.items()) {
                    if (val.is_object() && val.contains("texture") && val["texture"].is_string()) {
                        textureName = val["texture"].get<std::string>();
                        break;
                    }
                }
            }
        }
    }

    if (textureName.empty()) {
        auto colon = identifier.find(':');
        textureName = (colon != std::string::npos) ? identifier.substr(colon + 1) : identifier;
    }

    std::string baseBlock = "minecraft:stone";
    if (identifier.find("wild_") != std::string::npos || identifier.find("flower") != std::string::npos ||
        identifier.find("shrub") != std::string::npos || identifier.find("allium") != std::string::npos ||
        identifier.find("poppy") != std::string::npos) {
        baseBlock = "minecraft:allium";
    } else if (identifier.find("seed") != std::string::npos || identifier.find("rice") != std::string::npos ||
               identifier.find("cabbage") != std::string::npos || identifier.find("tomato") != std::string::npos ||
               identifier.find("onion") != std::string::npos || identifier.find("crop") != std::string::npos) {
        baseBlock = "minecraft:wheat";
    } else if (identifier.find("table") != std::string::npos || identifier.find("crafting_table") != std::string::npos ||
        group.find("craftingTable") != std::string::npos || geometry.find("table") != std::string::npos ||
        identifier.find("cutting_board") != std::string::npos || identifier.find("board") != std::string::npos) {
        baseBlock = "minecraft:crafting_table";
    } else if (identifier.find("rug") != std::string::npos || identifier.find("carpet") != std::string::npos || identifier.find("canvas") != std::string::npos) {
        baseBlock = "minecraft:white_carpet";
    } else if (identifier.find("tatami") != std::string::npos || identifier.find("mat") != std::string::npos || identifier.find("slab") != std::string::npos) {
        baseBlock = "minecraft:wooden_slab";
    } else if (identifier.find("pot") != std::string::npos || identifier.find("skillet") != std::string::npos) {
        baseBlock = "minecraft:flower_pot";
    } else if (identifier.find("pie") != std::string::npos || identifier.find("cake") != std::string::npos || identifier.find("feast") != std::string::npos) {
        baseBlock = "minecraft:cake";
    } else if (identifier.find("stove") != std::string::npos || identifier.find("furnace") != std::string::npos) {
        baseBlock = "minecraft:furnace";
    } else if (identifier.find("cabinet") != std::string::npos || identifier.find("crate") != std::string::npos || identifier.find("basket") != std::string::npos) {
        baseBlock = "minecraft:barrel";
    } else if (identifier.find("soil") != std::string::npos || identifier.find("farmland") != std::string::npos || identifier.find("compost") != std::string::npos) {
        baseBlock = "minecraft:farmland";
    } else if (identifier.find("rope") != std::string::npos || identifier.find("net") != std::string::npos) {
        baseBlock = "minecraft:chain";
    }

    std::string blockDisplayName = "";
    if (blockObj.contains("components") && blockObj["components"].is_object()) {
        const auto& comps = blockObj["components"];
        if (comps.contains("minecraft:display_name")) {
            if (comps["minecraft:display_name"].is_object() && comps["minecraft:display_name"].contains("value") && comps["minecraft:display_name"]["value"].is_string()) {
                blockDisplayName = comps["minecraft:display_name"]["value"].get<std::string>();
            } else if (comps["minecraft:display_name"].is_string()) {
                blockDisplayName = comps["minecraft:display_name"].get<std::string>();
            }
        }
    }
    if (blockDisplayName.empty()) {
        auto colon = identifier.find(':');
        std::string s = (colon != std::string::npos) ? identifier.substr(colon + 1) : identifier;
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
        blockDisplayName = s;
    }

    ::levilamroid::block::CustomBlockDefinition def;
    def.identifier = identifier;
    def.displayName = blockDisplayName;
    def.textureName = textureName;
    def.category = category;
    def.creativeGroup = group;
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;
    def.baseBlock = baseBlock;
    def.geometry = geometry;

    sIngestedBlocks.insert(identifier);
    ::levilamroid::block::registerCustomBlock(def);

    // Register corresponding 3D BlockItem in ItemRegistry with authentic geometry
    ::levilamroid::item::CustomItemDefinition itemDef;
    itemDef.identifier = identifier;
    const auto* existingItem = ::levilamroid::item::getCustomItem(identifier);
    if (existingItem && existingItem->id > 0) {
        itemDef = *existingItem;
    } else {
        static std::atomic<short> sAutoIngestBlockId{10150};
        itemDef.id = sAutoIngestBlockId.fetch_add(1);
    }
    itemDef.displayName = blockDisplayName;
    itemDef.descriptionId = std::string("tile.") + identifier + ".name";
    itemDef.iconName = ""; // Empty 2D icon forces Bedrock to render authentic 3D block cube!
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
    ::levilamroid::item::registerCustomItem(itemDef);

    logger.info("§6[AddonIngest] §aIndexed block JSON for native engine loading: §b", identifier,
                " §7(Tex: ", textureName, ", Base: ", baseBlock, ", Geo: ", geometry, ", Light: ", (int)lightEmission, ")");
}

static std::string resolveRecipeTag(const std::string& tag) {
    if (tag == "minecraft:egg") return "minecraft:egg";
    if (tag == "minecraft:is_cooked") return "minecraft:cooked_beef";
    if (tag == "minecraft:planks") return "minecraft:oak_planks";
    if (tag == "minecraft:wooden_slabs") return "minecraft:oak_slab";
    if (tag == "minecraft:stone_tool_materials") return "minecraft:cobblestone";

    std::string stem = tag;
    auto colon = stem.find(':');
    std::string ns = "";
    if (colon != std::string::npos) {
        ns = stem.substr(0, colon);
        stem = stem.substr(colon + 1);
    }
    if (stem.rfind("is_", 0) == 0) {
        stem = stem.substr(3);
    }

    // Match exact shortName or containing stem across registered custom items
    const auto& items = ::levilamroid::item::getAllCustomItems();
    for (const auto& it : items) {
        if (!ns.empty() && it.identifier.rfind(ns + ":", 0) != 0) continue;
        std::string shortName = it.identifier;
        auto c = shortName.find(':');
        if (c != std::string::npos) shortName = shortName.substr(c + 1);
        if (shortName == stem) return it.identifier;
    }
    for (const auto& it : items) {
        if (!ns.empty() && it.identifier.rfind(ns + ":", 0) != 0) continue;
        if (it.identifier.find(stem) != std::string::npos) return it.identifier;
    }

    // Match across registered custom blocks
    const auto blocks = ::levilamroid::block::getRegisteredCustomBlocks();
    for (const auto& b : blocks) {
        if (!ns.empty() && b.identifier.rfind(ns + ":", 0) != 0) continue;
        std::string shortName = b.identifier;
        auto c = shortName.find(':');
        if (c != std::string::npos) shortName = shortName.substr(c + 1);
        if (shortName == stem) return b.identifier;
    }

    return "";
}

static void ingestRecipeJson(const std::filesystem::path& filePath) {
    auto& logger = ll::Logger::getInstance();
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
    } catch (...) {
        return;
    }
    if (root.is_discarded() || !root.is_object()) return;

    if (root.contains("minecraft:recipe_shaped") && root["minecraft:recipe_shaped"].is_object()) {
        const auto& shaped = root["minecraft:recipe_shaped"];
        std::string recipeId = shaped.value("description", nlohmann::json::object()).value("identifier", "");
        if (recipeId.empty()) recipeId = filePath.stem().string();
        if (sIngestedRecipes.count(recipeId)) return;
        sIngestedRecipes.insert(recipeId);

        std::string outItem = "";
        int outCount = 1;
        if (shaped.contains("result")) {
            if (shaped["result"].is_object()) {
                outItem = shaped["result"].value("item", "");
                outCount = shaped["result"].value("count", 1);
            } else if (shaped["result"].is_string()) {
                outItem = shaped["result"].get<std::string>();
            }
        }
        if (outItem.empty()) return;

        std::string r1 = "", r2 = "", r3 = "";
        if (shaped.contains("pattern") && shaped["pattern"].is_array()) {
            size_t pSize = shaped["pattern"].size();
            if (pSize > 0 && shaped["pattern"][0].is_string()) r1 = shaped["pattern"][0].get<std::string>();
            if (pSize > 1 && shaped["pattern"][1].is_string()) r2 = shaped["pattern"][1].get<std::string>();
            if (pSize > 2 && shaped["pattern"][2].is_string()) r3 = shaped["pattern"][2].get<std::string>();
        }

        std::vector<std::pair<char, std::string>> keyMap;
        if (shaped.contains("key") && shaped["key"].is_object()) {
            for (auto& [kStr, kVal] : shaped["key"].items()) {
                if (kStr.empty()) continue;
                char kChar = kStr[0];
                std::string inItem = "";
                if (kVal.is_object()) {
                    inItem = kVal.value("item", "");
                    if (inItem.empty() && kVal.contains("tag") && kVal["tag"].is_string()) {
                        inItem = resolveRecipeTag(kVal["tag"].get<std::string>());
                    }
                } else if (kVal.is_string()) {
                    inItem = kVal.get<std::string>();
                }
                if (!inItem.empty()) keyMap.push_back({kChar, inItem});
            }
        }

        sIngestedRecipes.insert(recipeId);
        ::levilamroid::recipe::addShapedRecipe(recipeId, outItem, outCount, r1, r2, r3, keyMap);
        logger.info("§6[AddonIngest] §aIndexed shaped recipe JSON for native engine loading: §b", recipeId,
                    " -> §e", outItem, " x", outCount);

    } else if (root.contains("minecraft:recipe_shapeless") && root["minecraft:recipe_shapeless"].is_object()) {
        const auto& shapeless = root["minecraft:recipe_shapeless"];
        std::string recipeId = shapeless.value("description", nlohmann::json::object()).value("identifier", "");
        if (recipeId.empty()) recipeId = filePath.stem().string();

        std::string outItem = "";
        int outCount = 1;
        if (shapeless.contains("result")) {
            if (shapeless["result"].is_object()) {
                outItem = shapeless["result"].value("item", "");
                outCount = shapeless["result"].value("count", 1);
            } else if (shapeless["result"].is_string()) {
                outItem = shapeless["result"].get<std::string>();
            }
        }
        if (outItem.empty()) return;

        std::vector<std::string> inputs;
        if (shapeless.contains("ingredients") && shapeless["ingredients"].is_array()) {
            for (const auto& ing : shapeless["ingredients"]) {
                if (ing.is_object()) {
                    std::string inItem = ing.value("item", "");
                    if (inItem.empty() && ing.contains("tag") && ing["tag"].is_string()) {
                        inItem = resolveRecipeTag(ing["tag"].get<std::string>());
                    }
                    if (!inItem.empty()) inputs.push_back(inItem);
                } else if (ing.is_string()) {
                    inputs.push_back(ing.get<std::string>());
                }
            }
        }

        sIngestedRecipes.insert(recipeId);
        ::levilamroid::recipe::addShapelessRecipe(recipeId, outItem, outCount, inputs);
        logger.info("§6[AddonIngest] §aIndexed shapeless recipe JSON for native engine loading: §b", recipeId,
                    " -> §e", outItem, " x", outCount);

    } else if (root.contains("minecraft:recipe_furnace") && root["minecraft:recipe_furnace"].is_object()) {
        const auto& furnace = root["minecraft:recipe_furnace"];
        std::string recipeId = furnace.value("description", nlohmann::json::object()).value("identifier", "");
        if (recipeId.empty()) recipeId = filePath.stem().string();
        if (sIngestedRecipes.count(recipeId)) return;
        sIngestedRecipes.insert(recipeId);

        std::string inItem = furnace.value("input", "");
        std::string outItem = furnace.value("output", "");
        if (!inItem.empty() && !outItem.empty()) {
            std::vector<std::string> inputs = { inItem };
            ::levilamroid::recipe::addShapelessRecipe(recipeId, outItem, 1, inputs);
            logger.info("§6[AddonIngest] §aIndexed furnace/cooking recipe: §b", recipeId,
                        " -> §e", outItem, " from ", inItem);
        }
    } else if (root.contains("minecraft:recipe_smithing_transform") && root["minecraft:recipe_smithing_transform"].is_object()) {
        const auto& smithing = root["minecraft:recipe_smithing_transform"];
        std::string recipeId = smithing.value("description", nlohmann::json::object()).value("identifier", "");
        if (recipeId.empty()) recipeId = filePath.stem().string();
        if (sIngestedRecipes.count(recipeId)) return;
        sIngestedRecipes.insert(recipeId);

        std::string templateItem = smithing.value("template", "");
        std::string baseItem = smithing.value("base", "");
        std::string additionItem = smithing.value("addition", "");
        std::string resultItem = smithing.value("result", "");

        if (!baseItem.empty() && !additionItem.empty() && !resultItem.empty()) {
            std::vector<std::string> inputs;
            if (!templateItem.empty()) inputs.push_back(templateItem);
            inputs.push_back(baseItem);
            inputs.push_back(additionItem);
            ::levilamroid::recipe::addShapelessRecipe(recipeId, resultItem, 1, inputs);
            logger.info("§6[AddonIngest] §aIndexed smithing transform recipe: §b", recipeId,
                        " -> §e", resultItem, " from ", baseItem, " + ", additionItem);
        }
    }
}

static void ingestGeometryJson(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string clean = stripJsonComments(rawContent);

    nlohmann::json root = nlohmann::json::parse(clean, nullptr, false, true);
    if (root.is_discarded() || !root.is_object()) return;

    std::string formatVer = root.value("format_version", "1.12.0");

    // Format 1.12.0+ (minecraft:geometry array)
    if (root.contains("minecraft:geometry") && root["minecraft:geometry"].is_array()) {
        for (const auto& geomObj : root["minecraft:geometry"]) {
            if (!geomObj.is_object() || !geomObj.contains("description")) continue;
            std::string ident = geomObj["description"].value("identifier", "");
            if (ident.empty() || sIngestedGeometries.count(ident)) continue;
            sIngestedGeometries.insert(ident);

            ::levilamroid::geometry::CustomGeometryDefinition def;
            def.identifier = ident;
            def.formatVersion = formatVer;
            def.visibleBoundsWidth = geomObj["description"].value("visible_bounds_width", 1.0f);
            def.visibleBoundsHeight = geomObj["description"].value("visible_bounds_height", 1.0f);
            def.jsonContent = geomObj.dump();

            if (geomObj.contains("bones") && geomObj["bones"].is_array()) {
                for (const auto& bone : geomObj["bones"]) {
                    if (bone.is_object() && bone.contains("name") && bone["name"].is_string()) {
                        def.boneNames.push_back(bone["name"].get<std::string>());
                    }
                }
            }

            ::levilamroid::geometry::registerGeometry(def);
            ll::Logger::getInstance().info("§6[AddonIngest] §aIndexed 3D geometry: §b", ident,
                                           " §7(Bones: ", def.boneNames.size(), ", Version: ", formatVer, ")");
        }
    } else {
        // Legacy format: top-level keys starting with "geometry."
        for (auto& [key, val] : root.items()) {
            if (key.rfind("geometry.", 0) == 0 && val.is_object()) {
                if (sIngestedGeometries.count(key)) continue;
                sIngestedGeometries.insert(key);

                ::levilamroid::geometry::CustomGeometryDefinition def;
                def.identifier = key;
                def.formatVersion = formatVer;
                def.jsonContent = val.dump();
                if (val.contains("bones") && val["bones"].is_array()) {
                    for (const auto& bone : val["bones"]) {
                        if (bone.is_object() && bone.contains("name") && bone["name"].is_string()) {
                            def.boneNames.push_back(bone["name"].get<std::string>());
                        }
                    }
                }

                ::levilamroid::geometry::registerGeometry(def);
                ll::Logger::getInstance().info("§6[AddonIngest] §aIndexed legacy 3D geometry: §b", key,
                                               " §7(Bones: ", def.boneNames.size(), ")");
            }
        }
    }
}

static void ingestEntityJson(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string clean = stripJsonComments(rawContent);

    nlohmann::json root = nlohmann::json::parse(clean, nullptr, false, true);
    if (root.is_discarded() || !root.contains("minecraft:entity")) return;

    const auto& entObj = root["minecraft:entity"];
    if (!entObj.contains("description") || !entObj["description"].contains("identifier")) return;

    std::string identifier = entObj["description"]["identifier"].get<std::string>();
    if (identifier.empty() || sIngestedEntities.count(identifier)) return;
    sIngestedEntities.insert(identifier);

    bool isSpawnable = entObj["description"].value("is_spawnable", true);
    bool isSummonable = entObj["description"].value("is_summonable", true);
    std::string runtimeId = entObj["description"].value("runtime_identifier", "minecraft:zombie");

    float width = 0.6f;
    float height = 1.8f;
    float maxHealth = 20.0f;

    if (entObj.contains("components") && entObj["components"].is_object()) {
        const auto& comps = entObj["components"];
        if (comps.contains("minecraft:collision_box") && comps["minecraft:collision_box"].is_object()) {
            width = comps["minecraft:collision_box"].value("width", 0.6f);
            height = comps["minecraft:collision_box"].value("height", 1.8f);
        }
        if (comps.contains("minecraft:health") && comps["minecraft:health"].is_object()) {
            maxHealth = comps["minecraft:health"].value("max", 20.0f);
        }
    }

    ::levilamroid::entity::CustomEntityDefinition def;
    def.identifier = identifier;
    def.displayName = identifier;
    def.runtimeId = runtimeId;
    def.width = width;
    def.height = height;
    def.maxHealth = maxHealth;
    def.isSpawnable = isSpawnable;
    def.isSummonable = isSummonable;

    ::levilamroid::entity::registerEntity(def);
    ll::Logger::getInstance().info("§6[AddonIngest] §aIndexed entity JSON: §b", identifier,
                                   " §7(Base: ", runtimeId, ", Health: ", maxHealth, ", Box: ", width, "x", height, ")");
}

void ingestAddonDirectory(const std::filesystem::path& dir) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return;

    static std::set<std::string> sIngestedDirs;
    std::string canonicalPath = dir.string();
    if (!sIngestedDirs.insert(canonicalPath).second) {
        return; // Already ingested, skip immediately!
    }

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[AddonIngest] §eScanning directory for unified BP-in-RP JSON files: ", dir.string());

    std::set<std::filesystem::path> modelDirs;
    std::set<std::filesystem::path> entityDirs;
    std::set<std::filesystem::path> blockDirs;
    std::set<std::filesystem::path> itemDirs;
    std::set<std::filesystem::path> recipeDirs;

    // Direct check for standard addon subfolders (models, entities, blocks, items, recipes)
    if (std::filesystem::is_directory(dir / "models", ec)) modelDirs.insert(dir / "models");
    if (std::filesystem::is_directory(dir / "entities", ec)) entityDirs.insert(dir / "entities");
    if (std::filesystem::is_directory(dir / "blocks", ec)) blockDirs.insert(dir / "blocks");
    if (std::filesystem::is_directory(dir / "items", ec)) itemDirs.insert(dir / "items");
    if (std::filesystem::is_directory(dir / "recipes", ec)) recipeDirs.insert(dir / "recipes");

    // Shallow check of subdirectories (avoids deep recursive iteration through textures/ and sounds/)
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_directory(ec)) {
            auto name = entry.path().filename().string();
            if (name == "models") modelDirs.insert(entry.path());
            else if (name == "entities") entityDirs.insert(entry.path());
            else if (name == "blocks") blockDirs.insert(entry.path());
            else if (name == "items") itemDirs.insert(entry.path());
            else if (name == "recipes") recipeDirs.insert(entry.path());
            else if (name == "subpacks") {
                for (const auto& sp : std::filesystem::directory_iterator(entry.path(), ec)) {
                    if (sp.is_directory(ec)) {
                        if (std::filesystem::is_directory(sp.path() / "models", ec)) modelDirs.insert(sp.path() / "models");
                        if (std::filesystem::is_directory(sp.path() / "entities", ec)) entityDirs.insert(sp.path() / "entities");
                        if (std::filesystem::is_directory(sp.path() / "blocks", ec)) blockDirs.insert(sp.path() / "blocks");
                        if (std::filesystem::is_directory(sp.path() / "items", ec)) itemDirs.insert(sp.path() / "items");
                        if (std::filesystem::is_directory(sp.path() / "recipes", ec)) recipeDirs.insert(sp.path() / "recipes");
                    }
                }
            }
        }
    }

    // 1. Scan models/ folders (3D geometries for blocks and entities)
    for (const auto& mDir : modelDirs) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(mDir, ec)) {
            if (entry.is_regular_file(ec)) {
                auto ext = entry.path().extension().string();
                auto fn = entry.path().filename().string();
                if (ext == ".json" || fn.find(".geo.json") != std::string::npos) {
                    try {
                        ingestGeometryJson(entry.path());
                    } catch (const std::exception& e) {
                        logger.error("[AddonIngest] Error parsing geometry JSON '", entry.path().filename().string(), "': ", e.what());
                    }
                }
            }
        }
    }

    // 2. Scan entities/ folders (custom mobs and entities)
    for (const auto& eDir : entityDirs) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(eDir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                try {
                    ingestEntityJson(entry.path());
                } catch (const std::exception& e) {
                    logger.error("[AddonIngest] Error parsing entity JSON '", entry.path().filename().string(), "': ", e.what());
                }
            }
        }
    }

    // 3. Scan all blocks/ folders first so custom block definitions and IDs are established
    for (const auto& bDir : blockDirs) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(bDir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                try {
                    ingestBlockJson(entry.path());
                } catch (const std::exception& e) {
                    logger.error("[AddonIngest] Error parsing block JSON '", entry.path().filename().string(), "': ", e.what());
                }
            }
        }
    }

    // 4. Scan all items/ folders (binds to established blocks for BlockItem vtables)
    for (const auto& iDir : itemDirs) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(iDir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                try {
                    ingestItemJson(entry.path());
                } catch (const std::exception& e) {
                    logger.error("[AddonIngest] Error parsing item JSON '", entry.path().filename().string(), "': ", e.what());
                }
            }
        }
    }

    // 5. Scan all recipes/ folders
    for (const auto& rDir : recipeDirs) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(rDir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                try {
                    ingestRecipeJson(entry.path());
                } catch (const std::exception& e) {
                    logger.error("[AddonIngest] Error parsing recipe JSON '", entry.path().filename().string(), "': ", e.what());
                }
            }
        }
    }

    logger.info("§6[AddonIngest] §aCompleted scanning: ", dir.string(),
                " (Geometries: ", sIngestedGeometries.size(),
                ", Entities: ", sIngestedEntities.size(),
                ", Items: ", sIngestedItems.size(),
                ", Blocks: ", sIngestedBlocks.size(),
                ", Recipes: ", sIngestedRecipes.size(), ")");
}

} // namespace levilamroid::addon
