#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ui/UIEvents.h"
#include "ll/api/ui/UI.h"
#include <vector>
#include <string>

namespace levilamroid::hooks {

void setInWorld(bool inWorld);
void setIsMainMenu(bool isMenu);

static void* sOrigGetTechStackForScreen = nullptr;
static void* sOrigStartMenuOnOpen = nullptr;
static void* sOrigPlayButton = nullptr;
static void* sOrigSettingsButton = nullptr;
static void* sOrigRealmsButton = nullptr;
static void* sOrigDressingRoomButton = nullptr;
static void* sOrigMarketplaceButton = nullptr;
static void* sOrigMenuExit = nullptr;

static std::vector<std::string> sForceJsonScreens;
static std::vector<void(*)()> sStartMenuOpenCallbacks;
static std::vector<void(*)()> sPlayButtonCallbacks;
static std::vector<void(*)()> sSettingsButtonCallbacks;
static std::vector<void(*)()> sRealmsButtonCallbacks;
static std::vector<void(*)()> sDressingRoomButtonCallbacks;
static std::vector<void(*)()> sMarketplaceButtonCallbacks;
static std::vector<void(*)()> sMenuExitCallbacks;

static int hooked_getTechStackForScreen(void* self, const std::string& name) {
    if (name == "/main-menu" || name == "start_screen" || name == "start_screen_pocket") {
        setInWorld(false);
        setIsMainMenu(true);
    } else if (name.find("hud") != std::string::npos || 
               name.find("gameplay") != std::string::npos ||
               name.find("in_game") != std::string::npos ||
               name.find("in-game") != std::string::npos ||
               name.find("pause") != std::string::npos) {
        setInWorld(true);
        setIsMainMenu(false);
    }

    for (const auto& s : sForceJsonScreens) {
        if (name == s) return 0; // Force JsonUI (0)
    }
    ll::event::ScreenTechStackQueryEvent event(name, 1);
    ll::event::EventBus::getInstance().publish(event);
    if (event.isCancelled() || event.getTechStack() == 0) {
        return 0; // Force JsonUI (0)
    }
    if (sOrigGetTechStackForScreen) {
        return reinterpret_cast<int(*)(void*, const std::string&)>(sOrigGetTechStackForScreen)(self, name);
    }
    return 0;
}

static void hooked_startMenuOnOpen(void* self) {
    setInWorld(false);
    setIsMainMenu(true);
    for (auto cb : sStartMenuOpenCallbacks) {
        if (cb) cb();
    }
    ll::event::StartMenuOpenEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigStartMenuOnOpen) {
        reinterpret_cast<void(*)(void*)>(sOrigStartMenuOnOpen)(self);
    }
}

static void hooked_playButton(void* self) {
    for (auto cb : sPlayButtonCallbacks) {
        if (cb) cb();
    }
    ll::event::PlayButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigPlayButton) {
        reinterpret_cast<void(*)(void*)>(sOrigPlayButton)(self);
    }
}

static void hooked_settingsButton(void* self) {
    for (auto cb : sSettingsButtonCallbacks) {
        if (cb) cb();
    }
    ll::event::SettingsButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigSettingsButton) {
        reinterpret_cast<void(*)(void*)>(sOrigSettingsButton)(self);
    }
}

static void hooked_realmsButton(void* self) {
    for (auto cb : sRealmsButtonCallbacks) {
        if (cb) cb();
    }
    ll::event::RealmsButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigRealmsButton) {
        reinterpret_cast<void(*)(void*)>(sOrigRealmsButton)(self);
    }
}

static void hooked_dressingRoomButton(void* self) {
    for (auto cb : sDressingRoomButtonCallbacks) {
        if (cb) cb();
    }
    ll::event::DressingRoomButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigDressingRoomButton) {
        reinterpret_cast<void(*)(void*)>(sOrigDressingRoomButton)(self);
    }
}

static void hooked_marketplaceButton(void* self) {
    for (auto cb : sMarketplaceButtonCallbacks) {
        if (cb) cb();
    }
    ll::event::MarketplaceButtonClickedEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigMarketplaceButton) {
        reinterpret_cast<void(*)(void*)>(sOrigMarketplaceButton)(self);
    }
}

static void hooked_menuExit(void* self) {
    for (auto cb : sMenuExitCallbacks) {
        if (cb) cb();
    }
    ll::event::MenuExitEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigMenuExit) {
        reinterpret_cast<void(*)(void*)>(sOrigMenuExit)(self);
    }
}

bool installUIHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.getTechStackForScreen) {
        if (ll::memory::hookRva(off.getTechStackForScreen, (void*)&hooked_getTechStackForScreen, &sOrigGetTechStackForScreen)) {
            logger.info("Hooked ScreenTechStackSelector::getTechStackForScreen at RVA 0x", std::hex, off.getTechStackForScreen);
        }
    }

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

    if (off.realmsButton) {
        if (ll::memory::hookRva(off.realmsButton, (void*)&hooked_realmsButton, &sOrigRealmsButton)) {
            logger.info("Hooked RealmsButton at RVA 0x", std::hex, off.realmsButton);
        }
    }

    if (off.dressingRoomButton) {
        if (ll::memory::hookRva(off.dressingRoomButton, (void*)&hooked_dressingRoomButton, &sOrigDressingRoomButton)) {
            logger.info("Hooked DressingRoomButton at RVA 0x", std::hex, off.dressingRoomButton);
        }
    }

    if (off.marketplaceButton) {
        if (ll::memory::hookRva(off.marketplaceButton, (void*)&hooked_marketplaceButton, &sOrigMarketplaceButton)) {
            logger.info("Hooked MarketplaceButton at RVA 0x", std::hex, off.marketplaceButton);
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

extern "C" {
LL_API void ll_ui_force_json_ui(const char* name) {
    if (name) ::levilamroid::hooks::sForceJsonScreens.emplace_back(name);
}
LL_API void ll_ui_on_start_menu_open(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sStartMenuOpenCallbacks.push_back(cb);
}
LL_API void ll_ui_on_play_button_clicked(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sPlayButtonCallbacks.push_back(cb);
}
LL_API void ll_ui_on_settings_button_clicked(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sSettingsButtonCallbacks.push_back(cb);
}
LL_API void ll_ui_on_realms_button_clicked(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sRealmsButtonCallbacks.push_back(cb);
}
LL_API void ll_ui_on_dressing_room_button_clicked(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sDressingRoomButtonCallbacks.push_back(cb);
}
LL_API void ll_ui_on_marketplace_button_clicked(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sMarketplaceButtonCallbacks.push_back(cb);
}
LL_API void ll_ui_on_menu_exit(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sMenuExitCallbacks.push_back(cb);
}
}
