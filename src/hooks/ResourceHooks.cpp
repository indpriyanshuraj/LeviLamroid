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
    uintptr_t untagged = v & 0x00ffffffffffffffULL;
    if (untagged < 0x10000 || untagged > 0x00007fffffffffffULL) return false;
    if (untagged + size < untagged) return false;
    return true;
}

static bool safeExtractPath(const void* pathPtr, std::string& outPath) {
    if (!pathPtr || !isPointerReadable(pathPtr, 8)) return false;

    // 1. Try std::string at offset 0 (standard Core::Path layout)
    try {
        if (isPointerReadable(pathPtr, sizeof(std::string))) {
            const auto* s = reinterpret_cast<const std::string*>(pathPtr);
            size_t sz = s->size();
            if (sz > 0 && sz < 1024) {
                const char* d = s->c_str();
                if (isPointerReadable(d, sz)) {
                    bool valid = true;
                    for (size_t i = 0; i < sz; ++i) {
                        unsigned char c = static_cast<unsigned char>(d[i]);
                        if (c < 32 || c > 126) { valid = false; break; }
                    }
                    if (valid) {
                        outPath.assign(d, sz);
                        return true;
                    }
                }
            }
        }
    } catch (...) {}

    // 2. Try std::string at offset 8 (in case Core::Path has an 8-byte vtable/prefix)
    try {
        const void* p8 = reinterpret_cast<const char*>(pathPtr) + 8;
        if (isPointerReadable(p8, sizeof(std::string))) {
            const auto* s = reinterpret_cast<const std::string*>(p8);
            size_t sz = s->size();
            if (sz > 0 && sz < 1024) {
                const char* d = s->c_str();
                if (isPointerReadable(d, sz)) {
                    bool valid = true;
                    for (size_t i = 0; i < sz; ++i) {
                        unsigned char c = static_cast<unsigned char>(d[i]);
                        if (c < 32 || c > 126) { valid = false; break; }
                    }
                    if (valid) {
                        outPath.assign(d, sz);
                        return true;
                    }
                }
            }
        }
    } catch (...) {}

    // 3. Try direct raw C string pointer
    try {
        const char* cs = reinterpret_cast<const char*>(pathPtr);
        if (isPointerReadable(cs, 4)) {
            size_t len = 0;
            while (len < 512 && isPointerReadable(cs + len, 1) && cs[len] != 0) {
                unsigned char c = static_cast<unsigned char>(cs[len]);
                if (c < 32 || c > 126) break;
                len++;
            }
            if (len >= 3 && cs[len] == 0) {
                outPath.assign(cs, len);
                return true;
            }
        }
    } catch (...) {}

    return false;
}

static inline bool isExcludedFromFallback(const std::string& p) {
    // Never exclude textures, images, sounds, or models from fallback
    if (p.size() >= 4) {
        if (p.ends_with(".png") || p.ends_with(".tga") || p.ends_with(".hdr") ||
            p.ends_with(".jpg") || p.ends_with(".mesh") || p.ends_with(".fsb")) {
            return false;
        }
    }

    if (p.find("blocks.json") != std::string::npos) return true;
    if (p.find("terrain_texture.json") != std::string::npos) return true;
    if (p.find("item_texture.json") != std::string::npos) return true;
    if (p.find("_ui_defs.json") != std::string::npos) return true;
    if (p.rfind("ui/", 0) == 0 && p.ends_with(".json")) return true;
    if (p.find("flipbook_textures.json") != std::string::npos) return true;
    if (p.find("manifest.json") != std::string::npos) return true;
    if (p.find("pack_manifest.json") != std::string::npos) return true;
    if (p.find("contents.json") != std::string::npos) return true;
    if (p.find(".lang") != std::string::npos) return true;
    if (p.find("languages.json") != std::string::npos) return true;
    if (p.find("sound_definitions.json") != std::string::npos) return true;
    if (p.find(".material") != std::string::npos) return true;
    if (p.find("materials/") != std::string::npos) return true;

    // Core vanilla JSON UI screens must never be replaced by partial delta snippets via fallback
    static const char* sVanillaScreens[] = {
        "hud_screen.json",
        "pause_screen.json",
        "chest_screen.json",
        "redstone_screen.json",
        "inventory_screen.json",
        "chat_screen.json",
        "death_screen.json",
        "start_screen.json",
        "settings_screen.json",
        "progress_screen.json",
        "world_loading_screen.json",
        "common.json",
        "ui_common.json"
    };
    for (const char* v : sVanillaScreens) {
        if (p.find(v) != std::string::npos) return true;
    }
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
            if (isExcludedFromFallback(p)) {
                return false;
            }
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
            if (isExcludedFromFallback(p)) {
                return false;
            }
            if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
                return true;
            }
        }
    } catch (...) {}
    return res;
}

static bool hooked_directoryPackGetAsset(void* self, void* pathPtr, std::string& outBuffer, bool loadMode) noexcept {
    if (!isPointerReadable(&outBuffer, sizeof(std::string))) return false;
    bool res = false;
    try {
        if (sOrigDirectoryPackGetAsset) {
            res = reinterpret_cast<bool(*)(void*, void*, std::string&, bool)>(sOrigDirectoryPackGetAsset)(self, pathPtr, outBuffer, loadMode);
        }

        std::string p;
        if (!safeExtractPath(pathPtr, p)) {
            return res;
        }

        // 1. Only patch global JSON files if the pack genuinely has them (e.g. vanilla packs)
        if (res && !outBuffer.empty()) {
            if (p.find("item_texture.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
            } else if (p.find("terrain_texture.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchTerrainTextureJson(outBuffer);
            } else if (p.find("_ui_defs.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchUiDefsJson(outBuffer);
            } else if (p.find("blocks.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
            } else if (p.find("sound_definitions.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchSoundDefinitionsJson(outBuffer);
            } else if (p.find("flipbook_textures.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchFlipbookTexturesJson(outBuffer);
            } else if (p.find(".lang") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchLangFile(outBuffer, p);
            }
            return true;
        }

        // 2. Never serve global metadata files as fake pack overrides
        if (isExcludedFromFallback(p)) {
            return false;
        }

        // 3. Fallback for mounted custom assets (textures, models, entities, particles, ui)
        if (::ll::resource::ResourceManager::getInstance().hasAsset(p)) {
            if (::ll::resource::ResourceManager::getInstance().getAsset(p, outBuffer)) {
                return true;
            }
        }
    } catch (...) {}

    return res;
}

static bool hooked_zipPackGetAsset(void* self, void* pathPtr, std::string& outBuffer, bool loadMode) noexcept {
    if (!isPointerReadable(&outBuffer, sizeof(std::string))) return false;
    bool res = false;
    try {
        if (sOrigZipPackGetAsset) {
            res = reinterpret_cast<bool(*)(void*, void*, std::string&, bool)>(sOrigZipPackGetAsset)(self, pathPtr, outBuffer, loadMode);
        }

        std::string p;
        if (!safeExtractPath(pathPtr, p)) {
            return res;
        }

        // 1. Only patch global JSON files if the pack genuinely has them (e.g. vanilla packs)
        if (res && !outBuffer.empty()) {
            if (p.find("item_texture.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchItemTextureJson(outBuffer);
            } else if (p.find("terrain_texture.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchTerrainTextureJson(outBuffer);
            } else if (p.find("_ui_defs.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchUiDefsJson(outBuffer);
            } else if (p.find("blocks.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchBlocksJson(outBuffer);
            } else if (p.find("sound_definitions.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchSoundDefinitionsJson(outBuffer);
            } else if (p.find("flipbook_textures.json") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchFlipbookTexturesJson(outBuffer);
            } else if (p.find(".lang") != std::string::npos) {
                ::ll::resource::ResourceManager::getInstance().patchLangFile(outBuffer, p);
            }
            return true;
        }

        // 2. Never serve global metadata files as fake pack overrides
        if (isExcludedFromFallback(p)) {
            return false;
        }

        // 3. Fallback for mounted custom assets (textures, models, entities, particles, ui)
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
