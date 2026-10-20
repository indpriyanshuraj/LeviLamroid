#pragma once
#include "ll/api/Macros.h"
#include <string>
#include <vector>
#include <utility>

namespace levilamroid::recipe {

LL_API bool addShapedRecipe(
    const std::string& recipeId,
    const std::string& output,
    int outputCount,
    const std::string& r1,
    const std::string& r2,
    const std::string& r3,
    const std::vector<std::pair<char, std::string>>& keyMap
);

LL_API bool addShapelessRecipe(
    const std::string& recipeId,
    const std::string& output,
    int outputCount,
    const std::vector<std::string>& inputs
);

} // namespace levilamroid::recipe
