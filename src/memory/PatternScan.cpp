#include "levilamroid/api/memory/PatternScan.h"
#include "levilamroid/api/memory/Hook.h"
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>

namespace ll::memory {

static std::vector<int> parsePattern(const std::string& pattern) {
    std::vector<int> bytes;
    std::istringstream stream(pattern);
    std::string byteStr;

    while (stream >> byteStr) {
        if (byteStr == "?" || byteStr == "??") {
            bytes.push_back(-1); // Wildcard
        } else {
            bytes.push_back(std::stoi(byteStr, nullptr, 16));
        }
    }
    return bytes;
}

uintptr_t scanPattern(const std::string& patternStr, uintptr_t start, size_t length) {
    if (start == 0 || length == 0) return 0;
    auto pattern = parsePattern(patternStr);
    if (pattern.empty()) return 0;

    const uint8_t* memory = reinterpret_cast<const uint8_t*>(start);
    size_t patternSize = pattern.size();

    for (size_t i = 0; i <= length - patternSize; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (pattern[j] != -1 && memory[i + j] != static_cast<uint8_t>(pattern[j])) {
                found = false;
                break;
            }
        }
        if (found) {
            return start + i;
        }
    }
    return 0;
}

uintptr_t scanMinecraft(const std::string& pattern) {
    uintptr_t base = getMinecraftBase();
    if (!base) return 0;
    // Scan executable text section (~150MB)
    return scanPattern(pattern, base, 0x0A000000);
}

} // namespace ll::memory
