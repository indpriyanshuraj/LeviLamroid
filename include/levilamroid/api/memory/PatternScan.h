#pragma once
#include "ll/api/Macros.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace ll::memory {

/// Scan for byte pattern with IDA-style string (e.g. "48 89 5C 24 ? 57 48 83 EC 20")
LL_API uintptr_t scanPattern(const std::string& pattern, uintptr_t start = 0, size_t length = 0);

/// Scan within a specific module or library (.text section)
LL_API uintptr_t scanModule(const std::string& moduleName, const std::string& pattern);

/// Scan within libminecraftpe.so
LL_API uintptr_t scanMinecraft(const std::string& pattern);

} // namespace ll::memory
