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
void setIsLoading(bool loading);
bool isLoading();

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

static std::string sLastScreenName;
static bool sInSubMenu = false;

void dispatchStartMenuOpen() {
    for (auto cb : sStartMenuOpenCallbacks) {
        if (cb) cb();
    }
    ll::event::StartMenuOpenEvent event;
    ll::event::EventBus::getInstance().publish(event);
}

static void* sOrigStartMenuTick = nullptr;
static std::atomic<bool> sFirstTickReceived = false;

static void hooked_startMenuTick(void* self) {
    if (isLoading() || !sFirstTickReceived.load()) {
        sFirstTickReceived.store(true);
        setIsLoading(false);
        setIsMainMenu(true);
        setInWorld(false);
        sInSubMenu = false;
        dispatchStartMenuOpen();
    } else if (sInSubMenu) {
        sInSubMenu = false;
        setIsMainMenu(true);
        setInWorld(false);
        dispatchStartMenuOpen();
    }

    if (sOrigStartMenuTick) {
        reinterpret_cast<void(*)(void*)>(sOrigStartMenuTick)(self);
    }
}

static int hooked_getTechStackForScreen(void* self, const std::string& name) {
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
    sInSubMenu = false;
    sLastScreenName = "/main-menu";
    if (!isLoading()) {
        setInWorld(false);
        setIsMainMenu(true);
        dispatchStartMenuOpen();
    }
    if (sOrigStartMenuOnOpen) {
        reinterpret_cast<void(*)(void*)>(sOrigStartMenuOnOpen)(self);
    }
}

static void hooked_playButton(void* self) {
    sInSubMenu = true;
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
    sInSubMenu = true;
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
    sInSubMenu = true;
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
    sInSubMenu = true;
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
    sInSubMenu = true;
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

    // ScreenTechStackSelector::getTechStackForScreen
    // Raw RVA: 1.26.44.3: 0x0938B7B4 | 1.26.45.1: 0x0938B714 | 1.26.51.1: 0x0974E7A4 | 1.26.52.3: 0x0974E844
    if (ll::memory::hookSignature("FD 7B BD A9 F5 0B 00 F9 F4 4F 02 A9 FD 03 00 91 F4 03 01 AA F5 03 00 AA ? ? ? ? 20 01 00 B4 F3 03 00 AA 00 1C 40 F9 40 01 00 B4",
                                  (void*)&hooked_getTechStackForScreen, &sOrigGetTechStackForScreen, off.getTechStackForScreen)) { /* 0x0938B7B4 */
        logger.info("Hooked ScreenTechStackSelector::getTechStackForScreen via signature [Raw RVA: 0x", std::hex, off.getTechStackForScreen, "]");
    }

    // StartMenuScreenController::tick
    // Raw RVA: 1.26.44.3: 0x09EC0678 | 1.26.45.1: 0x09EC05D8 | 1.26.51.1: 0x0A2E1F84 | 1.26.52.3: 0x0A2E1FF4
    if (ll::memory::hookSignature("FD 7B BA A9 FC 6F 01 A9 FA 67 02 A9 F8 5F 03 A9 F6 57 04 A9 F4 4F 05 A9",
                                  (void*)&hooked_startMenuTick, &sOrigStartMenuTick, off.startMenuTick)) {
        logger.info("Hooked StartMenu::tick via signature [Raw RVA: 0x", std::hex, off.startMenuTick, "]");
    }

    // StartMenuScreenController::_onOpen (concrete override, slot 14)
    // Raw RVA: 1.26.44.3: 0x09EBD9FC | 1.26.45.1: 0x09EBD95C | 1.26.51.1: 0x0A2DF2A4 | 1.26.52.3: 0x0A2DF314
    if (ll::memory::hookSignature("FF ? ? D1 FD 7B ? A9 FC 6F ? A9 FA 67 ? A9 F8 5F ? A9 F6 57 ? A9 F4 4F ? A9 FD ? 04 91 ? D0 3B D5 D8 1B 98 52 F3 03 00 AA ? 17 40 F9 B8 D5 BB 72 A8 03 1F F8",
                                  (void*)&hooked_startMenuOnOpen, &sOrigStartMenuOnOpen, off.startMenuOnOpen)) { /* 0x09EBD9FC */
        logger.info("Hooked StartMenu::onOpen via signature [Raw RVA: 0x", std::hex, off.startMenuOnOpen, "]");
    }

    // StartMenuScreenController::_onPlayButton
    // Raw RVA: 1.26.44.3: 0x09EF3E04 | 1.26.45.1: 0x09EF3D64 | 1.26.51.1: 0x0A3133DC | 1.26.52.3: 0x0A31344C
    if (ll::memory::hookSignature("FF 83 02 D1 FD 7B 07 A9 F6 57 08 A9 F4 4F 09 A9 FD C3 01 91 56 D0 3B D5 ? ? FC ? 21 ? ? 91 C8 16 40 F9 02 04 80 52 A8 83 1F F8 13 04 40 F9",
                                  (void*)&hooked_playButton, &sOrigPlayButton, off.playButton)) { /* 0x09EF3E04 */
        logger.info("Hooked PlayButton via signature [Raw RVA: 0x", std::hex, off.playButton, "]");
    }

    // StartMenuScreenController::_onSettingsButton
    // Raw RVA: 1.26.44.3: 0x09EF7360 | 1.26.45.1: 0x09EF72C0 | 1.26.51.1: 0x0A31713C | 1.26.52.3: 0x0A3171AC
    if (ll::memory::hookSignature("FF 43 02 D1 FD 7B 06 A9 F6 57 07 A9 F4 4F 08 A9 FD 83 01 91 56 D0 3B D5 ? ? FC ? 21 ? ? 91 C8 16 40 F9 82 04 80 52 A8 83 1F F8 13 04 40 F9",
                                  (void*)&hooked_settingsButton, &sOrigSettingsButton, off.settingsButton)) { /* 0x09EF7360 */
        logger.info("Hooked SettingsButton via signature [Raw RVA: 0x", std::hex, off.settingsButton, "]");
    }

    // StartMenuScreenController::_onRealmsButton
    // Raw RVA: 1.26.44.3: 0x09EF7690 | 1.26.45.1: 0x09EF75F0 | 1.26.51.1: 0x0A31746C | 1.26.52.3: 0x0A3174DC
    if (ll::memory::hookSignature("FF 43 02 D1 FD 7B 06 A9 F6 57 07 A9 F4 4F 08 A9 FD 83 01 91 56 D0 3B D5 C8 16 40 F9 A8 83 1F F8 13 04 40 F9 E0 03 13 AA ? ? FF 97 A0 0F 00 36",
                                  (void*)&hooked_realmsButton, &sOrigRealmsButton, off.realmsButton)) { /* 0x09EF7690 */
        logger.info("Hooked RealmsButton via signature [Raw RVA: 0x", std::hex, off.realmsButton, "]");
    }

    // StartMenuScreenController::_onDressingRoomButton
    // Raw RVA: 1.26.44.3: 0x09EF8310 | 1.26.45.1: 0x09EF8270 | 1.26.51.1: 0x0A3180EC | 1.26.52.3: 0x0A31815C
    if (ll::memory::hookSignature("FF 43 02 D1 FD 7B 06 A9 F6 57 07 A9 F4 4F 08 A9 FD 83 01 91 56 D0 3B D5 ? ? FC B0 21 ? ? 91 C8 16 40 F9 02 05 80 52 A8 83 1F F8 13 04 40 F9",
                                  (void*)&hooked_dressingRoomButton, &sOrigDressingRoomButton, off.dressingRoomButton)) { /* 0x09EF8310 */
        logger.info("Hooked DressingRoomButton via signature [Raw RVA: 0x", std::hex, off.dressingRoomButton, "]");
    }

    // StartMenuScreenController::_onMarketplaceButton
    // Raw RVA: 1.26.44.3: 0x09EF8944 | 1.26.45.1: 0x09EF88A4 | 1.26.51.1: 0x0A318734 | 1.26.52.3: 0x0A3187A4
    if (ll::memory::hookSignature("FF 83 03 D1 FD 7B 0B A9 F6 57 0C A9 F4 4F 0D A9 FD C3 02 91 56 D0 3B D5 ? ? FC ? 21 ? ? 91 C8 16 40 F9 E2 04 80 52 A8 83 1F F8 13 04 40 F9",
                                  (void*)&hooked_marketplaceButton, &sOrigMarketplaceButton, off.marketplaceButton)) { /* 0x09EF8944 */
        logger.info("Hooked MarketplaceButton via signature [Raw RVA: 0x", std::hex, off.marketplaceButton, "]");
    }

    // StartMenuScreenController::_onMenuExit
    // Raw RVA: 1.26.44.3: 0x0A15252C | 1.26.45.1: 0x0A15248C | 1.26.51.1: 0x0A5674D0 | 1.26.52.3: 0x0A567540
    if (ll::memory::hookSignature("FD 7B BE A9 F3 0B 00 F9 FD 03 00 91 09 04 40 F9 F3 03 08 AA 20 6D 44 F9 ? ? EE 97 ? ? ? ? 08 ? ? 91 ? ? ? ? 29 ? ? 91 1F 00 00 72",
                                  (void*)&hooked_menuExit, &sOrigMenuExit, off.menuExit)) { /* 0x0A15252C */
        logger.info("Hooked MenuExit via signature [Raw RVA: 0x", std::hex, off.menuExit, "]");
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
