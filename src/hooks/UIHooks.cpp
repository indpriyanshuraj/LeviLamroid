#include "levilamroid/api/Logger.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "levilamroid/api/math/HashedString.h"
#include "ll/api/memory/Hook.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/ui/UIEvents.h"
#include "ll/api/ui/UI.h"
#include <atomic>
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>

static inline bool isPointerReadable(const void* ptr, size_t size = sizeof(void*)) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    if (v < 0x10000 || v > 0x7fffffffff) return false;
    if (v + size < v) return false;
    return true;
}

extern "C" LL_API void* ll_service_get_client_instance();

namespace levilamroid::hooks {

void setInWorld(bool inWorld);
void setIsMainMenu(bool isMenu);
void setIsLoading(bool loading);
bool isLoading();
void dispatchPauseMenuOpen(void* controller);
void dispatchPauseMenuClose(void* controller);

static void* sLastScreenController = nullptr;
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
    if (name == "pause_screen" || name == "pause") {
        dispatchPauseMenuOpen(sLastScreenController);
        return 0; // Force JsonUI (0) so pause_screen.json modifications render cleanly
    }
    if (name.find("advancement") != std::string::npos) {
        return 0; // Force JsonUI (0) for custom advancements screen
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

[[maybe_unused]] static void hooked_playButton(void* self) {
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

[[maybe_unused]] static void hooked_settingsButton(void* self) {
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

[[maybe_unused]] static void hooked_realmsButton(void* self) {
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

[[maybe_unused]] static void hooked_dressingRoomButton(void* self) {
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

[[maybe_unused]] static void hooked_marketplaceButton(void* self) {
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

[[maybe_unused]] static void hooked_menuExit(void* self) {
    for (auto cb : sMenuExitCallbacks) {
        if (cb) cb();
    }
    ll::event::MenuExitEvent event;
    ll::event::EventBus::getInstance().publish(event);
    if (!event.isCancelled() && sOrigMenuExit) {
        reinterpret_cast<void(*)(void*)>(sOrigMenuExit)(self);
    }
}

static std::vector<void(*)(void*)> sPauseMenuOpenCallbacks;
static std::vector<void(*)(void*)> sPauseMenuCloseCallbacks;
static std::atomic<bool> sIsInPauseMenu = false;

void dispatchPauseMenuOpen(void* controller) {
    if (!sIsInPauseMenu.exchange(true)) {
        ll::Logger::getInstance().info("§6[LeviLamroid] §ePause menu OPEN detected.");
        for (auto cb : sPauseMenuOpenCallbacks) {
            if (cb) cb(controller);
        }
        ll::event::PauseMenuOpenEvent event(controller);
        ll::event::EventBus::getInstance().publish(event);
    }
}

void dispatchPauseMenuClose(void* controller) {
    if (sIsInPauseMenu.exchange(false)) {
        ll::Logger::getInstance().info("§6[LeviLamroid] §ePause menu CLOSE detected.");
        for (auto cb : sPauseMenuCloseCallbacks) {
            if (cb) cb(controller);
        }
        ll::event::PauseMenuCloseEvent event(controller);
        ll::event::EventBus::getInstance().publish(event);
    }
}
static std::unordered_map<std::string, std::vector<void (*)(void*, void*)>> sCustomButtonHandlers;
static std::unordered_map<uint32_t, std::vector<void (*)(void*, void*)>> sCustomButtonHashHandlers;
static std::mutex sButtonHandlerMutex;

static inline uint32_t fnv1a_32(std::string_view str) {
    uint32_t h = 0x811c9dc5;
    for (char c : str) {
        h = ((h ^ static_cast<uint8_t>(c)) * 0x01000193) & 0xFFFFFFFF;
    }
    return h;
}

static void* sOrigScreenControllerHandleEvent = nullptr;

static int hooked_screenControllerHandleEvent(void* self, void* screenEvent) {
    sLastScreenController = self;
    if (screenEvent && isPointerReadable(screenEvent, 32)) {
        uint8_t* evBytes = reinterpret_cast<uint8_t*>(screenEvent);
        uint32_t b8 = *reinterpret_cast<uint32_t*>(evBytes + 8);
        uint32_t b4 = *reinterpret_cast<uint32_t*>(evBytes + 4);
        uint32_t b0 = *reinterpret_cast<uint32_t*>(evBytes + 0);

        std::vector<void (*)(void*, void*)> callbacks;
        std::string matchedButton;

        {
            std::lock_guard<std::mutex> lock(sButtonHandlerMutex);

            // 1. Direct string scan across the ScreenEvent object (first 48 bytes)
            for (const auto& pair : sCustomButtonHandlers) {
                const std::string& name = pair.first;
                if (name.empty()) continue;
                if (isPointerReadable(evBytes, 48)) {
                    for (size_t offset = 0; offset <= 32; offset += 8) {
                        void* ptrCandidate = *reinterpret_cast<void**>(evBytes + offset);
                        if (ptrCandidate && isPointerReadable(ptrCandidate, name.size())) {
                            if (std::memcmp(ptrCandidate, name.data(), name.size()) == 0) {
                                matchedButton = name;
                                callbacks = pair.second;
                                break;
                            }
                        }
                    }
                    if (!matchedButton.empty()) break;

                    std::string_view rawChunk(reinterpret_cast<const char*>(evBytes), 48);
                    if (rawChunk.find(name) != std::string_view::npos) {
                        matchedButton = name;
                        callbacks = pair.second;
                        break;
                    }
                }
            }

            // 2. Hash lookups across event fields (b8 is canonical Bedrock button id)
            if (callbacks.empty()) {
                uint32_t candidates[] = {
                    b8, b4, b0,
                    isPointerReadable(evBytes + 12, 4) ? *reinterpret_cast<uint32_t*>(evBytes + 12) : 0u,
                    isPointerReadable(evBytes + 16, 4) ? *reinterpret_cast<uint32_t*>(evBytes + 16) : 0u,
                    isPointerReadable(evBytes + 20, 4) ? *reinterpret_cast<uint32_t*>(evBytes + 20) : 0u,
                    isPointerReadable(evBytes + 24, 4) ? *reinterpret_cast<uint32_t*>(evBytes + 24) : 0u
                };
                for (uint32_t h : candidates) {
                    if (h == 0) continue;
                    auto it = sCustomButtonHashHandlers.find(h);
                    if (it != sCustomButtonHashHandlers.end()) {
                        callbacks = it->second;
                        char hexBuf[32];
                        std::snprintf(hexBuf, sizeof(hexBuf), "0x%08X", h);
                        matchedButton = hexBuf;
                        break;
                    }
                }
            }
        }

        if (!callbacks.empty()) {
            static auto sLastClickTime = std::chrono::steady_clock::time_point::min();
            static std::string sLastFiredName;
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - sLastClickTime).count();
            if (sLastFiredName == matchedButton && elapsed < 350) {
                if (sOrigScreenControllerHandleEvent) {
                    using Fn = int (*)(void*, void*);
                    return reinterpret_cast<Fn>(sOrigScreenControllerHandleEvent)(self, screenEvent);
                }
                return 0;
            }
            sLastClickTime = now;
            sLastFiredName = matchedButton;

            ll::Logger::getInstance().info("§6[LeviLamroid] §aScreenController::handleEvent dispatched button: §e", matchedButton);
            for (auto cb : callbacks) {
                if (cb) cb(self, screenEvent);
            }
            ll::event::ButtonClickedEvent ev(matchedButton, self, screenEvent);
            ll::event::EventBus::getInstance().publish(ev);
        }
    }
    if (sOrigScreenControllerHandleEvent) {
        using Fn = int (*)(void*, void*);
        return reinterpret_cast<Fn>(sOrigScreenControllerHandleEvent)(self, screenEvent);
    }
    return 0;
}

// ── Step B & C Verified OreUI & JSON UI Hooks ──
bool installUIHooks() {
    auto& logger = ll::Logger::getInstance();
    const auto& off = ::levilamroid::mcpe::getOffsets();

    if (off.getTechStackForScreen) {
        if (ll::memory::hookRva(off.getTechStackForScreen, reinterpret_cast<void*>(&hooked_getTechStackForScreen), &sOrigGetTechStackForScreen)) {
            logger.info("Hooked getTechStackForScreen [RVA: 0x", std::hex, off.getTechStackForScreen, "] for OreUI/JsonUI routing.");
        }
    }

    if (off.screenControllerHandleEvent) {
        if (ll::memory::hookRva(off.screenControllerHandleEvent, reinterpret_cast<void*>(&hooked_screenControllerHandleEvent), &sOrigScreenControllerHandleEvent)) {
            logger.info("Hooked ScreenController::handleEvent [RVA: 0x", std::hex, off.screenControllerHandleEvent, "] for universal button interception.");
        }
    }

    if (off.startMenuTick) {
        if (ll::memory::hookRva(off.startMenuTick, reinterpret_cast<void*>(&hooked_startMenuTick), &sOrigStartMenuTick)) {
            logger.info("Hooked startMenuTick [RVA: 0x", std::hex, off.startMenuTick, "] for start menu lifecycle events.");
        }
    }

    if (off.startMenuOnOpen) {
        if (ll::memory::hookRva(off.startMenuOnOpen, reinterpret_cast<void*>(&hooked_startMenuOnOpen), &sOrigStartMenuOnOpen)) {
            logger.info("Hooked startMenuOnOpen [RVA: 0x", std::hex, off.startMenuOnOpen, "].");
        }
    }

    logger.info("LeviLamroid UI hook subsystem initialized with universal button and container support.");
    return true;
}

} // namespace levilamroid::hooks

extern "C" {

LL_API void ll_ui_force_json_ui(const char* name) {
    if (name) ::levilamroid::hooks::sForceJsonScreens.emplace_back(name);
}

LL_API bool ll_ui_is_forced_json_ui(const char* name) {
    if (!name) return false;
    for (const auto& s : ::levilamroid::hooks::sForceJsonScreens) {
        if (s == name) return true;
    }
    return false;
}

LL_API void ll_ui_register_button(const char* buttonName, void (*handler)(void* controller, void* propertyBag)) {
    if (!buttonName || !handler) return;
    std::string bName = buttonName;
    uint32_t fnv = ::levilamroid::hooks::fnv1a_32(bName);
    uint64_t h64 = ::levilamroid::HashedString::computeHash(bName);
    uint32_t h32_low = static_cast<uint32_t>(h64 & 0xFFFFFFFF);
    uint32_t h32_hi = static_cast<uint32_t>(h64 >> 32);

    std::lock_guard<std::mutex> lock(::levilamroid::hooks::sButtonHandlerMutex);
    ::levilamroid::hooks::sCustomButtonHandlers[bName].push_back(handler);
    ::levilamroid::hooks::sCustomButtonHashHandlers[fnv].push_back(handler);
    ::levilamroid::hooks::sCustomButtonHashHandlers[h32_low].push_back(handler);
    ::levilamroid::hooks::sCustomButtonHashHandlers[h32_hi].push_back(handler);

    // Also register alternate form: with/without "button." prefix
    if (bName.rfind("button.", 0) == 0) {
        std::string stripped = bName.substr(7);
        uint32_t s_fnv = ::levilamroid::hooks::fnv1a_32(stripped);
        uint64_t s_h64 = ::levilamroid::HashedString::computeHash(stripped);
        ::levilamroid::hooks::sCustomButtonHandlers[stripped].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[s_fnv].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[static_cast<uint32_t>(s_h64 & 0xFFFFFFFF)].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[static_cast<uint32_t>(s_h64 >> 32)].push_back(handler);
    } else {
        std::string prefixed = "button." + bName;
        uint32_t p_fnv = ::levilamroid::hooks::fnv1a_32(prefixed);
        uint64_t p_h64 = ::levilamroid::HashedString::computeHash(prefixed);
        ::levilamroid::hooks::sCustomButtonHandlers[prefixed].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[p_fnv].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[static_cast<uint32_t>(p_h64 & 0xFFFFFFFF)].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[static_cast<uint32_t>(p_h64 >> 32)].push_back(handler);
    }

    // Specific precomputed hashes for advancements button across Bedrock revisions
    if (bName == "button.advancements_open" || bName == "advancements_open") {
        ::levilamroid::hooks::sCustomButtonHashHandlers[0xC312F1AF].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[0x668F6DA3].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[0x80D844EB].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[0x7883CF23].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[0x327BD7EB].push_back(handler);
        ::levilamroid::hooks::sCustomButtonHashHandlers[0x80974FEB].push_back(handler);
    }

    ll::Logger::getInstance().info("§6[LeviLamroid] §aRegistered custom JSON UI button listener: §b", bName,
                                " §7(HashedString: 0x", std::hex, h32_low, " | FNV32: 0x", fnv, ")");
}

LL_API bool ll_ui_dispatch_button(const char* buttonName, void* controller, void* propertyBag) {
    if (!buttonName) return false;
    std::string bName = buttonName;
    uint32_t fnv = ::levilamroid::hooks::fnv1a_32(bName);
    uint32_t h32 = static_cast<uint32_t>(::levilamroid::HashedString::computeHash(bName) & 0xFFFFFFFF);
    std::vector<void (*)(void*, void*)> callbacks;
    {
        std::lock_guard<std::mutex> lock(::levilamroid::hooks::sButtonHandlerMutex);
        auto itH = ::levilamroid::hooks::sCustomButtonHashHandlers.find(h32);
        if (itH != ::levilamroid::hooks::sCustomButtonHashHandlers.end()) {
            callbacks = itH->second;
        } else {
            auto itF = ::levilamroid::hooks::sCustomButtonHashHandlers.find(fnv);
            if (itF != ::levilamroid::hooks::sCustomButtonHashHandlers.end()) {
                callbacks = itF->second;
            } else {
                auto itN = ::levilamroid::hooks::sCustomButtonHandlers.find(bName);
                if (itN != ::levilamroid::hooks::sCustomButtonHandlers.end()) {
                    callbacks = itN->second;
                }
            }
        }
    }
    for (auto cb : callbacks) {
        if (cb) cb(controller, propertyBag);
    }
    ll::event::ButtonClickedEvent ev(bName, controller, propertyBag);
    ll::event::EventBus::getInstance().publish(ev);
    return !callbacks.empty();
}

static std::atomic<bool> sAdvScreenVisible{false};

LL_API bool ll_ui_open_screen(const char* screenName) {
    if (!screenName) return false;

    static auto sLastOpenTime = std::chrono::steady_clock::time_point::min();
    static std::string sLastOpenedScreen;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - sLastOpenTime).count();
    if (sLastOpenedScreen == screenName && elapsed < 500) {
        ll::Logger::getInstance().warn("§e[LeviLamroid] Suppressed duplicate screen open for: ", screenName);
        return true;
    }
    sLastOpenedScreen = screenName;
    sLastOpenTime = now;

    ::levilamroid::hooks::sLastScreenName = screenName;
    if (std::string_view(screenName).find("advancement") != std::string_view::npos) {
        sAdvScreenVisible.store(true);
        ll::Logger::getInstance().info("§6[LeviLamroid] §aAdvancements UI toggled VISIBLE, pushing advancements_screen to SceneStack...");
    }
    ll::Logger::getInstance().info("§6[LeviLamroid] §aOpening screen: §e", screenName);

    void* client = ll_service_get_client_instance();
    if (!client) {
        ll::Logger::getInstance().error("§c[LeviLamroid] Cannot open screen: ClientInstance is null!");
        return false;
    }

    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t* clientVtable = *reinterpret_cast<uintptr_t**>(client);

    // ClientInstance::getSceneFactory()
    void* sceneFactory = *(void**)((uintptr_t)client + 0x1e8);
    if (!sceneFactory && clientVtable && off.clientInstanceSceneFactorySlot > 0) {
        using GetSceneFactoryFn = void* (*)(void*);
        auto getSceneFactory = reinterpret_cast<GetSceneFactoryFn>(clientVtable[off.clientInstanceSceneFactorySlot]);
        if (getSceneFactory) {
            sceneFactory = getSceneFactory(client);
        }
    }
    if (!sceneFactory) {
        ll::Logger::getInstance().error("§c[LeviLamroid] Cannot open screen: SceneFactory is null!");
        return false;
    }

    // ClientInstance::getCurrentSceneStack()
    void* sceneStack = *(void**)((uintptr_t)client + 0x5b8);
    if (!sceneStack && clientVtable && off.clientInstanceSceneStackSlot > 0) {
        void* fn = reinterpret_cast<void*>(clientVtable[off.clientInstanceSceneStackSlot]);
        if (fn) {
            alignas(16) uint8_t ptrBuf[32] = {0};
            register void* x0_in __asm__("x0") = client;
            register void* x8_in __asm__("x8") = ptrBuf;
            register void* fn_in __asm__("x16") = fn;
            __asm__ volatile(
                "blr %2\n"
                : "+r"(x0_in), "+r"(x8_in)
                : "r"(fn_in)
                : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x17", "x30", "memory"
            );
            sceneStack = *reinterpret_cast<void**>(ptrBuf + 16);
        }
    }
    if (!sceneStack) {
        ll::Logger::getInstance().error("§c[LeviLamroid] Cannot open screen: SceneStack is null!");
        return false;
    }

    void* ctrl = ::levilamroid::hooks::sLastScreenController;
    if (!ctrl || !isPointerReadable(ctrl, 32)) {
        ll::Logger::getInstance().warn("§e[LeviLamroid] Cannot open UI screen directly in-world: No active ScreenController. Please open via Pause Menu!");
        return false;
    }

    uintptr_t mcBase = ll::memory::getMinecraftBase();
    struct RawSharedPtr {
        void* ptr;
        void* rep;
    };
    RawSharedPtr ctrlShared = { ctrl, nullptr };

    // Step 1: SceneFactory::createUIScene(screenName, ctrlShared)
    alignas(16) uint8_t uiSceneShared[16] = {0};
    std::string sName = screenName;
    uintptr_t rvaCreateUIScene = off.sceneFactoryCreateUIScene;
    void* fnCreateUIScene = rvaCreateUIScene ? reinterpret_cast<void*>(mcBase + rvaCreateUIScene) : nullptr;
    if (fnCreateUIScene) {
        register void* x0_f __asm__("x0") = sceneFactory;
        register void* x1_n __asm__("x1") = &sName;
        register void* x2_c __asm__("x2") = &ctrlShared;
        register void* x8_r __asm__("x8") = uiSceneShared;
        register void* fn_call __asm__("x16") = fnCreateUIScene;
        __asm__ volatile(
            "blr %4\n"
            : "+r"(x0_f), "+r"(x1_n), "+r"(x2_c), "+r"(x8_r)
            : "r"(fn_call)
            : "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x17", "x30", "memory"
        );
    }

    void* uiScenePtr = *reinterpret_cast<void**>(uiSceneShared);
    if (!uiScenePtr) {
        ll::Logger::getInstance().error("§c[LeviLamroid] Failed to create UI Scene for: ", screenName);
        return false;
    }

    ll::Logger::getInstance().info("§a[LeviLamroid] UIScene created successfully! [UIScene: 0x", std::hex, (uintptr_t)uiScenePtr, "]");

    // Step 2: Wrap UIScene into AbstractScene via SceneFactory::_createScreen
    alignas(16) uint8_t abstractSceneShared[16] = {0};
    uintptr_t rvaCreateScreen = off.sceneFactoryCreateScreenAbstract;
    void* fnCreateScreen = rvaCreateScreen ? reinterpret_cast<void*>(mcBase + rvaCreateScreen) : nullptr;
    if (fnCreateScreen) {
        register void* x0_cs __asm__("x0") = sceneFactory;
        register void* x1_cs __asm__("x1") = uiSceneShared;
        register void* x8_cs __asm__("x8") = abstractSceneShared;
        register void* fn_cs __asm__("x16") = fnCreateScreen;
        __asm__ volatile(
            "blr %3\n"
            : "+r"(x0_cs), "+r"(x1_cs), "+r"(x8_cs)
            : "r"(fn_cs)
            : "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x17", "x30", "memory"
        );
    }

    void* abstractScenePtr = *reinterpret_cast<void**>(abstractSceneShared);
    void* pushScenePtr = abstractScenePtr ? abstractSceneShared : uiSceneShared;

    ll::Logger::getInstance().info("§a[LeviLamroid] AbstractScene ready! [AbstractScene: 0x", std::hex, (uintptr_t)abstractScenePtr, "] Pushing to SceneStack...");

    // Step 3: ISceneStack::pushScreen
    uintptr_t* stackVtable = *reinterpret_cast<uintptr_t**>(sceneStack);
    size_t pushSlot = off.sceneStackPushScreenSlot ? off.sceneStackPushScreenSlot : 26;
    if (stackVtable && stackVtable[pushSlot]) {
        using PushScreenFn = void (*)(void* stack, void* sceneShared, bool flush);
        auto pushScreen = reinterpret_cast<PushScreenFn>(stackVtable[pushSlot]);
        pushScreen(sceneStack, pushScenePtr, false);
        ll::Logger::getInstance().info("§a[LeviLamroid] Screen pushed to SceneStack successfully!");
        return true;
    }

    return false;
}

LL_API bool ll_ui_close_screen() {
    sAdvScreenVisible.store(false);
    ll::Logger::getInstance().info("§6[LeviLamroid] §eClosing current screen");
    void* client = ll_service_get_client_instance();
    if (!client) return false;

    const auto& off = ::levilamroid::mcpe::getOffsets();
    void* sceneStack = *(void**)((uintptr_t)client + 0x5b8);
    if (!sceneStack) {
        uintptr_t* clientVtable = *reinterpret_cast<uintptr_t**>(client);
        size_t stackSlot = off.clientInstanceSceneStackSlot > 0 ? off.clientInstanceSceneStackSlot : (0x758 / 8);
        if (clientVtable && clientVtable[stackSlot]) {
            void* fn = reinterpret_cast<void*>(clientVtable[stackSlot]);
            alignas(16) uint8_t ptrBuf[32] = {0};
            register void* x0_in __asm__("x0") = client;
            register void* x8_in __asm__("x8") = ptrBuf;
            register void* fn_in __asm__("x16") = fn;
            __asm__ volatile(
                "blr %2\n"
                : "+r"(x0_in), "+r"(x8_in)
                : "r"(fn_in)
                : "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x9", "x10", "x11", "x12", "x13", "x14", "x15", "x17", "x30", "memory"
            );
            sceneStack = *reinterpret_cast<void**>(ptrBuf + 16);
        }
    }
    if (!sceneStack) return false;

    uintptr_t* stackVtable = *reinterpret_cast<uintptr_t**>(sceneStack);
    size_t popSlot = off.sceneStackSchedulePopSlot ? off.sceneStackSchedulePopSlot : 27;
    if (stackVtable && stackVtable[popSlot]) {
        using SchedulePopScreenFn = void (*)(void* stack, int totalPopNumber);
        auto schedulePopScreen = reinterpret_cast<SchedulePopScreenFn>(stackVtable[popSlot]);
        schedulePopScreen(sceneStack, 1);
        ::levilamroid::hooks::dispatchPauseMenuClose(nullptr);
        ll::Logger::getInstance().info("§a[LeviLamroid] Screen popped from SceneStack successfully!");
        return true;
    }
    return false;
}

LL_API bool ll_ui_is_adv_screen_visible() {
    return sAdvScreenVisible.load();
}

LL_API void ll_ui_set_adv_screen_visible(bool visible) {
    sAdvScreenVisible.store(visible);
}

LL_API const char* ll_ui_get_current_screen() {
    return ::levilamroid::hooks::sLastScreenName.c_str();
}

extern "C" LL_API void ll_command_dispatch_client(const char* rawInput);

LL_API void ll_ui_send_toast(const char* title, const char* message, const char* iconItem, int /*durationMs*/) {
    if (!title) return;
    ll::Logger::getInstance().info("§6[LeviLamroid Toast] §e", title, "§7: ", (message ? message : ""),
                                   " §b[Icon: ", (iconItem ? iconItem : "default"), "§b]");

    static thread_local bool sInToast = false;
    if (sInToast) return;
    sInToast = true;
    std::string text = "§6§l[" + std::string(title) + "§r§6§l] §e" + (message ? std::string(message) : "");
    std::string cmd = "/title @s actionbar " + text;
    ll_command_dispatch_client(cmd.c_str());
    sInToast = false;
}

LL_API void ll_ui_on_start_menu_open(void (*cb)()) {
    if (cb) ::levilamroid::hooks::sStartMenuOpenCallbacks.push_back(cb);
}

LL_API void ll_ui_on_pause_menu_open(void (*cb)(void* pauseScreenController)) {
    if (cb) ::levilamroid::hooks::sPauseMenuOpenCallbacks.push_back(cb);
}

LL_API void ll_ui_on_pause_menu_close(void (*cb)(void* pauseScreenController)) {
    if (cb) ::levilamroid::hooks::sPauseMenuCloseCallbacks.push_back(cb);
}

LL_API bool ll_ui_is_in_pause_menu() {
    return ::levilamroid::hooks::sIsInPauseMenu.load();
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

// ─── Inventory & Container Access APIs ─────────────────────────────────────

LL_API void* ll_player_get_inventory(void* player) {
    if (!player) return nullptr;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    void* inv = *reinterpret_cast<void**>(p + 0x1C8);
    if (!inv) inv = *reinterpret_cast<void**>(p + 0x1C0);
    if (!inv) inv = *reinterpret_cast<void**>(p + 0x140);
    return inv;
}

LL_API bool ll_player_get_armor(void* player, int slot, void* outItemStack) {
    if (!player || slot < 0 || slot > 3 || !outItemStack) return false;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    void* armorContainer = *reinterpret_cast<void**>(p + 0x228);
    if (!armorContainer) armorContainer = *reinterpret_cast<void**>(p + 0x218);
    if (armorContainer) {
        return ll_container_get_item(armorContainer, slot, outItemStack);
    }
    return false;
}

LL_API bool ll_player_set_armor(void* player, int slot, const void* itemStack) {
    if (!player || slot < 0 || slot > 3 || !itemStack) return false;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    void* armorContainer = *reinterpret_cast<void**>(p + 0x228);
    if (!armorContainer) armorContainer = *reinterpret_cast<void**>(p + 0x218);
    if (armorContainer) {
        return ll_container_set_item(armorContainer, slot, itemStack);
    }
    return false;
}

LL_API bool ll_player_get_offhand(void* player, void* outItemStack) {
    if (!player || !outItemStack) return false;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    void* offhandContainer = *reinterpret_cast<void**>(p + 0x240);
    if (!offhandContainer) offhandContainer = *reinterpret_cast<void**>(p + 0x230);
    if (offhandContainer) {
        return ll_container_get_item(offhandContainer, 0, outItemStack);
    }
    return false;
}

LL_API bool ll_player_set_offhand(void* player, const void* itemStack) {
    if (!player || !itemStack) return false;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    void* offhandContainer = *reinterpret_cast<void**>(p + 0x240);
    if (!offhandContainer) offhandContainer = *reinterpret_cast<void**>(p + 0x230);
    if (offhandContainer) {
        return ll_container_set_item(offhandContainer, 0, itemStack);
    }
    return false;
}

LL_API int ll_player_get_selected_slot(void* player) {
    if (!player) return 0;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    return static_cast<int>(*reinterpret_cast<uint8_t*>(p + 0x208));
}

LL_API bool ll_player_set_selected_slot(void* player, int slot) {
    if (!player || slot < 0 || slot > 8) return false;
    uint8_t* p = reinterpret_cast<uint8_t*>(player);
    *reinterpret_cast<uint8_t*>(p + 0x208) = static_cast<uint8_t>(slot);
    return true;
}

LL_API int ll_container_get_size(void* container) {
    if (!container) return 0;
    // Default player supplies container has 36 slots
    return 36;
}

LL_API bool ll_container_get_item(void* container, int slot, void* outItemStack) {
    if (!container || slot < 0 || !outItemStack) return false;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ll::memory::getMinecraftBase();
    if (off.containerGetItem) {
        using GetItemFn = void* (*)(void*, int);
        auto fn = reinterpret_cast<GetItemFn>(mcBase + off.containerGetItem);
        void* item = fn(container, slot);
        if (item) {
            std::memcpy(outItemStack, item, 152);
            return true;
        }
    }
    return false;
}

LL_API bool ll_container_set_item(void* container, int slot, const void* itemStack) {
    if (!container || slot < 0 || !itemStack) return false;
    const auto& off = ::levilamroid::mcpe::getOffsets();
    uintptr_t mcBase = ll::memory::getMinecraftBase();
    if (off.containerSetItem) {
        using SetItemFn = void (*)(void*, int, const void*);
        auto fn = reinterpret_cast<SetItemFn>(mcBase + off.containerSetItem);
        fn(container, slot, itemStack);
        return true;
    }
    return false;
}

LL_API bool ll_container_clear_slot(void* container, int slot) {
    if (!container || slot < 0) return false;
    alignas(8) uint8_t airStack[160] = {0};
    return ll_container_set_item(container, slot, airStack);
}

LL_API bool ll_container_has_item(void* container, const char* typeName, int count) {
    if (!container || !typeName || count <= 0) return false;
    int size = ll_container_get_size(container);
    int foundCount = 0;
    alignas(8) uint8_t rawStack[160] = {0};
    std::string_view targetView(typeName);
    if (targetView.rfind("minecraft:", 0) == 0) {
        targetView.remove_prefix(10);
    }

    for (int i = 0; i < size; ++i) {
        if (ll_container_get_item(container, i, rawStack)) {
            uint8_t stackCount = rawStack[34];
            if (stackCount > 0) {
                void* ctrl = *reinterpret_cast<void**>(rawStack + 0x08);
                if (ctrl && isPointerReadable(ctrl, sizeof(void*))) {
                    void* itemPtr = *reinterpret_cast<void**>(ctrl);
                    if (itemPtr && isPointerReadable(itemPtr, 0xF0)) {
                        const std::string* rawName = reinterpret_cast<const std::string*>(reinterpret_cast<uint8_t*>(itemPtr) + 0xD0);
                        if (rawName && isPointerReadable(rawName->data(), std::min<size_t>(rawName->size(), 64))) {
                            std::string_view itemName(*rawName);
                            if (itemName.rfind("minecraft:", 0) == 0) {
                                itemName.remove_prefix(10);
                            }
                            if (itemName == targetView || itemName.find(targetView) != std::string_view::npos) {
                                foundCount += stackCount;
                                if (foundCount >= count) return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return foundCount >= count;
}

} // extern "C"

