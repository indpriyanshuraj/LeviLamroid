#include "ll/api/resource/ResourceManager.h"
#include "levilamroid/api/Logger.h"
#include "levilamroid/api/item/ItemRegistry.h"
#include "levilamroid/api/block/BlockTypeRegistry.h"
#include "AddonIngestion.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <dlfcn.h>

namespace ll::resource {

void ResourceManager::mountDirectory(const std::filesystem::path& dir) {
    std::lock_guard<std::mutex> lock(mMutex);
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
        ll::Logger::getInstance().warn("ResourceManager: Directory does not exist: ", dir.string());
        return;
    }

    for (const auto& existing : mMountedDirectories) {
        if (existing == dir) return;
        std::error_code eqEc;
        if (std::filesystem::equivalent(existing, dir, eqEc)) return;
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
            if (rel.size() > 4 && rel.substr(rel.size() - 4) == ".png") {
                mAssetCache[rel.substr(0, rel.size() - 4)] = entry.path();
            }

            // Also index stripped relative paths for resource pack subfolders
            static const char* sPrefixes[] = {
                "textures/", "texts/", "models/", "ui/", "materials/", "particles/",
                "sounds/", "animations/", "render_controllers/", "attachables/", "entity/", "font/"
            };
            for (const char* pfx : sPrefixes) {
                auto pos = rel.find(pfx);
                if (pos != std::string::npos && pos > 0) {
                    auto stripped = rel.substr(pos);
                    mAssetCache[stripped] = entry.path();
                    if (stripped.size() > 4 && stripped.substr(stripped.size() - 4) == ".png") {
                        mAssetCache[stripped.substr(0, stripped.size() - 4)] = entry.path();
                    }
                }
            }

            auto filename = entry.path().filename().string();
            mAssetCache[filename] = entry.path();
            if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".png") {
                mAssetCache[filename.substr(0, filename.size() - 4)] = entry.path();
            }

            ll::Logger::getInstance().debug("ResourceManager: Indexed raw asset: ", rel);
        }
    }
}

bool ResourceManager::hasAsset(std::string_view relativePath) const {
    if (relativePath.empty()) return false;

    std::string cleanPath(relativePath);
    if (cleanPath[0] == '/' || cleanPath[0] == '\\') {
        cleanPath = cleanPath.substr(1);
    }
    std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

    std::lock_guard<std::mutex> lock(mMutex);
    if (mAssetCache.empty()) return false;

    if (mAssetCache.find(cleanPath) != mAssetCache.end()) return true;

    auto texPos = cleanPath.find("textures/");
    if (texPos != std::string::npos && texPos > 0) {
        if (mAssetCache.find(cleanPath.substr(texPos)) != mAssetCache.end()) return true;
    }

    auto slash = cleanPath.rfind('/');
    if (slash != std::string::npos && slash + 1 < cleanPath.size()) {
        std::string fname = cleanPath.substr(slash + 1);
        if (mAssetCache.find(fname) != mAssetCache.end()) return true;
    }

    return false;
}

bool ResourceManager::getAsset(std::string_view relativePath, std::string& outBuffer) const {
    if (relativePath.empty()) return false;

    std::string cleanPath(relativePath);
    if (cleanPath[0] == '/' || cleanPath[0] == '\\') {
        cleanPath = cleanPath.substr(1);
    }
    std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

    std::filesystem::path targetFile;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mAssetCache.empty()) return false;

        auto it = mAssetCache.find(cleanPath);

        if (it == mAssetCache.end()) {
            auto texPos = cleanPath.find("textures/");
            if (texPos != std::string::npos && texPos > 0) {
                it = mAssetCache.find(cleanPath.substr(texPos));
            }
        }

        if (it == mAssetCache.end()) {
            auto slash = cleanPath.rfind('/');
            if (slash != std::string::npos && slash + 1 < cleanPath.size()) {
                it = mAssetCache.find(cleanPath.substr(slash + 1));
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

struct TextureJsonCache {
    std::string cachedJson;
    size_t lastInputHash = 0;
    size_t lastCount = 0;
    size_t lastAssetCount = 0;
};
static TextureJsonCache sItemTexCache;
static std::mutex sItemTexCacheMutex;

static TextureJsonCache sTerrainTexCache;
static std::mutex sTerrainTexCacheMutex;

bool ResourceManager::patchItemTextureJson(std::string& inOutJson) const {
    if (inOutJson.empty()) return false;

    size_t inHash = std::hash<std::string_view>{}(inOutJson);
    size_t customItemCount = ::levilamroid::item::getAllCustomItems().size();
    {
        std::lock_guard<std::mutex> cacheLock(sItemTexCacheMutex);
        std::lock_guard<std::mutex> assetLock(mMutex);
        if (inHash == sItemTexCache.lastInputHash &&
            customItemCount == sItemTexCache.lastCount &&
            mAssetCache.size() == sItemTexCache.lastAssetCount &&
            !sItemTexCache.cachedJson.empty()) {
            inOutJson = sItemTexCache.cachedJson;
            return true;
        }
    }

    try {
        std::string clean = stripJsonComments(inOutJson);
        auto root = nlohmann::json::parse(clean, nullptr, false, true);
        if (root.is_discarded()) {
            ll::Logger::getInstance().warn("ResourceManager: item_texture.json parse discarded; preserving raw vanilla file");
            return false;
        }

        root["resource_pack_name"] = "vanilla";
        root["texture_name"] = "atlas.items";

        if (!root.contains("texture_data") || !root["texture_data"].is_object()) {
            root["texture_data"] = nlohmann::json::object();
        }

        std::lock_guard<std::mutex> lock(mMutex);

        // 1. Merge all mounted item_texture.json files directly (deduplicated by path)
        std::unordered_set<std::string> loadedItemTex;
        for (const auto& [rel, path] : mAssetCache) {
            if (rel == "item_texture.json" || (rel.size() > 17 && rel.substr(rel.size() - 17) == "item_texture.json")) {
                std::string pStr = path.string();
                if (loadedItemTex.count(pStr)) continue;
                loadedItemTex.insert(pStr);

                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded() && customJson.contains("texture_data") && customJson["texture_data"].is_object()) {
                            for (auto& [key, val] : customJson["texture_data"].items()) {
                                root["texture_data"][key] = val;
                                auto colon = key.find(':');
                                if (colon != std::string::npos) {
                                    std::string unnamespaced = key.substr(colon + 1);
                                    if (!root["texture_data"].contains(unnamespaced)) {
                                        root["texture_data"][unnamespaced] = val;
                                    }
                                }
                                ll::Logger::getInstance().debug("ResourceManager: Merged custom item_texture entry: ", key);
                            }
                        }
                    } catch (...) {}
                }
            }
        }

        // 2. Iterate mounted textures/items/ and textures/blocks/ preserving actual relative paths
        for (const auto& [rel, path] : mAssetCache) {
            bool isItem = (rel.rfind("textures/items/", 0) == 0 && rel.size() > 15);
            bool isBlock = (rel.rfind("textures/blocks/", 0) == 0 && rel.size() > 16);
            if (isItem || isBlock) {
                std::string stem = std::filesystem::path(rel).stem().string();
                std::string texPath = rel;
                if (texPath.size() > 4 && texPath.substr(texPath.size() - 4) == ".png") {
                    texPath = texPath.substr(0, texPath.size() - 4);
                }

                nlohmann::json texEntry;
                texEntry["textures"] = texPath;
                if (!root["texture_data"].contains(stem)) {
                    root["texture_data"][stem] = texEntry;
                }
            }
        }

        // 3. For EVERY registered custom item, ensure full identifier, shortName, and iconName are mapped
        for (const auto& it : ::levilamroid::item::getAllCustomItems()) {
            auto colon = it.identifier.find(':');
            std::string shortName = (colon != std::string::npos) ? it.identifier.substr(colon + 1) : it.identifier;
            std::string iconClean = it.iconName;
            auto iconColon = iconClean.find(':');
            std::string iconShort = (iconColon != std::string::npos) ? iconClean.substr(iconColon + 1) : iconClean;

            std::string matchedPath = "";
            const char* candidates[] = {
                it.iconName.c_str(), iconShort.c_str(), shortName.c_str(), it.identifier.c_str()
            };
            for (const char* cand : candidates) {
                if (!cand || !cand[0]) continue;
                std::string itemP = std::string("textures/items/") + cand;
                std::string blockP = std::string("textures/blocks/") + cand;
                if (mAssetCache.count(itemP + ".png") || mAssetCache.count(itemP)) {
                    matchedPath = itemP; break;
                }
                if (mAssetCache.count(blockP + ".png") || mAssetCache.count(blockP)) {
                    matchedPath = blockP; break;
                }
                auto itFound = mAssetCache.find(std::string(cand) + ".png");
                if (itFound == mAssetCache.end()) itFound = mAssetCache.find(cand);
                if (itFound != mAssetCache.end()) {
                    std::string pStr = itFound->first;
                    if (pStr.size() > 4 && pStr.substr(pStr.size() - 4) == ".png") {
                        pStr = pStr.substr(0, pStr.size() - 4);
                    }
                    if (pStr.rfind("textures/", 0) == 0) {
                        matchedPath = pStr;
                        break;
                    }
                }
            }

            if (matchedPath.empty()) {
                matchedPath = "textures/items/" + shortName;
            }

            nlohmann::json texEntry;
            texEntry["textures"] = matchedPath;

            if (!root["texture_data"].contains(it.identifier)) {
                root["texture_data"][it.identifier] = texEntry;
            }
            if (!root["texture_data"].contains(shortName)) {
                root["texture_data"][shortName] = texEntry;
            }
            if (!it.iconName.empty() && !root["texture_data"].contains(it.iconName)) {
                root["texture_data"][it.iconName] = texEntry;
            }
            if (!iconShort.empty() && !root["texture_data"].contains(iconShort)) {
                root["texture_data"][iconShort] = texEntry;
            }
        }

        inOutJson = root.dump();
        {
            std::lock_guard<std::mutex> cacheLock(sItemTexCacheMutex);
            sItemTexCache.cachedJson = inOutJson;
            sItemTexCache.lastInputHash = inHash;
            sItemTexCache.lastCount = customItemCount;
            sItemTexCache.lastAssetCount = mAssetCache.size();
        }
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch item_texture.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchTerrainTextureJson(std::string& inOutJson) const {
    if (inOutJson.empty()) return false;

    size_t inHash = std::hash<std::string_view>{}(inOutJson);
    size_t customBlockCount = ::levilamroid::block::getRegisteredCustomBlocks().size();
    {
        std::lock_guard<std::mutex> cacheLock(sTerrainTexCacheMutex);
        std::lock_guard<std::mutex> assetLock(mMutex);
        if (inHash == sTerrainTexCache.lastInputHash &&
            customBlockCount == sTerrainTexCache.lastCount &&
            mAssetCache.size() == sTerrainTexCache.lastAssetCount &&
            !sTerrainTexCache.cachedJson.empty()) {
            inOutJson = sTerrainTexCache.cachedJson;
            return true;
        }
    }

    try {
        std::string clean = stripJsonComments(inOutJson);
        auto root = nlohmann::json::parse(clean, nullptr, false, true);
        if (root.is_discarded()) {
            ll::Logger::getInstance().warn("ResourceManager: terrain_texture.json parse discarded; preserving raw vanilla file");
            return false;
        }

        root["resource_pack_name"] = "vanilla";
        root["texture_name"] = "atlas.terrain";

        if (!root.contains("texture_data") || !root["texture_data"].is_object()) {
            root["texture_data"] = nlohmann::json::object();
        }

        std::lock_guard<std::mutex> lock(mMutex);

        // 1. Merge all mounted terrain_texture.json files directly (deduplicated by path)
        std::unordered_set<std::string> loadedTerrainTex;
        for (const auto& [rel, path] : mAssetCache) {
            if (rel == "terrain_texture.json" || (rel.size() > 20 && rel.substr(rel.size() - 20) == "terrain_texture.json")) {
                std::string pStr = path.string();
                if (loadedTerrainTex.count(pStr)) continue;
                loadedTerrainTex.insert(pStr);

                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded() && customJson.contains("texture_data") && customJson["texture_data"].is_object()) {
                            size_t mergedCount = 0;
                            for (auto& [key, val] : customJson["texture_data"].items()) {
                                root["texture_data"][key] = val;
                                auto colon = key.find(':');
                                if (colon != std::string::npos) {
                                    std::string unnamespaced = key.substr(colon + 1);
                                    if (!root["texture_data"].contains(unnamespaced)) {
                                        root["texture_data"][unnamespaced] = val;
                                    }
                                }
                                mergedCount++;
                            }
                            if (mergedCount > 0) {
                                ll::Logger::getInstance().info("ResourceManager: Merged ", mergedCount, " custom terrain_texture entries.");
                            }
                        }
                    } catch (...) {}
                }
            }
        }

        // 2. Iterate mounted textures/blocks/ preserving actual relative paths
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.rfind("textures/blocks/", 0) == 0 && rel.size() > 16) {
                std::string stem = std::filesystem::path(rel).stem().string();
                std::string texPath = rel;
                if (texPath.size() > 4 && texPath.substr(texPath.size() - 4) == ".png") {
                    texPath = texPath.substr(0, texPath.size() - 4);
                }

                nlohmann::json texEntry;
                texEntry["textures"] = texPath;
                if (!root["texture_data"].contains(stem)) {
                    root["texture_data"][stem] = texEntry;
                }
                for (const auto& b : ::levilamroid::block::getRegisteredCustomBlocks()) {
                    auto colon = b.identifier.find(':');
                    std::string shortName = (colon != std::string::npos) ? b.identifier.substr(colon + 1) : b.identifier;
                    if (shortName == stem || b.textureName == stem) {
                        if (!root["texture_data"].contains(b.identifier)) {
                            root["texture_data"][b.identifier] = texEntry;
                        }
                    }
                }
            }
        }

        // 3. For EVERY registered custom block, ensure identifier, shortName, and textureName are mapped in atlas.terrain
        for (const auto& b : ::levilamroid::block::getRegisteredCustomBlocks()) {
            auto colon = b.identifier.find(':');
            std::string shortName = (colon != std::string::npos) ? b.identifier.substr(colon + 1) : b.identifier;
            std::string texClean = b.textureName;
            auto texColon = texClean.find(':');
            std::string texShort = (texColon != std::string::npos) ? texClean.substr(texColon + 1) : texClean;

            std::string matchedPath = "";
            const char* candidates[] = {
                b.textureName.c_str(), texShort.c_str(), shortName.c_str(), b.identifier.c_str()
            };
            for (const char* cand : candidates) {
                if (!cand || !cand[0]) continue;
                std::string blockP = std::string("textures/blocks/") + cand;
                std::string itemP = std::string("textures/items/") + cand;
                if (mAssetCache.count(blockP + ".png") || mAssetCache.count(blockP)) {
                    matchedPath = blockP; break;
                }
                if (mAssetCache.count(itemP + ".png") || mAssetCache.count(itemP)) {
                    matchedPath = itemP; break;
                }
                auto itFound = mAssetCache.find(std::string(cand) + ".png");
                if (itFound == mAssetCache.end()) itFound = mAssetCache.find(cand);
                if (itFound != mAssetCache.end()) {
                    std::string pStr = itFound->first;
                    if (pStr.size() > 4 && pStr.substr(pStr.size() - 4) == ".png") {
                        pStr = pStr.substr(0, pStr.size() - 4);
                    }
                    if (pStr.rfind("textures/", 0) == 0) {
                        matchedPath = pStr;
                        break;
                    }
                }
            }

            if (matchedPath.empty()) {
                matchedPath = "textures/blocks/" + (!texShort.empty() ? texShort : shortName);
            }

            nlohmann::json texEntry;
            texEntry["textures"] = matchedPath;

            if (!root["texture_data"].contains(b.identifier)) {
                root["texture_data"][b.identifier] = texEntry;
            }
            if (!root["texture_data"].contains(shortName)) {
                root["texture_data"][shortName] = texEntry;
            }
            if (!b.textureName.empty() && !root["texture_data"].contains(b.textureName)) {
                root["texture_data"][b.textureName] = texEntry;
            }
            if (!texShort.empty() && !root["texture_data"].contains(texShort)) {
                root["texture_data"][texShort] = texEntry;
            }
        }

        inOutJson = root.dump();
        {
            std::lock_guard<std::mutex> cacheLock(sTerrainTexCacheMutex);
            sTerrainTexCache.cachedJson = inOutJson;
            sTerrainTexCache.lastInputHash = inHash;
            sTerrainTexCache.lastCount = customBlockCount;
            sTerrainTexCache.lastAssetCount = mAssetCache.size();
        }
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch terrain_texture.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchBlocksJson(std::string& inOutJson) const {
    if (inOutJson.empty()) return false;
    try {
        std::string clean = stripJsonComments(inOutJson);
        auto root = nlohmann::json::parse(clean, nullptr, false, true);
        if (root.is_discarded()) {
            ll::Logger::getInstance().warn("ResourceManager: blocks.json parse discarded; preserving vanilla blocks");
            return false;
        }

        if (!root.contains("format_version")) {
            root["format_version"] = nlohmann::json::array({1, 1, 0});
        }

        std::lock_guard<std::mutex> lock(mMutex);

        // Merge only genuine mounted blocks.json files directly (deduplicated by path)
        // CRITICAL: NEVER auto-generate fake block entries from textures/blocks/!
        // Injecting hundreds of non-existent blocks causes Bedrock's asynchronous block loading
        // worker threads to stall indefinitely during world load waiting for missing definitions.
        std::unordered_set<std::string> loadedBlocksJson;
        size_t mergedCount = 0;
        for (const auto& [rel, path] : mAssetCache) {
            if (rel == "blocks.json" || (rel.size() > 11 && rel.substr(rel.size() - 11) == "blocks.json")) {
                std::string pStr = path.string();
                if (loadedBlocksJson.count(pStr)) continue;
                loadedBlocksJson.insert(pStr);

                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded()) {
                            for (auto& [key, val] : customJson.items()) {
                                if (key != "format_version" && key != "minecraft:stone" && key != "stone") {
                                    root[key] = val;
                                    mergedCount++;
                                }
                            }
                        }
                    } catch (...) {}
                }
            }
        }

        // 2. Ensure every registered custom block has an authentic entry in blocks.json
        for (const auto& b : ::levilamroid::block::getRegisteredCustomBlocks()) {
            auto colon = b.identifier.find(':');
            std::string shortName = (colon != std::string::npos) ? b.identifier.substr(colon + 1) : b.identifier;
            std::string tex = !b.textureName.empty() ? b.textureName : shortName;

            nlohmann::json blockEntry;
            blockEntry["textures"] = tex;
            blockEntry["sound"] = ::levilamroid::block::getPlacementSoundForBlock(b.identifier);

            if (!root.contains(b.identifier)) {
                root[b.identifier] = blockEntry;
                mergedCount++;
            }
            if (!root.contains(shortName)) {
                root[shortName] = blockEntry;
            }
        }

        inOutJson = root.dump();
        ll::Logger::getInstance().info("ResourceManager: Successfully patched blocks.json (merged ", mergedCount, " authentic custom block definitions).");
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch blocks.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchLangFile(std::string& inOutLang, std::string_view reqPath) const {
    std::lock_guard<std::mutex> lock(mMutex);
    bool added = false;

    // Determine target locale from reqPath (e.g. "texts/en_US.lang" -> "en_US.lang")
    std::string targetLocale = "en_US.lang";
    auto slash = reqPath.rfind('/');
    if (slash == std::string_view::npos) slash = reqPath.rfind('\\');
    if (slash != std::string_view::npos) {
        targetLocale = std::string(reqPath.substr(slash + 1));
    } else if (!reqPath.empty()) {
        targetLocale = std::string(reqPath);
    }

    // Try to find the matching .lang file; if not found, fallback to en_US.lang
    std::filesystem::path selectedLangPath;
    for (const auto& [rel, path] : mAssetCache) {
        if (rel.size() >= targetLocale.size() && rel.compare(rel.size() - targetLocale.size(), targetLocale.size(), targetLocale) == 0) {
            selectedLangPath = path;
            break;
        }
    }
    if (selectedLangPath.empty()) {
        for (const auto& [rel, path] : mAssetCache) {
            if (rel.size() >= 10 && rel.compare(rel.size() - 10, 10, "en_US.lang") == 0) {
                selectedLangPath = path;
                break;
            }
        }
    }

    if (!selectedLangPath.empty()) {
        std::ifstream f(selectedLangPath);
        if (f.is_open()) {
            std::string line;
            if (!inOutLang.empty() && inOutLang.back() != '\n') {
                inOutLang.push_back('\n');
            }
            while (std::getline(f, line)) {
                if (!line.empty() && line[0] != '#') {
                    inOutLang += line + "\n";
                    added = true;

                    // Bidirectional translation mapping:
                    // Bedrock HUD displays item.<id>.name for held items, whereas
                    // custom blocks define tile.<id>.name. Synthesizing both forms
                    // eliminates raw identifier strings in the HUD and inventory.
                    if (line.rfind("tile.", 0) == 0) {
                        inOutLang += "item." + line.substr(5) + "\n";
                    } else if (line.rfind("item.", 0) == 0) {
                        inOutLang += "tile." + line.substr(5) + "\n";
                    }
                }
            }
        }
    }

    auto beautifyName = [](std::string_view ident) -> std::string {
        auto colon = ident.find(':');
        std::string s = (colon != std::string_view::npos) ? std::string(ident.substr(colon + 1)) : std::string(ident);
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
        return s;
    };

    if (!inOutLang.empty() && inOutLang.back() != '\n') {
        inOutLang.push_back('\n');
    }

    // Synthesize display names for all registered custom items
    for (const auto& it : ::levilamroid::item::getAllCustomItems()) {
        std::string name = !it.displayName.empty() ? it.displayName : beautifyName(it.identifier);
        auto colon = it.identifier.find(':');
        std::string shortName = (colon != std::string::npos) ? it.identifier.substr(colon + 1) : it.identifier;

        inOutLang += "item." + it.identifier + ".name=" + name + "\n";
        inOutLang += "item." + shortName + ".name=" + name + "\n";
        inOutLang += "tile." + it.identifier + ".name=" + name + "\n";
        inOutLang += "tile." + shortName + ".name=" + name + "\n";
        if (!it.descriptionId.empty()) {
            inOutLang += it.descriptionId + "=" + name + "\n";
            if (it.descriptionId.find(".name") == std::string::npos) {
                inOutLang += it.descriptionId + ".name=" + name + "\n";
            }
        }
        inOutLang += it.identifier + "=" + name + "\n";
        added = true;
    }

    // Synthesize display names for all registered custom blocks
    for (const auto& b : ::levilamroid::block::getRegisteredCustomBlocks()) {
        std::string name = !b.displayName.empty() ? b.displayName : beautifyName(b.identifier);
        auto colon = b.identifier.find(':');
        std::string shortName = (colon != std::string::npos) ? b.identifier.substr(colon + 1) : b.identifier;

        inOutLang += "tile." + b.identifier + ".name=" + name + "\n";
        inOutLang += "tile." + shortName + ".name=" + name + "\n";
        inOutLang += "item." + b.identifier + ".name=" + name + "\n";
        inOutLang += "item." + shortName + ".name=" + name + "\n";
        inOutLang += b.identifier + "=" + name + "\n";
        added = true;
    }

    return added;
}

static bool isVanillaUiScreen(std::string_view path) {
    static const std::string_view sVanillaScreens[] = {
        "hud_screen",
        "pause_screen",
        "chest_screen",
        "redstone_screen",
        "inventory_screen",
        "chat_screen",
        "death_screen",
        "start_screen",
        "settings_screen",
        "progress_screen",
        "world_loading_screen",
        "common",
        "ui_common",
        "ui_template"
    };
    for (auto v : sVanillaScreens) {
        if (path.find(v) != std::string_view::npos) return true;
    }
    return false;
}

bool ResourceManager::patchUiDefsJson(std::string& inOutJson) const {
    if (inOutJson.empty()) return false;

    // Collect all authentic custom UI definitions across mounted folders
    std::vector<std::string> customDefsToInject;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        std::unordered_set<std::string> parsedUiDefs;
        for (const auto& [rel, path] : mAssetCache) {
            if (rel == "_ui_defs.json" || (rel.size() > 13 && rel.substr(rel.size() - 13) == "_ui_defs.json")) {
                std::string pStr = path.string();
                if (parsedUiDefs.count(pStr)) continue;
                parsedUiDefs.insert(pStr);

                std::ifstream f(path);
                if (f.is_open()) {
                    try {
                        std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                        auto customJson = nlohmann::json::parse(stripJsonComments(content), nullptr, false, true);
                        if (!customJson.is_discarded() && customJson.contains("ui_defs") && customJson["ui_defs"].is_array()) {
                            for (const auto& defPath : customJson["ui_defs"]) {
                                if (defPath.is_string()) {
                                    std::string defStr = defPath.get<std::string>();
                                    if (!isVanillaUiScreen(defStr) && !defStr.empty()) {
                                        customDefsToInject.push_back(defStr);
                                    }
                                }
                            }
                        }
                    } catch (...) {}
                }
            }
        }
    }

    // If no authentic custom UI screens exist, leave inOutJson 100% untouched to protect vanilla engine screens
    if (customDefsToInject.empty()) {
        return true;
    }

    try {
        std::string clean = stripJsonComments(inOutJson);
        auto root = nlohmann::json::parse(clean, nullptr, false, true);
        if (root.is_discarded() || !root.contains("ui_defs") || !root["ui_defs"].is_array()) {
            return false;
        }

        std::unordered_set<std::string> existingDefs;
        for (const auto& el : root["ui_defs"]) {
            if (el.is_string()) {
                existingDefs.insert(el.get<std::string>());
            }
        }

        bool injectedAny = false;
        for (const auto& defStr : customDefsToInject) {
            if (existingDefs.find(defStr) == existingDefs.end()) {
                root["ui_defs"].push_back(defStr);
                existingDefs.insert(defStr);
                injectedAny = true;
                ll::Logger::getInstance().info("ResourceManager: Injected custom UI def from _ui_defs.json: ", defStr);
            }
        }

        if (injectedAny) {
            inOutJson = root.dump();
        }
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch _ui_defs.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchSoundDefinitionsJson(std::string& inOutJson) const {
    try {
        nlohmann::json root;
        if (inOutJson.empty()) return false;
        std::string clean = stripJsonComments(inOutJson);
        root = nlohmann::json::parse(clean, nullptr, false, true);
        if (root.is_discarded() || !root.contains("sound_definitions") || !root["sound_definitions"].is_object()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mMutex);
        std::unordered_set<std::string> parsedFiles;
        int mergedCount = 0;
        for (const auto& [rel, path] : mAssetCache) {
            if (rel == "sounds/sound_definitions.json" || (rel.size() > 22 && rel.substr(rel.size() - 22) == "sound_definitions.json")) {
                std::string pStr = path.string();
                if (parsedFiles.count(pStr)) continue;
                parsedFiles.insert(pStr);

                std::ifstream f(path);
                if (!f.is_open()) continue;
                std::string customContent((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                auto customJson = nlohmann::json::parse(stripJsonComments(customContent), nullptr, false, true);
                if (!customJson.is_discarded() && customJson.contains("sound_definitions") && customJson["sound_definitions"].is_object()) {
                    for (auto& [soundKey, soundVal] : customJson["sound_definitions"].items()) {
                        root["sound_definitions"][soundKey] = soundVal;
                        mergedCount++;
                    }
                }
            }
        }
        if (mergedCount > 0) {
            inOutJson = root.dump();
            ll::Logger::getInstance().info("ResourceManager: Successfully patched sound_definitions.json (merged ", mergedCount, " definitions).");
        }
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch sound_definitions.json: ", e.what());
        return false;
    }
}

bool ResourceManager::patchFlipbookTexturesJson(std::string& inOutJson) const {
    try {
        nlohmann::json root;
        if (inOutJson.empty()) return false;
        std::string clean = stripJsonComments(inOutJson);
        root = nlohmann::json::parse(clean, nullptr, false, true);
        if (root.is_discarded() || !root.is_array()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mMutex);
        std::unordered_set<std::string> parsedFiles;
        std::unordered_set<std::string> existingTiles;
        for (const auto& item : root) {
            if (item.is_object() && item.contains("atlas_tile") && item["atlas_tile"].is_string()) {
                existingTiles.insert(item["atlas_tile"].get<std::string>());
            }
        }

        int mergedCount = 0;
        for (const auto& [rel, path] : mAssetCache) {
            if (rel == "textures/flipbook_textures.json" || (rel.size() > 22 && rel.substr(rel.size() - 22) == "flipbook_textures.json")) {
                std::string pStr = path.string();
                if (parsedFiles.count(pStr)) continue;
                parsedFiles.insert(pStr);

                std::ifstream f(path);
                if (!f.is_open()) continue;
                std::string customContent((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
                auto customJson = nlohmann::json::parse(stripJsonComments(customContent), nullptr, false, true);
                if (!customJson.is_discarded() && customJson.is_array()) {
                    for (const auto& entry : customJson) {
                        if (entry.is_object() && entry.contains("atlas_tile") && entry["atlas_tile"].is_string()) {
                            std::string tile = entry["atlas_tile"].get<std::string>();
                            if (!existingTiles.count(tile)) {
                                root.push_back(entry);
                                existingTiles.insert(tile);
                                mergedCount++;
                            }
                        }
                    }
                }
            }
        }
        if (mergedCount > 0) {
            inOutJson = root.dump();
            ll::Logger::getInstance().info("ResourceManager: Successfully patched flipbook_textures.json (merged ", mergedCount, " animated textures).");
        }
        return true;
    } catch (const std::exception& e) {
        ll::Logger::getInstance().error("ResourceManager: Failed to patch flipbook_textures.json: ", e.what());
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

    std::string prefixWithSlash = p.empty() ? "" : (p + "/");
    std::unordered_set<std::string> seen;

    for (const auto& [rel, diskPath] : mAssetCache) {
        if (prefixWithSlash.empty() || rel.rfind(prefixWithSlash, 0) == 0 || rel == p) {
            if (seen.insert(rel).second) {
                callback(rel);
            }
        }
    }
}

void ResourceManager::activateModResourcesOnStartScreen() {
    std::lock_guard<std::mutex> lock(mMutex);
    auto& logger = ll::Logger::getInstance();
    logger.info("§6[ResourceManager] §aActivating mod resources over vanilla on start screen...");
    logger.info("§6[ResourceManager] §aTotal mounted resource folders: ", mMountedDirectories.size(),
                ", Total indexed assets: ", mAssetCache.size());
    for (const auto& dir : mMountedDirectories) {
        logger.info("§6[ResourceManager] §2  Active mod resource layer: §e", dir.string());
    }
    logger.info("§6[ResourceManager] §aGlobal mod resources are active across all worlds (zero per-world RP/BP bypass).");
}

void ResourceManager::autoDiscoverModResources() {
    // No-op: LeviLamroid does not hardcode or auto-discover resources on its own.
    // Individual mods call ll_resource_mount_dir() or ll_addon_ingest() with their own paths.
}

} // namespace ll::resource

extern "C" {

LL_API void ll_resource_mount_dir(const char* dir) {
    if (dir) {
        ::ll::resource::ResourceManager::getInstance().mountDirectory(dir);
    }
}

LL_API void ll_addon_ingest(const char* dir) {
    if (dir) {
        ::levilamroid::addon::ingestAddonDirectory(dir);
    }
}

LL_API void ll_resource_auto_discover() {
    // Retained for backward API compatibility
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
