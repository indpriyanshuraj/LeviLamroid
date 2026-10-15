#pragma once
#include "ll/api/Macros.h"
#include <dlfcn.h>

extern "C" {
LL_API void ll_render_suppress_vanilla_background(bool suppress);
LL_API void ll_render_register_post_clear_callback(void (*cb)(int width, int height));
}

namespace ll::render {

inline void* getLoaderHandle() {
    static void* handle = nullptr;
    if (handle) return handle;
    handle = dlopen("liblevilamroid.so", RTLD_NOLOAD);
    if (!handle) handle = dlopen("libLeviLamroid.so", RTLD_NOLOAD);
    if (!handle) handle = RTLD_DEFAULT;
    return handle;
}

inline void suppressVanillaBackground(bool suppress) {
    using Fn = void (*)(bool);
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_render_suppress_vanilla_background"));
    if (fn) fn(suppress);
}

inline void registerPostClearCallback(void (*cb)(int width, int height)) {
    using Fn = void (*)(void (*)(int, int));
    static auto fn = reinterpret_cast<Fn>(dlsym(getLoaderHandle(), "ll_render_register_post_clear_callback"));
    if (fn) fn(cb);
}

} // namespace ll::render
