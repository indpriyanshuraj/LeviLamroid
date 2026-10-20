#pragma once
#include "ll/api/Macros.h"
#include <string>
#include <dlfcn.h>
#include <cstdint>

extern "C" {
LL_API void ll_ui_force_json_ui(const char* name);
LL_API bool ll_ui_is_forced_json_ui(const char* name);
LL_API void ll_ui_register_button(const char* buttonName, void (*handler)(void* controller, void* propertyBag));
LL_API bool ll_ui_dispatch_button(const char* buttonName, void* controller, void* propertyBag);

// Screen Navigation & Lifecycle APIs
LL_API bool ll_ui_open_screen(const char* screenName);
LL_API bool ll_ui_close_screen();
LL_API const char* ll_ui_get_current_screen();
LL_API bool ll_ui_is_adv_screen_visible();
LL_API void ll_ui_set_adv_screen_visible(bool visible);
LL_API void ll_ui_send_toast(const char* title, const char* message, const char* iconItem, int durationMs);

LL_API void ll_ui_on_start_menu_open(void (*cb)());
LL_API void ll_ui_on_pause_menu_open(void (*cb)(void* pauseScreenController));
LL_API void ll_ui_on_pause_menu_close(void (*cb)(void* pauseScreenController));
LL_API bool ll_ui_is_in_pause_menu();
LL_API void ll_ui_on_play_button_clicked(void (*cb)());
LL_API void ll_ui_on_settings_button_clicked(void (*cb)());
LL_API void ll_ui_on_realms_button_clicked(void (*cb)());
LL_API void ll_ui_on_dressing_room_button_clicked(void (*cb)());
LL_API void ll_ui_on_marketplace_button_clicked(void (*cb)());
LL_API void ll_ui_on_menu_exit(void (*cb)());

// Inventory & Container Access APIs
LL_API void* ll_player_get_inventory(void* player);
LL_API bool ll_player_get_armor(void* player, int slot, void* outItemStack);
LL_API bool ll_player_set_armor(void* player, int slot, const void* itemStack);
LL_API bool ll_player_get_offhand(void* player, void* outItemStack);
LL_API bool ll_player_set_offhand(void* player, const void* itemStack);
LL_API int ll_player_get_selected_slot(void* player);
LL_API bool ll_player_set_selected_slot(void* player, int slot);

LL_API int ll_container_get_size(void* container);
LL_API bool ll_container_get_item(void* container, int slot, void* outItemStack);
LL_API bool ll_container_set_item(void* container, int slot, const void* itemStack);
LL_API bool ll_container_clear_slot(void* container, int slot);
LL_API bool ll_container_has_item(void* container, const char* typeName, int count);
}

namespace ll::ui {

inline void* getLoaderHandle() {
    static void* handle = nullptr;
    if (handle) return handle;
    handle = dlopen("liblevilamroid.so", RTLD_NOLOAD);
    if (!handle) handle = dlopen("libLeviLamroid.so", RTLD_NOLOAD);
    if (!handle) handle = RTLD_DEFAULT;
    return handle;
}

inline void forceJsonUIForScreen(const std::string& screenName) {
    using Fn = void (*)(const char*);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_force_json_ui"));
    if (fn) fn(screenName.c_str());
}

inline bool isForcedJsonUI(const std::string& screenName) {
    using Fn = bool (*)(const char*);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_is_forced_json_ui"));
    return fn ? fn(screenName.c_str()) : false;
}

inline void registerButton(const std::string& buttonName, void (*handler)(void* controller, void* propertyBag)) {
    using Fn = void (*)(const char*, void (*)(void*, void*));
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_register_button"));
    if (fn) fn(buttonName.c_str(), handler);
}

inline bool dispatchButton(const std::string& buttonName, void* controller, void* propertyBag) {
    using Fn = bool (*)(const char*, void*, void*);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_dispatch_button"));
    if (fn) return fn(buttonName.c_str(), controller, propertyBag);
    return false;
}

inline bool openScreen(const std::string& screenName) {
    using Fn = bool (*)(const char*);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_open_screen"));
    return fn ? fn(screenName.c_str()) : false;
}

inline bool closeScreen() {
    using Fn = bool (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_close_screen"));
    return fn ? fn() : false;
}

inline std::string getCurrentScreen() {
    using Fn = const char* (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_get_current_screen"));
    const char* str = fn ? fn() : nullptr;
    return str ? std::string(str) : std::string();
}

inline bool isAdvScreenVisible() {
    using Fn = bool (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_is_adv_screen_visible"));
    return fn ? fn() : false;
}

inline void setAdvScreenVisible(bool visible) {
    using Fn = void (*)(bool);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_set_adv_screen_visible"));
    if (fn) fn(visible);
}

inline void sendToast(const std::string& title, const std::string& message, const std::string& iconItem = "", int durationMs = 3000) {
    using Fn = void (*)(const char*, const char*, const char*, int);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_send_toast"));
    if (fn) fn(title.c_str(), message.c_str(), iconItem.c_str(), durationMs);
}

inline void onStartMenuOpen(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_start_menu_open"));
    if (fn) fn(cb);
}

inline void onPauseMenuOpen(void (*cb)(void*)) {
    using Fn = void (*)(void (*)(void*));
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_pause_menu_open"));
    if (fn) fn(cb);
}

inline void onPauseMenuClose(void (*cb)(void*)) {
    using Fn = void (*)(void (*)(void*));
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_pause_menu_close"));
    if (fn) fn(cb);
}

inline bool isInPauseMenu() {
    using Fn = bool (*)();
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_is_in_pause_menu"));
    return fn ? fn() : false;
}

inline void onPlayButtonClicked(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_play_button_clicked"));
    if (fn) fn(cb);
}

inline void onSettingsButtonClicked(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_settings_button_clicked"));
    if (fn) fn(cb);
}

inline void onRealmsButtonClicked(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_realms_button_clicked"));
    if (fn) fn(cb);
}

inline void onDressingRoomButtonClicked(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_dressing_room_button_clicked"));
    if (fn) fn(cb);
}

inline void onMarketplaceButtonClicked(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_marketplace_button_clicked"));
    if (fn) fn(cb);
}

inline void onMenuExit(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_menu_exit"));
    if (fn) fn(cb);
}

} // namespace ll::ui

#include "ll/api/inventory/PlayerInventory.h"
