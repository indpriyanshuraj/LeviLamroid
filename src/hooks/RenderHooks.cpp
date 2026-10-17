#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/RenderEvents.h"
#include "ll/api/render/Render.h"
#include <GLES3/gl3.h>
#include <vector>
#include <atomic>

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

static std::atomic<bool> sSuppressBackground = false;
static std::vector<void(*)(int, int)> sPostClearCallbacks;

static void hooked_glClear(GLbitfield mask) {
    // 1. Let OpenGL driver and game execute original clear first
    if (sOrigGlClear) {
        reinterpret_cast<void(*)(GLbitfield)>(sOrigGlClear)(mask);
    }

    // 2. Only intercept color clear on main menus, NEVER in-world, NEVER while loading
    if ((mask & GL_COLOR_BUFFER_BIT) && isMainMenu() && !isInWorld() && !isLoading()) {
        // Crucial fix: Only execute post-clear callbacks on the default backbuffer (FBO 0).
        // UI screens use off-screen FBOs for text caching, widgets, paperdolls, and sub-panels.
        // Calling post-clear callbacks during off-screen FBO clears causes redundant rendering passes
        // and texture pollution.
        GLint currentFbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);
        if (currentFbo != 0) {
            return;
        }

        GLint vp[4] = {0, 0, 1920, 1080};
        glGetIntegerv(GL_VIEWPORT, vp);

        ll::event::GlClearEvent event(static_cast<uint32_t>(mask), vp);
        ll::event::EventBus::getInstance().publish(event);

        for (auto cb : sPostClearCallbacks) {
            if (cb) cb(vp[2], vp[3]);
        }
    }
}

static void hooked_panoramaRender(void* self, void* ctx, void* client, void* owner, int layer) {
    if (sSuppressBackground.load() && isMainMenu() && !isInWorld() && !isLoading()) {
        return; // Suppressed on menu!
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

static void hooked_cubemapBackgroundRender(void* self, void* ctx, const void* frameObj) {
    if (sSuppressBackground.load() && isMainMenu() && !isInWorld() && !isLoading()) {
        return; // Suppressed on menu!
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

static void hooked_progressBarRender(void* self, void* ctx, void* client, void* owner, int layer) {
    // A progress bar is rendering (World Loading Screen, Building Terrain, Boot Progress, etc.)
    setIsLoading(true);
    setIsMainMenu(false);
    setInWorld(false);

    ll::event::ProgressBarRenderEvent event;
    ll::event::EventBus::getInstance().publish(event);
    // Safe passthrough; never modify raw struct offsets
    if (sOrigProgressBarRender) {
        reinterpret_cast<void(*)(void*, void*, void*, void*, int)>(sOrigProgressBarRender)(self, ctx, client, owner, layer);
    }
}

bool installRenderHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    // glClear GOT Relocation Slot Hook
    // Raw GOT RVA: 1.26.44.3: 0x12c40c30 | 1.26.45.1: 0x12c40b90 | 1.26.51.1: 0x13289338 | 1.26.52.3: 0x132893d8
    if (off.glClearGotRva) {
        if (ll::memory::hookGotSlot(off.glClearGotRva, (void*)&hooked_glClear, &sOrigGlClear)) {
            logger.info("Hooked glClear GOT relocation [Raw RVA: 0x", std::hex, off.glClearGotRva, "]");
        }
    }

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
}
