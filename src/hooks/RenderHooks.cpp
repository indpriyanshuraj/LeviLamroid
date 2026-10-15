#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/render/RenderEvents.h"
#include <GLES3/gl3.h>

namespace levilamroid::hooks {

static void* sOrigGlClear = nullptr;

static void hooked_glClear(GLbitfield mask) {
    int vp[4] = {0, 0, 1920, 1080};
    glGetIntegerv(GL_VIEWPORT, vp);

    ll::event::GlClearEvent event(static_cast<uint32_t>(mask), vp);
    ll::event::EventBus::getInstance().publish(event);

    if (sOrigGlClear) {
        reinterpret_cast<void(*)(GLbitfield)>(sOrigGlClear)(mask);
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

    logger.info("LeviLamroid Render hook subsystem initialized.");
    return true;
}

} // namespace levilamroid::hooks
