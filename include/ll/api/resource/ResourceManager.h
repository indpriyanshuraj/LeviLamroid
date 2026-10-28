#pragma once

#include "ll/api/Macros.h"
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <functional>

namespace ll::resource {

class LL_API ResourceManager {
private:
    std::vector<std::filesystem::path> mMountedDirectories;
    std::unordered_map<std::string, std::filesystem::path> mAssetCache;
    mutable std::mutex mMutex;


    ResourceManager() = default;

public:
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    /// Mount a directory containing raw resource files (textures/, texts/, items/, recipes/)
    /// No manifest.json or pack UUID required.
    void mountDirectory(const std::filesystem::path& dir);

    /// Check if any mounted directory contains the specified asset
    bool hasAsset(std::string_view relativePath) const;

    /// Read asset content from mounted directories into outBuffer
    bool getAsset(std::string_view relativePath, std::string& outBuffer) const;

    /// Dynamically inject custom item textures into item_texture.json buffer
    bool patchItemTextureJson(std::string& inOutJson) const;

    /// Dynamically inject custom block textures into terrain_texture.json buffer
    bool patchTerrainTextureJson(std::string& inOutJson) const;

    /// Dynamically inject custom block definitions into blocks.json buffer
    bool patchBlocksJson(std::string& inOutJson) const;

    /// Dynamically append custom item names into .lang buffer
    bool patchLangFile(std::string& inOutLang, std::string_view reqPath = "") const;

    /// Dynamically inject custom screen json definitions into ui/_ui_defs.json
    bool patchUiDefsJson(std::string& inOutJson) const;

    /// Dynamically merge custom sound definitions into sounds/sound_definitions.json buffer
    bool patchSoundDefinitionsJson(std::string& inOutJson) const;

    /// Dynamically merge custom flipbook texture definitions into textures/flipbook_textures.json buffer
    bool patchFlipbookTexturesJson(std::string& inOutJson) const;

    /// Register a direct memory/file asset
    void registerAsset(const std::string& relativePath, const std::filesystem::path& diskPath);

    /// Get list of all mounted directories
    std::vector<std::filesystem::path> getMountedDirectories() const;

    /// Enumerate all mounted assets whose relative path starts with prefix
    void forEachAssetMatching(std::string_view prefix, const std::function<void(const std::string& relPath)>& callback) const;

    /// Automatically discover and mount resources folders from all installed mods and profiles
    void autoDiscoverModResources();

    /// Called on start screen after normal resource loading completes to activate mod resources globally over vanilla
    void activateModResourcesOnStartScreen();
};

} // namespace ll::resource

extern "C" {
LL_API void ll_resource_mount_dir(const char* dir);
LL_API void ll_resource_auto_discover();
LL_API bool ll_resource_has_asset(const char* relativePath);
LL_API bool ll_resource_get_asset(const char* relativePath, char** outBuffer, size_t* outSize);
LL_API void ll_resource_free_buffer(char* buffer);
}
