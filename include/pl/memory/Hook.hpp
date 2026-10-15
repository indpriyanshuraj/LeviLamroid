#pragma once
#include <string_view>
#include <cstddef>
#include "ll/api/memory/Hook.h"

namespace pl::memory {

using HookPriority = ::ll::memory::HookPriority;

inline bool hook(void* target, void* detour, void** original, HookPriority priority = HookPriority::Normal) {
    (void)priority;
    return ::ll::memory::hook(target, detour, original);
}

inline bool unhook(void* target, void* detour) {
    (void)target;
    (void)detour;
    return true;
}

void* resolveVtableFunction(std::string_view className, size_t slot, std::string_view moduleName = "libminecraftpe.so");

} // namespace pl::memory
