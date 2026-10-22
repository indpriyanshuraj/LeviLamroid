#include "AddonIngestion.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "levilamroid/api/recipe/RecipeRegistry.h"
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

    auto& logger = ll::Logger::getInstance();

    int category = 4; // Default: Items (4)
    std::string group = "";

    if (itemObj["description"].contains("menu_category") && itemObj["description"]["menu_category"].is_object()) {
        const auto& mc = itemObj["description"]["menu_category"];
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
    } else if (itemObj["description"].contains("category") && itemObj["description"]["category"].is_string()) {
        std::string c = itemObj["description"]["category"].get<std::string>();
        std::transform(c.begin(), c.end(), c.begin(), [](unsigned char ch){ return std::tolower(ch); });
        if (c == "construction") category = 1;
        else if (c == "nature") category = 2;
        else if (c == "equipment") category = 3;
        else if (c == "items") category = 4;
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
    int nutrition = 0;
    float saturationModifier = 0.6f;
    int useAnimation = 0;
    int useDuration = 0;
    uint8_t maxStackSize = 0;
    bool isHandEquipped = false;
    bool allowOffhand = true;

    if (itemObj.contains("components") && itemObj["components"].is_object()) {
        const auto& comps = itemObj["components"];

        // Icon
        if (comps.contains("minecraft:icon")) {
            if (comps["minecraft:icon"].is_string()) {
                iconName = comps["minecraft:icon"].get<std::string>();
            } else if (comps["minecraft:icon"].is_object()) {
                if (comps["minecraft:icon"].contains("texture") && comps["minecraft:icon"]["texture"].is_string()) {
                    iconName = comps["minecraft:icon"]["texture"].get<std::string>();
                }
                if (comps["minecraft:icon"].contains("frame") && comps["minecraft:icon"]["frame"].is_number()) {
                    iconFrame = comps["minecraft:icon"]["frame"].get<int>();
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
    }

    if (iconName.empty()) {
        auto colon = identifier.find(':');
        iconName = (colon != std::string::npos) ? identifier.substr(colon + 1) : identifier;
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

    bool isBlock = (category == 1 || identifier.find("block") != std::string::npos ||
                    identifier.find("table") != std::string::npos || sIngestedBlocks.count(identifier) > 0);
    std::string baseBlock = (identifier.find("table") != std::string::npos || identifier.find("crafting_table") != std::string::npos) ?
                            "minecraft:crafting_table" : "minecraft:stone";

    ::levilamroid::item::CustomItemDefinition def;
    def.identifier = identifier;
    def.id = itemId;
    def.descriptionId = "item." + identifier;
    def.iconName = iconName;
    def.iconFrame = iconFrame;
    def.category = category;
    def.creativeGroup = group;
    def.allowOffhand = allowOffhand;
    def.maxStackSize = maxStackSize;
    def.isHandEquipped = isHandEquipped;
    def.isFood = isFood;
    def.nutrition = nutrition;
    def.saturationModifier = saturationModifier;
    def.useAnimation = useAnimation;
    def.useDuration = useDuration;
    def.attackDamage = attackDamage;
    def.maxDamage = maxDamage;
    def.isBlock = isBlock;
    def.baseBlock = baseBlock;

    bool ok = ::levilamroid::item::registerCustomItem(def);
    sIngestedItems.insert(identifier);

    logger.info("§6[AddonIngest] §aIngested item from JSON: §b", identifier,
                " §7(ID: ", itemId, ", Cat: ", category, ", Icon: ", iconName, ", Stack: ", (int)maxStackSize, ") -> ", ok ? "SUCCESS" : "FAILED");
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

        if (comps.contains("minecraft:geometry") && comps["minecraft:geometry"].is_string()) {
            geometry = comps["minecraft:geometry"].get<std::string>();
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
    if (identifier.find("table") != std::string::npos || identifier.find("crafting_table") != std::string::npos ||
        group.find("craftingTable") != std::string::npos || geometry.find("table") != std::string::npos) {
        baseBlock = "minecraft:crafting_table";
    }

    ::levilamroid::block::CustomBlockDefinition def;
    def.identifier = identifier;
    def.displayName = identifier;
    def.textureName = textureName;
    def.category = category;
    def.creativeGroup = group;
    def.destroyTime = destroyTime;
    def.explosionResistance = explosionResistance;
    def.lightEmission = lightEmission;
    def.baseBlock = baseBlock;

    bool ok = ::levilamroid::block::registerCustomBlock(def);
    sIngestedBlocks.insert(identifier);

    logger.info("§6[AddonIngest] §aIngested block from JSON: §b", identifier,
                " §7(Tex: ", textureName, ", Base: ", baseBlock, ", Light: ", (int)lightEmission, ") -> ", ok ? "SUCCESS" : "FAILED");
}

static void ingestRecipeJson(const std::filesystem::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return;

    std::string rawContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string clean = stripJsonComments(rawContent);

    nlohmann::json root = nlohmann::json::parse(clean, nullptr, false, true);
    if (root.is_discarded()) return;

    auto& logger = ll::Logger::getInstance();

    if (root.contains("minecraft:recipe_shaped") && root["minecraft:recipe_shaped"].is_object()) {
        const auto& shaped = root["minecraft:recipe_shaped"];
        std::string recipeId = shaped.value("description", nlohmann::json::object()).value("identifier", "");
        if (recipeId.empty()) recipeId = filePath.stem().string();

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
                if (kVal.is_object()) inItem = kVal.value("item", "");
                else if (kVal.is_string()) inItem = kVal.get<std::string>();
                if (!inItem.empty()) keyMap.push_back({kChar, inItem});
            }
        }

        bool ok = ::levilamroid::recipe::addShapedRecipe(recipeId, outItem, outCount, r1, r2, r3, keyMap);
        sIngestedRecipes.insert(recipeId);
        logger.info("§6[AddonIngest] §aIngested shaped recipe from JSON: §b", recipeId,
                    " -> §e", outItem, " x", outCount, " §7-> ", ok ? "SUCCESS" : "FAILED");

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
                std::string inItem = "";
                if (ing.is_object()) inItem = ing.value("item", "");
                else if (ing.is_string()) inItem = ing.get<std::string>();
                if (!inItem.empty()) inputs.push_back(inItem);
            }
        }

        bool ok = ::levilamroid::recipe::addShapelessRecipe(recipeId, outItem, outCount, inputs);
        sIngestedRecipes.insert(recipeId);
        logger.info("§6[AddonIngest] §aIngested shapeless recipe from JSON: §b", recipeId,
                    " -> §e", outItem, " x", outCount, " §7-> ", ok ? "SUCCESS" : "FAILED");
    }
}

void ingestAddonDirectory(const std::filesystem::path& dir) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return;

    auto& logger = ll::Logger::getInstance();
    logger.info("§6[AddonIngest] §eScanning directory for unified BP-in-RP JSON files: ", dir.string());

    std::vector<std::filesystem::path> blockDirs;
    std::vector<std::filesystem::path> itemDirs;
    std::vector<std::filesystem::path> recipeDirs;

    // Direct check
    if (std::filesystem::is_directory(dir / "blocks", ec)) blockDirs.push_back(dir / "blocks");
    if (std::filesystem::is_directory(dir / "items", ec)) itemDirs.push_back(dir / "items");
    if (std::filesystem::is_directory(dir / "recipes", ec)) recipeDirs.push_back(dir / "recipes");

    // Recursive search for any subdirectories named blocks, items, or recipes
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        if (entry.is_directory(ec)) {
            auto name = entry.path().filename().string();
            if (name == "blocks") blockDirs.push_back(entry.path());
            else if (name == "items") itemDirs.push_back(entry.path());
            else if (name == "recipes") recipeDirs.push_back(entry.path());
        }
    }

    // 1. Scan all blocks/ folders first so custom block definitions and IDs are established
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

    // 2. Scan all items/ folders (binds to established blocks for BlockItem vtables)
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

    // 3. Scan all recipes/ folders
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
                " (Total Items: ", sIngestedItems.size(),
                ", Blocks: ", sIngestedBlocks.size(),
                ", Recipes: ", sIngestedRecipes.size(), ")");
}

} // namespace levilamroid::addon
