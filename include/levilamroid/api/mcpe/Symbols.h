#pragma once
#include "ll/api/Macros.h"
#include <string_view>
#include <cstdint>
#include <unordered_map>

namespace levilamroid::mcpe {

struct SymbolEntry {
    const char* humanName;
    const char* msvcSymbol;
    const char* itaniumSymbol;
    const char* idaSignature;
    uintptr_t rva1_26_52;
    uintptr_t rva1_26_45;
    size_t vtableSlot;
};

/// Lookup verified symbol metadata
LL_API const SymbolEntry* findSymbolInfo(std::string_view identifier);

/// Resolve effective target address by symbol or human readable name
LL_API void* resolveSymbolAddress(std::string_view identifier);

/// Get total number of registered universal symbols
LL_API size_t getRegisteredSymbolCount();

} // namespace levilamroid::mcpe
