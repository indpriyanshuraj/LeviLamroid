#pragma once
#include "ll/api/mod/NativeMod.h"

#define LL_REGISTER_MOD(ModClass, instance) \
    extern "C" [[gnu::visibility("default")]] bool mod_load() { \
        return (instance).load(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool mod_enable() { \
        return (instance).enable(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool mod_disable() { \
        return (instance).disable(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool mod_unload() { \
        return true; \
    } \
    extern "C" [[gnu::visibility("default")]] bool ll_mod_load(void*) { \
        return (instance).load(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool ll_mod_enable(void*) { \
        return (instance).enable(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool ll_mod_disable(void*) { \
        return (instance).disable(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool ll_mod_unload(void*) { \
        return true; \
    } \
    extern "C" [[gnu::visibility("default")]] bool PLMod_Load(void*) { \
        return (instance).load(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool PLMod_Enable(void*) { \
        return (instance).enable(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool PLMod_Disable(void*) { \
        return (instance).disable(); \
    } \
    extern "C" [[gnu::visibility("default")]] bool PLMod_Unload(void*) { \
        return true; \
    } \
    extern "C" [[gnu::visibility("default")]] bool LeviMod_Load(void*) { \
        return (instance).load(); \
    } \
    extern "C" [[gnu::visibility("default")]] const void* PLGetModRegistration() { \
        static struct { \
            void* inst; \
            bool (*load)(void*, void*); \
            bool (*enable)(void*, void*); \
            bool (*disable)(void*, void*); \
            bool (*unload)(void*, void*); \
        } sReg = { \
            &(instance), \
            [](void* inst, void*) -> bool { \
                return static_cast<ModClass*>(inst)->load(); \
            }, \
            [](void* inst, void*) -> bool { \
                return static_cast<ModClass*>(inst)->enable(); \
            }, \
            [](void* inst, void*) -> bool { \
                return static_cast<ModClass*>(inst)->disable(); \
            }, \
            [](void*, void*) -> bool { \
                return true; \
            } \
        }; \
        return &sReg; \
    }


