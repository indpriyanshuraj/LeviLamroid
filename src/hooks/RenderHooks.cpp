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

    // PanoramaRenderer::render
    // Raw RVA: 1.26.44.3: 0x092B8E60 | 1.26.45.1: 0x092B8DC0 | 1.26.51.1: 0x0967F9F8 | 1.26.52.3: 0x0967FA98
    if (ll::memory::hookSignature("ED 33 B7 6D EB 2B 01 6D E9 23 02 6D FD 7B 03 A9 FC 23 00 F9 FA 67 05 A9 F8 5F 06 A9 F6 57 07 A9 F4 4F 08 A9 FD C3 00 91 FF 83 13 D1 59 D0 3B D5",
                                  (void*)&hooked_panoramaRender, &sOrigPanoramaRender, off.panoramaRender)) { /* 0x092B8E60 */
        logger.info("Hooked PanoramaRenderer::render via signature [Raw RVA: 0x", std::hex, off.panoramaRender, "]");
    }

    // CubemapBackgroundScreen::render
    // Raw RVA: 1.26.44.3: 0x097C9A20 | 1.26.45.1: 0x097C9980 | 1.26.51.1: 0x09B7C960 | 1.26.52.3: 0x09B7C9E0
    if (off.cubemapBackgroundRender) {
        // 1.26.5x layout
        bool ok = ll::memory::hookSignature("FF 03 02 D1 FD 7B 04 A9 F7 2B 00 F9 F6 57 06 A9 F4 4F 07 A9 FD 03 01 91 56 D0 3B D5 D7 1B 98 52",
                                            (void*)&hooked_cubemapBackgroundRender, &sOrigCubemapBackgroundRender, off.cubemapBackgroundRender); /* 0x09B7C9E0 */
        if (!ok) {
            // 1.26.4x layout
            ok = ll::memory::hookSignature("FD 7B BE A9 F3 0B 00 F9 FD 03 00 91 F3 03 00 AA ? ? ? ? E0 03 13 AA F3 0B 40 F9 FD 7B C2 A8",
                                           (void*)&hooked_cubemapBackgroundRender, &sOrigCubemapBackgroundRender, off.cubemapBackgroundRender); /* 0x097C9A20 */
        }
        if (ok) {
            logger.info("Hooked CubemapBackgroundScreen::render via signature [Raw RVA: 0x", std::hex, off.cubemapBackgroundRender, "]");
        }
    }

    // ProgressBarRenderer::render
    // Raw RVA: 1.26.44.3: 0x092BECCC | 1.26.45.1: 0x092BEC2C | 1.26.51.1: 0x09685ED0 | 1.26.52.3: 0x09685F70
    if (ll::memory::hookSignature("FF 03 05 D1 EF 3B 0A 6D ED 33 0B 6D EB 2B 0C 6D E9 23 0D 6D FD 7B 0E A9 FC 6F 0F A9 FA 67 10 A9",
                                  (void*)&hooked_progressBarRender, &sOrigProgressBarRender, off.progressBarRender)) { /* 0x092BECCC */
        logger.info("Hooked ProgressBarRenderer::render via signature [Raw RVA: 0x", std::hex, off.progressBarRender, "]");
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
