#pragma once
#include <filesystem>

namespace levilamroid::addon {

/// Scans and ingests standard Bedrock addon data from a directory:
/// - items/*.json (minecraft:item)
/// - blocks/*.json (minecraft:block)
/// - recipes/*.json (minecraft:recipe_shaped, minecraft:recipe_shapeless)
/// Automatically parses components and registers into native engine systems.
void ingestAddonDirectory(const std::filesystem::path& dir);

} // namespace levilamroid::addon
