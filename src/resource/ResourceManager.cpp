#include "ll/api/resource/ResourceManager.h"
#include "levilamroid/api/Logger.h"
#include "AddonIngestion.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace ll::resource {

void ResourceManager::mountDirectory(const std::filesystem::path& dir) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
        ll::Logger::getInstance().warn("ResourceManager: Directory does not exist: ", dir.string());
        return;
    }

    mMountedDirectories.push_back(dir);
    ll::Logger::getInstance().info("ResourceManager: Mounting raw resource directory: ", dir.string());

    // Recursively index all assets in the mounted directory
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec)) {
            auto rel = std::filesystem::relative(entry.path(), dir, ec).string();
            // Normalize path separators to forward slash
            std::replace(rel.begin(), rel.end(), '\\', '/');
            mAssetCache[rel] = entry.path();

            // Also index stripped relative paths for resource pack subfolders
            auto texPos = rel.find("textures/");
            if (texPos != std::string::npos && texPos > 0) {
                mAssetCache[rel.substr(texPos)] = entry.path();
            }
            auto langPos = rel.find("texts/");
            if (langPos != std::string::npos && langPos > 0) {
                mAssetCache[rel.substr(langPos)] = entry.path();
            }

            auto filename = entry.path().filename().string();
            mAssetCache[filename] = entry.path();
            if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".png") {
                mAssetCache[filename.substr(0, filename.size() - 4)] = entry.path();
            }

            ll::Logger::getInstance().debug("ResourceManager: Indexed raw asset: ", rel);
        }
    }

    // Ingest unified BP-in-RP JSON files (items, blocks, recipes)
    ::levilamroid::addon::ingestAddonDirectory(dir);
}

bool ResourceManager::hasAsset(std::string_view relativePath) const {
    std::string cleanPath(relativePath);
    if (!cleanPath.empty() && (cleanPath[0] == '/' || cleanPath[0] == '\\')) {
        cleanPath = cleanPath.substr(1);
    }
    std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

    std::lock_guard<std::mutex> lock(mMutex);
    if (mAssetCache.find(cleanPath) != mAssetCache.end()) return true;
    if (mAssetCache.find(cleanPath + ".png") != mAssetCache.end()) return true;
    if (mAssetCache.find(cleanPath + ".json") != mAssetCache.end()) return true;

    auto texPos = cleanPath.find("textures/");
    if (texPos != std::string::npos && texPos > 0) {
        std::string stripped = cleanPath.substr(texPos);
        if (mAssetCache.find(stripped) != mAssetCache.end()) return true;
        if (mAssetCache.find(stripped + ".png") != mAssetCache.end()) return true;
        if (mAssetCache.find(stripped + ".json") != mAssetCache.end()) return true;
    }
    return false;
}

bool ResourceManager::getAsset(std::string_view relativePath, std::string& outBuffer) const {
    std::string cleanPath(relativePath);
    if (!cleanPath.empty() && (cleanPath[0] == '/' || cleanPath[0] == '\\')) {
        cleanPath = cleanPath.substr(1);
    }
    std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

    std::filesystem::path targetFile;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        auto it = mAssetCache.find(cleanPath);
        if (it == mAssetCache.end()) it = mAssetCache.find(cleanPath + ".png");
        if (it == mAssetCache.end()) it = mAssetCache.find(cleanPath + ".json");

        if (it == mAssetCache.end()) {
            auto texPos = cleanPath.find("textures/");
            if (texPos != std::string::npos && texPos > 0) {
                std::string stripped = cleanPath.substr(texPos);
                it = mAssetCache.find(stripped);
                if (it == mAssetCache.end()) it = mAssetCache.find(stripped + ".png");
                if (it == mAssetCache.end()) it = mAssetCache.find(stripped + ".json");
            }
        }

        if (it != mAssetCache.end()) {
            targetFile = it->second;
        }
    }

    if (targetFile.empty()) {
        return false;
    }

    std::ifstream file(targetFile, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    outBuffer.resize(size);
    file.read(outBuffer.data(), size);
    ll::Logger::getInstance().debug("ResourceManager: Served asset: ", cleanPath, " (", size, " bytes)");
    return true;
}

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
                ++i; // skip '/'
            }
            continue;
        }

        if (inString) {
            result += c;
            if (c == '\\' && i + 1 < input.size()) {
                result += input[++i]; // skip escaped char
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
            ++i; // skip '/'
        } else if (c == '/' && next == '*') {
            inMultiLineComment = true;
            ++i; // skip '*'
        } else {
            result += c;
        }
    }
    return result;
}

bool ResourceManager::patchItemTextureJson(std::string& inOutJson) const {
    try {
        nlohmann::json root;
        if (inOutJson.empty()) {
            root = nlohmann::json::object();
            root["resource_pack_name"] = "vanilla";
            root["texture_name"] = "atlas.items";
            root["texture_data"] = nlohmann::json::object();
        } else {
            std::string clean = stripJsonComments(inOutJson);
            root = nlohmann::json::parse(clean, nullptr, false, true);
            if (root.is_discarded()) {
                root = nlohmann::json::object();
                root["resource_pack_name"] = "vanilla";
                root["texture_name"] = "atlas.items";
                root["texture_data"] = nlohmann::json::object();
            }
        }

        root["resource_pack_name"] = "vanilla";
        root["texture_name"] = "atlas.items";

        if (!root.contains("texture_data") || !root["texture_data"].is_object()) {
            root["texture_data"] = nlohmann::json::object();
        }

        std::lock_guard<std::mutex> lock(mMutex);

        // 1. Merge all mounted item_texture.json files directly
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.find("item_texture.json") != std::string::npos) {
                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded() && customJson.contains("texture_data") && customJson["texture_data"].is_object()) {
                            for (auto& [key, val] : customJson["texture_data"].items()) {
                                root["texture_data"][key] = val;
                                ll::Logger::getInstance().debug("ResourceManager: Merged custom item_texture entry: ", key);
                            }
                        }
                    } catch (...) {}
                }
            }
        }

        // 2. Iterate mounted textures/items/ to auto-generate entries if missing
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.rfind("textures/items/", 0) == 0 && rel.size() > 19) {
                std::string stem = std::filesystem::path(rel).stem().string();
                std::string texPath = "textures/items/" + stem;

                nlohmann::json texEntry;
                texEntry["textures"] = texPath;
                if (!root["texture_data"].contains(stem)) {
                    root["texture_data"][stem] = texEntry;
                }
                ll::Logger::getInstance().info("ResourceManager: Injected custom texture into atlas: ", stem, " -> ", texPath);
            }
        }

        // 3. Fallback textures for blocks so 2D UI renderers always find them
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.rfind("textures/blocks/", 0) == 0 && rel.size() > 20) {
                std::string stem = std::filesystem::path(rel).stem().string();
                std::string texPath = "textures/blocks/" + stem;

                nlohmann::json texEntry;
                texEntry["textures"] = texPath;
                if (!root["texture_data"].contains(stem)) {
                    root["texture_data"][stem] = texEntry;
                }
                if (!root["texture_data"].contains("levitest:" + stem)) {
                    root["texture_data"]["levitest:" + stem] = texEntry;
                }
            }
        }

        inOutJson = root.dump(2);
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch item_texture.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchTerrainTextureJson(std::string& inOutJson) const {
    try {
        nlohmann::json root;
        if (inOutJson.empty()) {
            root = nlohmann::json::object();
            root["resource_pack_name"] = "vanilla";
            root["texture_name"] = "atlas.terrain";
            root["padding"] = 8;
            root["num_mip_levels"] = 4;
            root["texture_data"] = nlohmann::json::object();
        } else {
            std::string clean = stripJsonComments(inOutJson);
            root = nlohmann::json::parse(clean, nullptr, false, true);
            if (root.is_discarded()) {
                root = nlohmann::json::object();
                root["resource_pack_name"] = "vanilla";
                root["texture_name"] = "atlas.terrain";
                root["padding"] = 8;
                root["num_mip_levels"] = 4;
                root["texture_data"] = nlohmann::json::object();
            }
        }

        root["resource_pack_name"] = "vanilla";
        root["texture_name"] = "atlas.terrain";

        if (!root.contains("texture_data") || !root["texture_data"].is_object()) {
            root["texture_data"] = nlohmann::json::object();
        }

        std::lock_guard<std::mutex> lock(mMutex);

        // 1. Merge all mounted terrain_texture.json files directly
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.find("terrain_texture.json") != std::string::npos) {
                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded() && customJson.contains("texture_data") && customJson["texture_data"].is_object()) {
                            for (auto& [key, val] : customJson["texture_data"].items()) {
                                root["texture_data"][key] = val;
                                ll::Logger::getInstance().debug("ResourceManager: Merged custom terrain_texture entry: ", key);
                            }
                        }
                    } catch (...) {}
                }
            }
        }

        // 2. Iterate mounted textures/blocks/ to auto-generate entries if missing
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.rfind("textures/blocks/", 0) == 0 && rel.size() > 20) {
                std::string stem = std::filesystem::path(rel).stem().string();
                std::string texPath = "textures/blocks/" + stem;

                nlohmann::json texEntry;
                texEntry["textures"] = texPath;
                if (!root["texture_data"].contains(stem)) {
                    root["texture_data"][stem] = texEntry;
                }
                if (!root["texture_data"].contains("levitest:" + stem)) {
                    root["texture_data"]["levitest:" + stem] = texEntry;
                }
                ll::Logger::getInstance().info("ResourceManager: Injected custom block texture into atlas: ", stem, " -> ", texPath);
            }
        }

        inOutJson = root.dump(2);
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch terrain_texture.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchBlocksJson(std::string& inOutJson) const {
    try {
        nlohmann::json root;
        if (inOutJson.empty()) {
            root = nlohmann::json::object();
            root["format_version"] = nlohmann::json::array({1, 1, 0});
        } else {
            std::string clean = stripJsonComments(inOutJson);
            root = nlohmann::json::parse(clean, nullptr, false, true);
            if (root.is_discarded()) {
                ll::Logger::getInstance().warn("ResourceManager: blocks.json parse discarded after stripping comments; initializing fresh object");
                root = nlohmann::json::object();
                root["format_version"] = nlohmann::json::array({1, 1, 0});
            }
        }

        if (!root.contains("format_version")) {
            root["format_version"] = nlohmann::json::array({1, 1, 0});
        }

        std::lock_guard<std::mutex> lock(mMutex);

        // 1. Merge all mounted blocks.json files directly
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.find("blocks.json") != std::string::npos) {
                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded()) {
                            for (auto& [key, val] : customJson.items()) {
                                if (key != "format_version" && key != "minecraft:stone" && key != "stone") {
                                    root[key] = val;
                                    ll::Logger::getInstance().debug("ResourceManager: Merged custom blocks.json entry: ", key);
                                }
                            }
                        }
                    } catch (...) {}
                }
            }
        }

        // 2. Auto-generate block entries for all mounted textures/blocks/
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.rfind("textures/blocks/", 0) == 0 && rel.size() > 20) {
                std::string stem = std::filesystem::path(rel).stem().string();
                nlohmann::json blockEntry;
                blockEntry["textures"] = stem;
                blockEntry["sound"] = (stem.find("table") != std::string::npos || stem.find("wood") != std::string::npos) ? "wood" : "stone";

                if (!root.contains(stem)) {
                    root[stem] = blockEntry;
                }
                if (!root.contains("levitest:" + stem)) {
                    root["levitest:" + stem] = blockEntry;
                }
                ll::Logger::getInstance().info("ResourceManager: Injected custom block into blocks.json: ", stem);
            }
        }

        inOutJson = root.dump(2);
        ll::Logger::getInstance().info("ResourceManager: Successfully patched blocks.json with custom blocks!");
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch blocks.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchLangFile(std::string& inOutLang) const {
    std::lock_guard<std::mutex> lock(mMutex);
    bool added = false;
    std::error_code ec;

    // Scan all mounted directories for every .lang file to ensure all mod translations are merged
    for (const auto& dir : mMountedDirectories) {
        if (!std::filesystem::exists(dir, ec)) continue;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (entry.is_regular_file(ec) && entry.path().extension().string() == ".lang") {
                std::ifstream f(entry.path());
                if (f.is_open()) {
                    std::string line;
                    if (!inOutLang.empty() && inOutLang.back() != '\n') {
                        inOutLang.push_back('\n');
                    }
                    while (std::getline(f, line)) {
                        if (!line.empty() && line[0] != '#') {
                            inOutLang += line + "\n";
                            added = true;
                        }
                    }
                }
            }
        }
    }

    // Also scan mAssetCache for any registered .lang files
    for (const auto& [rel, path] : mAssetCache) {
        if (rel.find(".lang") != std::string::npos) {
            std::ifstream f(path);
            if (f.is_open()) {
                std::string line;
                if (!inOutLang.empty() && inOutLang.back() != '\n') {
                    inOutLang.push_back('\n');
                }
                while (std::getline(f, line)) {
                    if (!line.empty() && line[0] != '#') {
                        inOutLang += line + "\n";
                        added = true;
                    }
                }
            }
        }
    }
    return added;
}

bool ResourceManager::patchUiDefsJson(std::string& inOutJson) const {
    try {
        nlohmann::json root;
        if (inOutJson.empty()) {
            root = nlohmann::json::object();
            root["ui_defs"] = nlohmann::json::array();
        } else {
            std::string clean = stripJsonComments(inOutJson);
            root = nlohmann::json::parse(clean, nullptr, false, true);
            if (root.is_discarded() || !root.contains("ui_defs") || !root["ui_defs"].is_array()) {
                root = nlohmann::json::object();
                root["ui_defs"] = nlohmann::json::array();
            }
        }

        std::lock_guard<std::mutex> lock(mMutex);
        std::unordered_set<std::string> existingDefs;
        for (const auto& el : root["ui_defs"]) {
            if (el.is_string()) {
                existingDefs.insert(el.get<std::string>());
            }
        }

        std::error_code ec;
        // 1. Discover custom _ui_defs.json files from all mounted directories
        for (const auto& dir : mMountedDirectories) {
            if (!std::filesystem::exists(dir, ec)) continue;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
                if (entry.is_regular_file(ec)) {
                    std::string fn = entry.path().filename().string();
                    if (fn == "_ui_defs.json") {
                        std::ifstream f(entry.path());
                        if (f.is_open()) {
                            try {
                                std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                                auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                                if (!customJson.is_discarded() && customJson.contains("ui_defs") && customJson["ui_defs"].is_array()) {
                                    for (const auto& defPath : customJson["ui_defs"]) {
                                        if (defPath.is_string()) {
                                            std::string defStr = defPath.get<std::string>();
                                            if (existingDefs.find(defStr) == existingDefs.end()) {
                                                root["ui_defs"].push_back(defStr);
                                                existingDefs.insert(defStr);
                                                ll::Logger::getInstance().info("ResourceManager: Injected custom UI def from _ui_defs.json: ", defStr);
                                            }
                                        }
                                    }
                                }
                            } catch (...) {}
                        }
                    }
                }
            }
        }

        // 2. Discover and register individual screen json files under ui/
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.rfind("ui/", 0) == 0 && rel.size() > 8 && rel.find("_ui_defs") == std::string::npos &&
                rel.substr(rel.size() - 5) == ".json") {
                if (existingDefs.find(rel) == existingDefs.end()) {
                    root["ui_defs"].push_back(rel);
                    existingDefs.insert(rel);
                    ll::Logger::getInstance().info("ResourceManager: Auto-injected mounted UI screen into ui_defs: ", rel);
                }
            }
        }

        inOutJson = root.dump(2);
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch _ui_defs.json: ", e.what());
        return false;
    }
}

void ResourceManager::registerAsset(const std::string& relativePath, const std::filesystem::path& diskPath) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::string clean = relativePath;
    std::replace(clean.begin(), clean.end(), '\\', '/');
    mAssetCache[clean] = diskPath;
}

std::vector<std::filesystem::path> ResourceManager::getMountedDirectories() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mMountedDirectories;
}

void ResourceManager::forEachAssetMatching(std::string_view prefix, const std::function<void(const std::string& relPath)>& callback) const {
    if (!callback) return;
    std::lock_guard<std::mutex> lock(mMutex);
    std::string p(prefix);
    if (!p.empty() && (p[0] == '/' || p[0] == '\\')) p = p.substr(1);
    while (!p.empty() && (p.back() == '/' || p.back() == '\\')) p.pop_back();

    for (const auto& [rel, diskPath] : mAssetCache) {
        if (rel.rfind(p, 0) == 0) {
            callback(rel);
        }
    }
}

} // namespace ll::resource

extern "C" {

LL_API void ll_resource_mount_dir(const char* dir) {
    if (dir) {
        ::ll::resource::ResourceManager::getInstance().mountDirectory(dir);
    }
}

LL_API bool ll_resource_has_asset(const char* relativePath) {
    if (!relativePath) return false;
    return ::ll::resource::ResourceManager::getInstance().hasAsset(relativePath);
}

LL_API bool ll_resource_get_asset(const char* relativePath, char** outBuffer, size_t* outSize) {
    if (!relativePath || !outBuffer || !outSize) return false;
    std::string data;
    if (::ll::resource::ResourceManager::getInstance().getAsset(relativePath, data)) {
        *outSize = data.size();
        *outBuffer = static_cast<char*>(malloc(data.size() + 1));
        std::memcpy(*outBuffer, data.data(), data.size());
        (*outBuffer)[data.size()] = '\0';
        return true;
    }
    return false;
}

LL_API void ll_resource_free_buffer(char* buffer) {
    if (buffer) {
        free(buffer);
    }
}

} // extern "C"
