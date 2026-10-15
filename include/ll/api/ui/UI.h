#pragma once
#include "ll/api/Macros.h"
#include <string>
#include <dlfcn.h>

extern "C" {
LL_API void ll_ui_force_json_ui(const char* name);
LL_API void ll_ui_on_start_menu_open(void (*cb)());
LL_API void ll_ui_on_play_button_clicked(void (*cb)());
LL_API void ll_ui_on_settings_button_clicked(void (*cb)());
LL_API void ll_ui_on_realms_button_clicked(void (*cb)());
LL_API void ll_ui_on_dressing_room_button_clicked(void (*cb)());
LL_API void ll_ui_on_marketplace_button_clicked(void (*cb)());
LL_API void ll_ui_on_menu_exit(void (*cb)());
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

inline void onStartMenuOpen(void (*cb)()) {
    using Fn = void (*)(void (*)());
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_ui_on_start_menu_open"));
    if (fn) fn(cb);
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
