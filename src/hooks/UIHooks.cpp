#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ui/UIEvents.h"

namespace levilamroid::hooks {

static void* sOrigStartMenuOnOpen = nullptr;
static void* sOrigPlayButton = nullptr;
static void* sOrigSettingsButton = nullptr;
static void* sOrigMenuExit = nullptr;

static void hooked_startMenuOnOpen(void* self) {
    ll::event::StartMenuOpenEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigStartMenuOnOpen) {
        reinterpret_cast<void(*)(void*)>(sOrigStartMenuOnOpen)(self);
    }
}

static void hooked_playButton(void* self) {
    ll::event::PlayButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigPlayButton) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayButton)(self);
    }
}

static void hooked_settingsButton(void* self) {
    ll::event::SettingsButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigSettingsButton) {
        reinterpret_cast<void(*)(void*)>(sOrigSettingsButton)(self);
    }
}

static void hooked_menuExit(void* self) {
    ll::event::MenuExitEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigMenuExit) {
        reinterpret_cast<void(*)(void*)>(sOrigMenuExit)(self);
    }
}

bool installUIHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.startMenuOnOpen) {
        if (ll::memory::hookRva(off.startMenuOnOpen, (void*)&hooked_startMenuOnOpen, &sOrigStartMenuOnOpen)) {
            logger.info("Hooked StartMenu::onOpen at RVA 0x", std::hex, off.startMenuOnOpen);
        }
    }

    if (off.playButton) {
        if (ll::memory::hookRva(off.playButton, (void*)&hooked_playButton, &sOrigPlayButton)) {
            logger.info("Hooked PlayButton at RVA 0x", std::hex, off.playButton);
        }
    }

    if (off.settingsButton) {
        if (ll::memory::hookRva(off.settingsButton, (void*)&hooked_settingsButton, &sOrigSettingsButton)) {
            logger.info("Hooked SettingsButton at RVA 0x", std::hex, off.settingsButton);
        }
    }

    if (off.menuExit) {
        if (ll::memory::hookRva(off.menuExit, (void*)&hooked_menuExit, &sOrigMenuExit)) {
            logger.info("Hooked MenuExit at RVA 0x", std::hex, off.menuExit);
        }
    }

    logger.info("LeviLamroid UI hook subsystem initialized.");
    return true;
}

} // namespace levilamroid::hooks
