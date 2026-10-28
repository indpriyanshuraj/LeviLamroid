#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/RenderEvents.h"
#include "ll/api/render/Render.h"
#include "fabric/ClientSignatures.hpp"
#include "fabric/hud/ModularHud.hpp"
#include "fabric/schematic/SchematicRenderer.hpp"
#include <GLES3/gl3.h>
#include <vector>
#include <atomic>
#include <mutex>

namespace levilamroid::hooks {

bool isInWorld();
bool isMainMenu();
void setInWorld(bool inWorld);
void setIsMainMenu(bool isMenu);
void setIsLoading(bool loading);
bool isLoading();
void dispatchStartMenuOpen();

static void* sOrigGlClear = nullptr;
static void* sOrigPanoramaRender = nullptr;
static void* sOrigCubemapBackgroundRender = nullptr;
static void* sOrigProgressBarRender = nullptr;

// Step B & C Verified RenderDragon Hooks
static void* sOrigInGamePlayScreenRender = nullptr;
static void* sOrigInGamePlayScreenRenderLevel = nullptr;
static void* sOrigInGamePlayScreenRenderLevelPrep = nullptr;
static void* sOrigMinecraftUIRenderContextFlushImages = nullptr;
static void* sOrigGameRendererPrepareFrame = nullptr;
static void* sOrigLevelRendererPlayerPreRenderUpdate = nullptr;

static std::atomic<bool> sSuppressBackground = false;
static std::vector<void(*)(int, int)> sPostClearCallbacks;
static std::vector<void(*)(void*, const void*)> sLevelRenderCallbacks;
static std::vector<void(*)(void*, const void*)> sUIRenderCallbacks;
static std::vector<void(*)(void*, const void*, float, const void*)> sFlushImagesCallbacks;
static std::mutex sRenderCallbackMutex;

static void hooked_glClear(GLbitfield mask) {
    if (sOrigGlClear) {
        reinterpret_cast<void(*)(GLbitfield)>(sOrigGlClear)(mask);
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
        GLint currentFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);
        if (currentFbo != 0) {
            return;
        }

        GLint vp[4] = {0, 0, 1920, 1080};
        glGetIntegerv(GL_VIEWPORT, vp);

        if (isMainMenu() && !isInWorld() && !isLoading()) {
            try {
                ll::event::GlClearEvent event(static_cast<uint32_t>(mask), vp);
                ll::event::EventBus::getInstance().publish(event);
            } catch (...) {}

            for (auto cb : sPostClearCallbacks) {
                try {
                    if (cb) cb(vp[2], vp[3]);
                } catch (...) {}
            }
        } else if (isInWorld() && !isLoading()) {
            // Safe in-world frame render pass for Fabric client mods
            std::lock_guard<std::mutex> lock(sRenderCallbackMutex);
            for (auto cb : sUIRenderCallbacks) {
                try {
                    if (cb) cb(nullptr, nullptr);
                } catch (...) {}
            }
            try {
                fabric::hud::ModularHudRegistry::getInstance().renderAll(nullptr, nullptr);
            } catch (...) {}
        }
    }
}

[[maybe_unused]] static void hooked_panoramaRender(void* self, void* ctx, void* client, void* owner, int layer) {
    if (sSuppressBackground.load() && isMainMenu() && !isInWorld() && !isLoading()) {
        return;
    }
    ll::event::CubemapRenderEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return;
    }
    if (sOrigPanoramaRender) {
        reinterpret_cast<void(*)(void*, void*, void*, void*, int)>(sOrigPanoramaRender)(self, ctx, client, owner, layer);
    }
}

[[maybe_unused]] static void hooked_cubemapBackgroundRender(void* self, void* ctx, const void* frameObj) {
    if (sSuppressBackground.load() && isMainMenu() && !isInWorld() && !isLoading()) {
        return;
    }
    ll::event::CubemapRenderEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled()) {
        return;
    }
    if (sOrigCubemapBackgroundRender) {
        reinterpret_cast<void(*)(void*, void*, const void*)>(sOrigCubemapBackgroundRender)(self, ctx, frameObj);
    }
}

[[maybe_unused]] static void hooked_progressBarRender(void* self, void* ctx, void* client, void* owner, int layer) {
    setIsLoading(true);
    setIsMainMenu(false);
    setInWorld(false);

    ll::event::ProgressBarRenderEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (sOrigProgressBarRender) {
        reinterpret_cast<void(*)(void*, void*, void*, void*, int)>(sOrigProgressBarRender)(self, ctx, client, owner, layer);
    }
}

// ── InGamePlayScreen::render (2D HUD & Overlay Composite Pass) ──
static void hooked_inGamePlayScreenRender(void* self, void* screenContext, const void* frameObj) {
    setInWorld(true);
    setIsMainMenu(false);
    setIsLoading(false);

    if (sOrigInGamePlayScreenRender) {
        reinterpret_cast<void(*)(void*, void*, const void*)>(sOrigInGamePlayScreenRender)(self, screenContext, frameObj);
    }

    try {
        ll::event::UIRenderEvent event(screenContext, frameObj);
        ll::event::EventBus::getInstance().publish(event);
    } catch (...) {}

    {
        std::lock_guard<std::mutex> lock(sRenderCallbackMutex);
        for (auto cb : sUIRenderCallbacks) {
            try {
                if (cb) cb(screenContext, frameObj);
            } catch (...) {}
        }
    }

    try {
        fabric::hud::ModularHudRegistry::getInstance().renderAll(screenContext, frameObj);
        fabric::schematic::SchematicRendererEngine::getInstance().renderGhostBlocks();
    } catch (...) {}
}

// ── InGamePlayScreen::_renderLevel (3D Voxel World Terrain & Mesh Pass) ──
static void hooked_inGamePlayScreenRenderLevel(void* self, void* screenContext, const void* frameObj) {
    setInWorld(true);

    if (sOrigInGamePlayScreenRenderLevel) {
        reinterpret_cast<void(*)(void*, void*, const void*)>(sOrigInGamePlayScreenRenderLevel)(self, screenContext, frameObj);
    }

    try {
        ll::event::RenderLevelEvent event(screenContext, frameObj);
        ll::event::EventBus::getInstance().publish(event);
    } catch (...) {}

    {
        std::lock_guard<std::mutex> lock(sRenderCallbackMutex);
        for (auto cb : sLevelRenderCallbacks) {
            try {
                if (cb) cb(screenContext, frameObj);
            } catch (...) {}
        }
    }

    try {
        fabric::schematic::SchematicRendererEngine::getInstance().renderGhostBlocks();
    } catch (...) {}
}

// ── InGamePlayScreen::_renderLevelPrep ──
static void hooked_inGamePlayScreenRenderLevelPrep(void* self, void* screenContext, void* levelRenderer, void* viewer) {
    if (sOrigInGamePlayScreenRenderLevelPrep) {
        reinterpret_cast<void(*)(void*, void*, void*, void*)>(sOrigInGamePlayScreenRenderLevelPrep)(self, screenContext, levelRenderer, viewer);
    }
}

// ── MinecraftUIRenderContext::flushImages (RenderDragon Quad Sprite Batcher) ──
[[maybe_unused]] static void hooked_flushImages(void* self, const void* color, float alpha, const void* material) {
    if (sOrigMinecraftUIRenderContextFlushImages) {
        reinterpret_cast<void(*)(void*, const void*, float, const void*)>(sOrigMinecraftUIRenderContextFlushImages)(self, color, alpha, material);
    }

    std::lock_guard<std::mutex> lock(sRenderCallbackMutex);
    for (auto cb : sFlushImagesCallbacks) {
        if (cb) cb(self, color, alpha, material);
    }
}

// ── GameRenderer::_prepareFrame (Camera Perspective Matrix Setup) ──
[[maybe_unused]] static void hooked_gameRendererPrepareFrame(void* self, void* screenContext) {
    if (sOrigGameRendererPrepareFrame) {
        reinterpret_cast<void(*)(void*, void*)>(sOrigGameRendererPrepareFrame)(self, screenContext);
    }
    ll::event::RenderFramePrepareEvent event(screenContext);
    ll::event::EventBus::getInstance().publish(event);
}

// ── LevelRendererPlayer::preRenderUpdate (Camera View Frustum & Orientation) ──
[[maybe_unused]] static void hooked_levelRendererPlayerPreRenderUpdate(void* self, void* screenContext, void* params) {
    if (sOrigLevelRendererPlayerPreRenderUpdate) {
        reinterpret_cast<void(*)(void*, void*, void*)>(sOrigLevelRendererPlayerPreRenderUpdate)(self, screenContext, params);
    }
}

static std::atomic<bool> sInGameRenderHooksInstalled = false;
static std::mutex sRenderHookInstallMutex;

static void ensureInGameRenderHooksInstalled() {
    if (sInGameRenderHooksInstalled.load()) return;
    std::lock_guard<std::mutex> lock(sRenderHookInstallMutex);
    if (sInGameRenderHooksInstalled.load()) return;
    sInGameRenderHooksInstalled.store(true);

    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.inGamePlayScreenRender) {
        if (ll::memory::hookRva(off.inGamePlayScreenRender, (void*)&hooked_inGamePlayScreenRender, &sOrigInGamePlayScreenRender)) {
            logger.info("[Fabric Render] Hooked InGamePlayScreen::render [RVA: 0x", std::hex, off.inGamePlayScreenRender, "]");
        }
    }

    if (off.inGamePlayScreenRenderLevel) {
        if (ll::memory::hookRva(off.inGamePlayScreenRenderLevel, (void*)&hooked_inGamePlayScreenRenderLevel, &sOrigInGamePlayScreenRenderLevel)) {
            logger.info("[Fabric Render] Hooked InGamePlayScreen::_renderLevel [RVA: 0x", std::hex, off.inGamePlayScreenRenderLevel, "]");
        }
    }

    if (off.inGamePlayScreenRenderLevelPrep) {
        if (ll::memory::hookRva(off.inGamePlayScreenRenderLevelPrep, (void*)&hooked_inGamePlayScreenRenderLevelPrep, &sOrigInGamePlayScreenRenderLevelPrep)) {
            logger.info("[Fabric Render] Hooked InGamePlayScreen::_renderLevelPrep [RVA: 0x", std::hex, off.inGamePlayScreenRenderLevelPrep, "]");
        }
    }
}

bool installRenderHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    // 1. glClear GOT Relocation Slot Hook (safe pointer replacement)
    if (off.glClearGotRva) {
        if (ll::memory::hookGotSlot(off.glClearGotRva, (void*)&hooked_glClear, &sOrigGlClear)) {
            logger.info("Hooked glClear GOT relocation [Raw RVA: 0x", std::hex, off.glClearGotRva, "]");
        }
    }

    ensureInGameRenderHooksInstalled();

    logger.info("LeviLamroid Render hook subsystem initialized.");
    return true;
}

} // namespace levilamroid::hooks

extern "C" {
LL_API void ll_render_suppress_vanilla_background(bool suppress) {
    ::levilamroid::hooks::sSuppressBackground.store(suppress);
}
LL_API void ll_render_register_post_clear_callback(void (*cb)(int width, int height)) {
    if (cb) ::levilamroid::hooks::sPostClearCallbacks.push_back(cb);
}
LL_API void ll_render_register_level_render_callback(void (*cb)(void* screenCtx, const void* frameObj)) {
    if (cb) {
        ::levilamroid::hooks::ensureInGameRenderHooksInstalled();
        std::lock_guard<std::mutex> lock(::levilamroid::hooks::sRenderCallbackMutex);
        ::levilamroid::hooks::sLevelRenderCallbacks.push_back(cb);
    }
}
LL_API void ll_render_register_ui_render_callback(void (*cb)(void* screenCtx, const void* frameObj)) {
    if (cb) {
        ::levilamroid::hooks::ensureInGameRenderHooksInstalled();
        std::lock_guard<std::mutex> lock(::levilamroid::hooks::sRenderCallbackMutex);
        ::levilamroid::hooks::sUIRenderCallbacks.push_back(cb);
    }
}
LL_API void ll_render_register_flush_images_callback(void (*cb)(void* ctx, const void* color, float alpha, const void* material)) {
    if (cb) {
        ::levilamroid::hooks::ensureInGameRenderHooksInstalled();
        std::lock_guard<std::mutex> lock(::levilamroid::hooks::sRenderCallbackMutex);
        ::levilamroid::hooks::sFlushImagesCallbacks.push_back(cb);
    }
}
}
