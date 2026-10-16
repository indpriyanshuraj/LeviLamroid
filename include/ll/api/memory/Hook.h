#pragma once
#include "ll/api/Macros.h"
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <string>
#include <string_view>
#include <fcntl.h>
#include <unistd.h>
#include <utility>

namespace bedrock_parity::core {

class SafeMemory {
private:
    static inline int sDevNullFd = -1;

public:
    static void init() {
        if (sDevNullFd < 0) {
            sDevNullFd = open("/dev/null", O_WRONLY);
        }
    }

    [[nodiscard]] static bool isReadable(const void* ptr, size_t size) {
        if (!ptr || size == 0) return false;
        init();
        if (sDevNullFd < 0) return false;
        return write(sDevNullFd, ptr, size) == static_cast<ssize_t>(size);
    }
};

} // namespace bedrock_parity::core

namespace ll::memory {

using SafeMemory = ::bedrock_parity::core::SafeMemory;

enum class HookPriority : int {
    Lowest = -2,
    Low = -1,
    Normal = 0,
    High = 1,
    Highest = 2
};

/// Get the discovered virtual base address of libminecraftpe.so in the current process
LL_API uintptr_t getMinecraftBase();

/// Set or override the base address manually if needed
LL_API void setMinecraftBase(uintptr_t base);

/// Low-level function detour hook
LL_API bool hook(void* target, void* detour, void** original);

/// Hook a function by its relative virtual address (RVA) from libminecraftpe.so base
LL_API bool hookRva(uintptr_t rva, void* detour, void** original);

/// Hook a function via Global Offset Table (GOT) relocation slot
LL_API bool hookGotSlot(uintptr_t gotRva, void* detour, void** original);

/// Hook a function by an IDA-style byte pattern signature, with optional fast-path fallback RVA
LL_API bool hookSignature(const std::string& pattern, void* detour, void** original, uintptr_t fallbackRva = 0);

/// Resolve a symbol dynamically from libminecraftpe.so or loaded libraries
LL_API void* findMinecraftSymbol(const char* symbolName);

/// Resolve virtual method from class VTable
LL_API void* resolveVtableFunction(const char* className, size_t slot, const char* moduleName = "libminecraftpe.so");

/// Typed helper for hooking functions
template <typename Fn>
inline bool hook(Fn target, Fn detour, Fn* original) {
    return hook(reinterpret_cast<void*>(target),
                reinterpret_cast<void*>(detour),
                reinterpret_cast<void**>(original));
}

/// Typed helper for hooking by RVA
template <typename Fn>
inline bool hookRva(uintptr_t rva, Fn detour, Fn* original) {
    return hookRva(rva,
                   reinterpret_cast<void*>(detour),
                   reinterpret_cast<void**>(original));
}

namespace internal {

LL_API bool registerHookHelper(const char* name, const char* symbolOrPattern, void* detour, void** original);

inline bool registerHookHelper(const char* name, std::string_view symbol, void* detour, void** original) {
    return registerHookHelper(name, symbol.data(), detour, original);
}

LL_API bool registerHookHelper(const char* name, uintptr_t rva, void* detour, void** original);

LL_API bool registerMemberHook(const char* name, const char* className, void* memberPtr, void* detour, void** original);

template <typename Class, typename Ret, typename... Args>
inline bool registerInstanceHookHelper(const char* name, const char* className, Ret (Class::*method)(Args...), void* detour, void** original) {
    return registerMemberHook(name, className, reinterpret_cast<void*&>(method), detour, original);
}

template <typename Class, typename Ret, typename... Args>
inline bool registerInstanceHookHelper(const char* name, const char* className, Ret (Class::*method)(Args...) const, void* detour, void** original) {
    return registerMemberHook(name, className, reinterpret_cast<void*&>(method), detour, original);
}

inline bool registerInstanceHookHelper(const char* name, const char* /*className*/, const char* symbolOrRva, void* detour, void** original) {
    return registerHookHelper(name, symbolOrRva, detour, original);
}

template <typename Ret, typename... Args>
inline bool registerHookHelper(const char* /*name*/, Ret (*func)(Args...), void* detour, void** original) {
    return hook(reinterpret_cast<void*>(func), detour, original);
}

} // namespace internal

} // namespace ll::memory

// ----------------------------------------------------------------------------
// LeviLamina Windows Parity Hook Macros
// ----------------------------------------------------------------------------

#define LL_TYPE_INSTANCE_HOOK(HookName, Priority, TargetClass, TargetFunc, ReturnType, ...) \
    struct HookName : public TargetClass { \
        using FnType = ReturnType (*)(TargetClass*, ##__VA_ARGS__); \
        static inline void* sOriginal = nullptr; \
        ReturnType _detour(__VA_ARGS__); \
        template <typename R = ReturnType, typename... CallArgs> \
        static R callOrigin(TargetClass* self, CallArgs&&... args) { \
            auto fn = reinterpret_cast<R (*)(TargetClass*, CallArgs...)>(sOriginal); \
            if constexpr (std::is_void_v<R>) { \
                if (fn) fn(self, std::forward<CallArgs>(args)...); \
                return; \
            } else if constexpr (std::is_reference_v<R>) { \
                using ValueType = std::remove_cvref_t<R>; \
                if (fn) return fn(self, std::forward<CallArgs>(args)...); \
                static ValueType sFallback{}; \
                return sFallback; \
            } else { \
                if (fn) return fn(self, std::forward<CallArgs>(args)...); \
                return R(); \
            } \
        } \
        template <typename... CallArgs> \
        ReturnType origin(CallArgs&&... args) { \
            return callOrigin<ReturnType>(static_cast<TargetClass*>(this), std::forward<CallArgs>(args)...); \
        } \
        struct AutoRegister { \
            AutoRegister() { \
                union { \
                    ReturnType (HookName::*mf)(__VA_ARGS__); \
                    void* ptr; \
                } u; \
                u.mf = &HookName::_detour; \
                ::ll::memory::internal::registerInstanceHookHelper( \
                    #HookName, #TargetClass, TargetFunc, u.ptr, &sOriginal \
                ); \
            } \
        }; \
        static inline AutoRegister sAutoReg; \
        static int hook() { return sOriginal != nullptr ? 0 : -1; } \
        static bool unhook() { return true; } \
    }; \
    ReturnType HookName::_detour(__VA_ARGS__)

#define LL_AUTO_INSTANCE_HOOK(HookName, Priority, TargetClass, TargetIdentifier, ReturnType, ...) \
    LL_TYPE_INSTANCE_HOOK(HookName, Priority, TargetClass, TargetIdentifier, ReturnType, ##__VA_ARGS__)

#define LL_TYPE_STATIC_HOOK(HookName, Priority, TargetFunc, ReturnType, ...) \
    struct HookName { \
        using FnType = ReturnType (*)(__VA_ARGS__); \
        static inline FnType sOriginal = nullptr; \
        static ReturnType detour(__VA_ARGS__); \
        template <typename... Args> \
        static ReturnType origin(Args&&... args) { \
            if constexpr (std::is_void_v<ReturnType>) { \
                if (sOriginal) sOriginal(std::forward<Args>(args)...); \
                return; \
            } else if constexpr (std::is_reference_v<ReturnType>) { \
                using ValueType = std::remove_cvref_t<ReturnType>; \
                if (sOriginal) return sOriginal(std::forward<Args>(args)...); \
                static ValueType sFallback{}; \
                return sFallback; \
            } else { \
                if (sOriginal) return sOriginal(std::forward<Args>(args)...); \
                return ReturnType(); \
            } \
        } \
        template <typename... Args> \
        static ReturnType callOrigin(Args&&... args) { \
            return origin(std::forward<Args>(args)...); \
        } \
        struct AutoRegister { \
            AutoRegister() { \
                ::ll::memory::internal::registerHookHelper( \
                    #HookName, TargetFunc, reinterpret_cast<void*>(&detour), reinterpret_cast<void**>(&sOriginal) \
                ); \
            } \
        }; \
        static inline AutoRegister sAutoReg; \
        static int hook() { return 0; } \
        static bool unhook() { return true; } \
    }; \
    ReturnType HookName::detour(__VA_ARGS__)

#define LL_AUTO_STATIC_HOOK(HookName, Priority, TargetIdentifier, ReturnType, ...) \
    LL_TYPE_STATIC_HOOK(HookName, Priority, TargetIdentifier, ReturnType, ##__VA_ARGS__)

// Amethyst / Classic THOOK macro parity
#define THOOK(TargetFunc, ReturnType, ...) \
    LL_TYPE_STATIC_HOOK(Hook_##TargetFunc, ::ll::memory::HookPriority::Normal, #TargetFunc, ReturnType, ##__VA_ARGS__)
