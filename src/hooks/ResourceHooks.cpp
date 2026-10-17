#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/resource/ResourceManager.h"
#include <string>
#include <cstring>

namespace levilamroid::hooks {

static void* sOrigDirectoryPackGetAsset = nullptr;
static void* sOrigDirectoryPackHasAsset = nullptr;
static void* sOrigZipPackGetAsset = nullptr;
static void* sOrigZipPackHasAsset = nullptr;

static bool hooked_directoryPackHasAsset(void* self, void* pathPtr, bool flag1, bool flag2) {
    bool res = false;
    if (sOrigDirectoryPackHasAsset) {
        res = reinterpret_cast<bool(*)(void*, void*, bool, bool)>(sOrigDirectoryPackHasAsset)(self, pathPtr, flag1, flag2);
    }
    if (res) return true;

    // Intercept item, block, and atlas texture requests from mounted resources
    if (pathPtr) {
        const std::string* pathStr = reinterpret_cast<const std::string*>(pathPtr);
        if (pathStr && !pathStr->empty() && pathStr->size() < 1024) {
            const std::string& p = *pathStr;
            if (p.find("textures/") != std::string::npos || p.find("blocks/") != std::string::npos) {
                if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool hooked_zipPackHasAsset(void* self, void* pathPtr, bool flag1, bool flag2) {
    bool res = false;
    if (sOrigZipPackHasAsset) {
        res = reinterpret_cast<bool(*)(void*, void*, bool, bool)>(sOrigZipPackHasAsset)(self, pathPtr, flag1, flag2);
    }
    if (res) return true;

    // Intercept item, block, and atlas texture requests from mounted resources
    if (pathPtr) {
        const std::string* pathStr = reinterpret_cast<const std::string*>(pathPtr);
        if (pathStr && !pathStr->empty() && pathStr->size() < 1024) {
            const std::string& p = *pathStr;
            if (p.find("textures/") != std::string::npos || p.find("blocks/") != std::string::npos) {
                if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static bool hooked_directoryPackGetAsset(void* self, void* pathPtr, std::string& outBuffer, bool loadMode) {
    bool res = false;
    if (sOrigDirectoryPackGetAsset) {
        res = reinterpret_cast<bool(*)(void*, void*, std::string&, bool)>(sOrigDirectoryPackGetAsset)(self, pathPtr, outBuffer, loadMode);
    }

    if (pathPtr) {
        const std::string* pathStr = reinterpret_cast<const std::string*>(pathPtr);
        if (pathStr && pathStr->size() > 0 && pathStr->size() < 1024) {
            const std::string& p = *pathStr;
            
            // 1. Patch item_texture.json when loaded
            if (p.find("item_texture.json") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
                    return true;
                }
            }
            
            // 2. Patch terrain_texture.json when loaded
            if (p.find("terrain_texture.json") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchTerrainTextureJson(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    ::ll::resource::ResourceManager::getInstance().patchTerrainTextureJson(outBuffer);
                    return true;
                }
            }

            // 3. Patch blocks.json when loaded
            if (p.find("blocks.json") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
                    return true;
                } else {
                    outBuffer = "{}";
                    ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
                    return true;
                }
            }

            // 4. Patch lang file when loaded
            if (p.find(".lang") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchLangFile(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    return true;
                }
            }

            // 5. Fallback for custom mounted textures (items, blocks, etc.)
            if (!res && (p.find("textures/") != std::string::npos || p.find("blocks/") != std::string::npos || p.find("blocks.json") != std::string::npos)) {
                if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    return true;
                }
            }
        }
    }

    return res;
}

static bool hooked_zipPackGetAsset(void* self, void* pathPtr, std::string& outBuffer, bool loadMode) {
    bool res = false;
    if (sOrigZipPackGetAsset) {
        res = reinterpret_cast<bool(*)(void*, void*, std::string&, bool)>(sOrigZipPackGetAsset)(self, pathPtr, outBuffer, loadMode);
    }

    if (pathPtr) {
        const std::string* pathStr = reinterpret_cast<const std::string*>(pathPtr);
        if (pathStr && pathStr->size() > 0 && pathStr->size() < 1024) {
            const std::string& p = *pathStr;
            if (p.find("item_texture.json") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
                    return true;
                }
            }
            if (p.find("terrain_texture.json") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchTerrainTextureJson(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    ::ll::resource::ResourceManager::getInstance().patchTerrainTextureJson(outBuffer);
                    return true;
                }
            }
            if (p.find("blocks.json") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
                    return true;
                } else {
                    outBuffer = "{}";
                    ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
                    return true;
                }
            }
            if (p.find(".lang") != std::string::npos) {
                if (res) {
                    ::ll::resource::ResourceManager::getInstance().patchLangFile(outBuffer);
                    return true;
                } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    return true;
                }
            }
            if (!res && (p.find("textures/") != std::string::npos || p.find("blocks/") != std::string::npos || p.find("blocks.json") != std::string::npos)) {
                if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                    return true;
                }
            }
        }
    }

    return res;
}

bool installResourceHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.directoryPackGetAsset) {
        if (ll::memory::hookRva(off.directoryPackGetAsset, (void*)&hooked_directoryPackGetAsset, &sOrigDirectoryPackGetAsset)) {
            logger.info("Hooked DirectoryPack::getAsset [RVA: 0x", std::hex, off.directoryPackGetAsset, "]");
        }
    }
    if (off.directoryPackHasAsset) {
        if (ll::memory::hookRva(off.directoryPackHasAsset, (void*)&hooked_directoryPackHasAsset, &sOrigDirectoryPackHasAsset)) {
            logger.info("Hooked DirectoryPack::hasAsset [RVA: 0x", std::hex, off.directoryPackHasAsset, "]");
        }
    }
    if (off.zipPackGetAsset) {
        if (ll::memory::hookRva(off.zipPackGetAsset, (void*)&hooked_zipPackGetAsset, &sOrigZipPackGetAsset)) {
            logger.info("Hooked ZipPack::getAsset [RVA: 0x", std::hex, off.zipPackGetAsset, "]");
        }
    }
    if (off.zipPackHasAsset) {
        if (ll::memory::hookRva(off.zipPackHasAsset, (void*)&hooked_zipPackHasAsset, &sOrigZipPackHasAsset)) {
            logger.info("Hooked ZipPack::hasAsset [RVA: 0x", std::hex, off.zipPackHasAsset, "]");
        }
    }

    logger.info("LeviLamroid Resource subsystem active.");
    return true;
}

} // namespace levilamroid::hooks
