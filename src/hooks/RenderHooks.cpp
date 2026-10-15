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

    // 2. Only intercept color clear on main menus, NEVER in-world
    if ((mask & GL_COLOR_BUFFER_BIT) && isMainMenu() && !isInWorld()) {
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
    if (sSuppressBackground.load() && isMainMenu() && !isInWorld()) {
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
    if (sSuppressBackground.load() && isMainMenu() && !isInWorld()) {
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
    // A progress bar is rendering (World Loading Screen, Building Terrain, etc.)
    setInWorld(true);
    setIsMainMenu(false);

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

    if (off.glClearGotRva) {
        if (ll::memory::hookGotSlot(off.glClearGotRva, (void*)&hooked_glClear, &sOrigGlClear)) {
            logger.info("Hooked glClear GOT relocation at RVA 0x", std::hex, off.glClearGotRva);
        }
    }

    if (off.panoramaRender) {
        if (ll::memory::hookRva(off.panoramaRender, (void*)&hooked_panoramaRender, &sOrigPanoramaRender)) {
            logger.info("Hooked PanoramaRenderer::render at RVA 0x", std::hex, off.panoramaRender);
        }
    }

    if (off.cubemapBackgroundRender) {
        if (ll::memory::hookRva(off.cubemapBackgroundRender, (void*)&hooked_cubemapBackgroundRender, &sOrigCubemapBackgroundRender)) {
            logger.info("Hooked CubemapBackgroundScreen::render at RVA 0x", std::hex, off.cubemapBackgroundRender);
        }
    }

    if (off.progressBarRender) {
        if (ll::memory::hookRva(off.progressBarRender, (void*)&hooked_progressBarRender, &sOrigProgressBarRender)) {
            logger.info("Hooked ProgressBarRenderer::render at RVA 0x", std::hex, off.progressBarRender);
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
