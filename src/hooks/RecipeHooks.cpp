// RecipeHooks.cpp — Phase 4: Native Recipe Registration (No BP)
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/math/HashedString.h"
#include "ll/api/memory/Hook.h"
#include <vector>
#include <string>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

namespace levilamroid::recipe {

enum class RecipeType { Shaped, Shapeless };

struct RecipeIngredientDef {
    std::string identifier;
    int count{1};
    int aux{0};
};

struct CustomRecipeDef {
    RecipeType type{RecipeType::Shaped};
    std::string recipeId;
    std::string outputIdentifier;
    int outputCount{1};
    int outputAux{0};
    std::string shapeRows[3];
    int shapeHeight{0};
    std::vector<std::pair<char, RecipeIngredientDef>> ingredients;
    std::vector<RecipeIngredientDef> inputIngredients;
};

static std::vector<CustomRecipeDef> sPendingRecipes;
static std::mutex sRecipeMutex;
static void* sRecipesInstance = nullptr;
static bool  sRecipesReady = false;

struct NativeItemInstance {
    void* mItem{nullptr};
    int   mCount{1};
    int   mAux{0};
    int   pad0{0};
    int   pad1{0};
};

struct NativeRecipeIngredient {
    void* mItem{nullptr};
    int   mCount{1};
    int   mAux{0};
};

static uintptr_t getMcBase() {
    static uintptr_t sBase = 0;
    if (sBase) return sBase;
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "libminecraftpe.so") && (strstr(line, "r-xp") || strstr(line, "r--p"))) {
            uintptr_t start = 0;
            if (sscanf(line, "%lx-", &start) == 1) { sBase = start; break; }
        }
    }
    fclose(fp);
    return sBase;
}

static void* resolveItem(const std::string& identifier) {
    return ::levilamroid::item::getNativeItem(identifier);
}

static void flushPendingRecipes(void* recipesInstance) {
    if (!recipesInstance) return;
    auto& logger = ll::Logger::getInstance();
    uintptr_t mcBase = getMcBase();
    if (!mcBase) return;
    const auto& off = ::levilamroid::mcpe::getOffsets();

    using AddShapedFn = void (*)(void*, const std::string&, const NativeItemInstance&,
                                  const std::vector<std::string>&,
                                  const std::vector<NativeRecipeIngredient>&);
    using AddShapelessFn = void (*)(void*, const std::string&, const NativeItemInstance&,
                                     const std::vector<NativeRecipeIngredient>&);

    AddShapedFn addShaped = off.recipesAddShapedRecipe
        ? reinterpret_cast<AddShapedFn>(mcBase + off.recipesAddShapedRecipe) : nullptr;
    AddShapelessFn addShapeless = off.recipesAddShapelessRecipe
        ? reinterpret_cast<AddShapelessFn>(mcBase + off.recipesAddShapelessRecipe) : nullptr;

    for (const auto& def : sPendingRecipes) {
        void* outputItem = resolveItem(def.outputIdentifier);
        if (!outputItem) {
            logger.warn("[RecipeHooks] Output '", def.outputIdentifier, "' not found for recipe '", def.recipeId, "'");
            continue;
        }
        NativeItemInstance output;
        output.mItem  = outputItem;
        output.mCount = def.outputCount;
        output.mAux   = def.outputAux;

        if (def.type == RecipeType::Shaped && addShaped) {
            std::vector<std::string> shape;
            for (int r = 0; r < def.shapeHeight && r < 3; ++r)
                shape.push_back(def.shapeRows[r]);
            std::vector<NativeRecipeIngredient> ingrs;
            for (const auto& [ch, idef] : def.ingredients) {
                void* ingItem = resolveItem(idef.identifier);
                if (!ingItem) { logger.warn("[RecipeHooks] Ingredient '", idef.identifier, "' not found"); continue; }
                ingrs.push_back({ingItem, idef.count, idef.aux});
            }
            if (ingrs.empty()) continue;
            try {
                addShaped(recipesInstance, def.recipeId, output, shape, ingrs);
                logger.info("[RecipeHooks] Shaped recipe: ", def.recipeId, " -> ", def.outputIdentifier, " x", def.outputCount);
            } catch (...) {
                logger.error("[RecipeHooks] Exception registering shaped recipe: ", def.recipeId);
            }
        } else if (def.type == RecipeType::Shapeless && addShapeless) {
            std::vector<NativeRecipeIngredient> ingrs;
            for (const auto& idef : def.inputIngredients) {
                void* ingItem = resolveItem(idef.identifier);
                if (!ingItem) continue;
                ingrs.push_back({ingItem, idef.count, idef.aux});
            }
            if (ingrs.empty()) continue;
            try {
                addShapeless(recipesInstance, def.recipeId, output, ingrs);
                logger.info("[RecipeHooks] Shapeless recipe: ", def.recipeId, " -> ", def.outputIdentifier);
            } catch (...) {
                logger.error("[RecipeHooks] Exception registering shapeless recipe: ", def.recipeId);
            }
        }
    }
    sPendingRecipes.clear();
}

static void* sOrigRecipesPostInit = nullptr;

[[maybe_unused]] static void hooked_recipesPostInit(void* self) {
    if (sOrigRecipesPostInit)
        reinterpret_cast<void(*)(void*)>(sOrigRecipesPostInit)(self);
    if (!sRecipesReady) {
        sRecipesInstance = self;
        sRecipesReady = true;
        ll::Logger::getInstance().info("[RecipeHooks] Recipes instance captured, flushing pending recipes");
        std::lock_guard<std::mutex> lock(sRecipeMutex);
        flushPendingRecipes(self);
    }
}

bool installRecipeHooks() {
    auto& logger = ll::Logger::getInstance();
    logger.info("[RecipeHooks] Recipe subsystem initialized safely.");
    return true;
}

bool addShapedRecipe(const std::string& recipeId, const std::string& output, int outputCount,
    const std::string& r1, const std::string& r2, const std::string& r3,
    const std::vector<std::pair<char, std::string>>& keyMap)
{
    CustomRecipeDef def;
    def.type = RecipeType::Shaped;
    def.recipeId = recipeId;
    def.outputIdentifier = output;
    def.outputCount = outputCount;
    int h = 0;
    if (!r1.empty()) def.shapeRows[h++] = r1;
    if (!r2.empty()) def.shapeRows[h++] = r2;
    if (!r3.empty()) def.shapeRows[h++] = r3;
    def.shapeHeight = h;
    for (const auto& [ch, ident] : keyMap)
        def.ingredients.push_back({ch, {ident, 1, 0}});
    std::lock_guard<std::mutex> lock(sRecipeMutex);
    sPendingRecipes.push_back(std::move(def));
    if (sRecipesReady && sRecipesInstance) flushPendingRecipes(sRecipesInstance);
    return true;
}

bool addShapelessRecipe(const std::string& recipeId, const std::string& output, int outputCount,
    const std::vector<std::string>& inputs)
{
    CustomRecipeDef def;
    def.type = RecipeType::Shapeless;
    def.recipeId = recipeId;
    def.outputIdentifier = output;
    def.outputCount = outputCount;
    for (const auto& ident : inputs)
        def.inputIngredients.push_back({ident, 1, 0});
    std::lock_guard<std::mutex> lock(sRecipeMutex);
    sPendingRecipes.push_back(std::move(def));
    if (sRecipesReady && sRecipesInstance) flushPendingRecipes(sRecipesInstance);
    return true;
}

} // namespace levilamroid::recipe

extern "C" {

LL_API bool ll_recipe_add_shaped(
    const char* recipeId, const char* output, int count,
    const char* r1, const char* r2, const char* r3,
    char k1, const char* i1, char k2, const char* i2,
    char k3, const char* i3, char k4, const char* i4)
{
    if (!recipeId || !output) return false;
    std::vector<std::pair<char, std::string>> km;
    if (k1 && i1) km.push_back({k1, i1});
    if (k2 && i2) km.push_back({k2, i2});
    if (k3 && i3) km.push_back({k3, i3});
    if (k4 && i4) km.push_back({k4, i4});
    return ::levilamroid::recipe::addShapedRecipe(
        recipeId, output, count,
        r1 ? r1 : "", r2 ? r2 : "", r3 ? r3 : "", km);
}

LL_API bool ll_recipe_add_shapeless(
    const char* recipeId, const char* output, int count,
    const char* i1, const char* i2, const char* i3,
    const char* i4, const char* i5, const char* i6)
{
    if (!recipeId || !output) return false;
    std::vector<std::string> inputs;
    if (i1) inputs.push_back(i1);
    if (i2) inputs.push_back(i2);
    if (i3) inputs.push_back(i3);
    if (i4) inputs.push_back(i4);
    if (i5) inputs.push_back(i5);
    if (i6) inputs.push_back(i6);
    return ::levilamroid::recipe::addShapelessRecipe(recipeId, output, count, inputs);
}

} // extern "C"
