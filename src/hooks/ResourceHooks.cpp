#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/resource/ResourceManager.h"
#include <string>
#include <cstring>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>

namespace levilamroid::hooks {

static void* sOrigDirectoryPackGetAsset = nullptr;
static void* sOrigDirectoryPackHasAsset = nullptr;
static void* sOrigZipPackGetAsset = nullptr;
static void* sOrigZipPackHasAsset = nullptr;

static inline bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    if (v < 0x10000 || v > 0x7fffffffff) return false;
    if (v + size < v) return false;
    return true;
}

static bool safeExtractPath(const void* pathPtr, std::string& outPath) {
    if (!pathPtr || !isPointerReadable(pathPtr, sizeof(std::string))) return false;
    try {
        const auto* s = reinterpret_cast<const std::string*>(pathPtr);
        if (s->size() > 0 && s->size() < 1024) {
            outPath = *s;
            return true;
        }
    } catch (...) {}
    return false;
}

static bool hooked_directoryPackHasAsset(void* self, void* pathPtr, bool flag1, bool flag2) noexcept {
    bool res = false;
    try {
        if (sOrigDirectoryPackHasAsset) {
            res = reinterpret_cast<bool(*)(void*, void*, bool, bool)>(sOrigDirectoryPackHasAsset)(self, pathPtr, flag1, flag2);
        }
        if (res) return true;

        std::string p;
        if (safeExtractPath(pathPtr, p)) {
            if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
                return true;
            }
        }
    } catch (...) {}
    return res;
}

static bool hooked_zipPackHasAsset(void* self, void* pathPtr, bool flag1, bool flag2) noexcept {
    bool res = false;
    try {
        if (sOrigZipPackHasAsset) {
            res = reinterpret_cast<bool(*)(void*, void*, bool, bool)>(sOrigZipPackHasAsset)(self, pathPtr, flag1, flag2);
        }
        if (res) return true;

        std::string p;
        if (safeExtractPath(pathPtr, p)) {
            if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
                return true;
            }
        }
    } catch (...) {}
    return res;
}

static bool hooked_directoryPackGetAsset(void* self, void* pathPtr, std::string& outBuffer, bool loadMode) noexcept {
    bool res = false;
    try {
        if (sOrigDirectoryPackGetAsset) {
            res = reinterpret_cast<bool(*)(void*, void*, std::string&, bool)>(sOrigDirectoryPackGetAsset)(self, pathPtr, outBuffer, loadMode);
        }

        std::string p;
        if (!safeExtractPath(pathPtr, p)) {
            return res;
        }

        // 1. Patch item_texture.json when loaded (merge custom item textures into atlas)
        if (p.find("item_texture.json") != std::string::npos) {
            if (res) {
                ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
                return true;
            } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
                return true;
            }
        }
        
        // 2. Patch terrain_texture.json when loaded (merge custom block textures into terrain atlas)
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
            }
        }

        // 4. Patch _ui_defs.json when loaded (inject custom JSON UI screen definitions)
        if (p.find("_ui_defs.json") != std::string::npos) {
            if (res) {
                ::ll::resource::ResourceManager::getInstance().patchUiDefsJson(outBuffer);
                return true;
            } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                ::ll::resource::ResourceManager::getInstance().patchUiDefsJson(outBuffer);
                return true;
            }
        }

        // 5. CRITICAL RULE FOR STABILITY & SCUDO:
        // If the original vanilla pack already loaded the asset (textures, audio, vanilla lang),
        // return true IMMEDIATELY without mutating outBuffer across DSO libc++ heap boundaries!
        if (res) {
            return true;
        }

        // CRITICAL FOR STABILITY: Never serve .lang or languages.json from the fallback!
        // Bedrock iterates 2,000+ pack entries checking for localization files. Serving a custom lang file
        // across libc++ boundaries on every iteration corrupts Scudo heap chunks.
        if (p.find(".lang") != std::string::npos || p.find("languages.json") != std::string::npos) {
            return res;
        }

        // 6. Fallback for mounted custom assets (textures, models, entities, particles, blocks)
        if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
            if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                return true;
            }
        }
    } catch (...) {}

    return res;
}

static bool hooked_zipPackGetAsset(void* self, void* pathPtr, std::string& outBuffer, bool loadMode) noexcept {
    bool res = false;
    try {
        if (sOrigZipPackGetAsset) {
            res = reinterpret_cast<bool(*)(void*, void*, std::string&, bool)>(sOrigZipPackGetAsset)(self, pathPtr, outBuffer, loadMode);
        }

        std::string p;
        if (!safeExtractPath(pathPtr, p)) {
            return res;
        }

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
            }
        }
        if (p.find("_ui_defs.json") != std::string::npos) {
            if (res) {
                ::ll::resource::ResourceManager::getInstance().patchUiDefsJson(outBuffer);
                return true;
            } else if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                ::ll::resource::ResourceManager::getInstance().patchUiDefsJson(outBuffer);
                return true;
            }
        }

        // CRITICAL RULE FOR STABILITY & SCUDO:
        if (res) {
            return true;
        }

        // CRITICAL FOR STABILITY: Never serve .lang or languages.json from the fallback!
        if (p.find(".lang") != std::string::npos || p.find("languages.json") != std::string::npos) {
            return res;
        }

        if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
            if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                return true;
            }
        }
    } catch (...) {}

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

    logger.info("LeviLamroid Resource subsystem active (crash-safe, zero foreign callback invocation).");
    return true;
}

} // namespace levilamroid::hooks
