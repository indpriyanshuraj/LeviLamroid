#include "levilamroid/api/memory/PatternScan.h"
#include "levilamroid/api/memory/Hook.h"
#include "ll/api/Logger.h"
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <unordered_map>
#include <mutex>

#include <fcntl.h>

namespace ll::memory {

static bool isMemoryReadable(const void* ptr, size_t size) {
    if (!ptr || size == 0) return false;
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t untagged = v & 0x00ffffffffffffffULL;
    if (untagged < 0x10000 || untagged > 0x00007fffffffffffULL) return false;
    if (untagged + size < untagged) return false;
    return true;
}

std::vector<int> parsePattern(const std::string& pattern) {
    std::vector<int> bytes;
    if (pattern.empty()) return bytes;

    bool hasSpaces = (pattern.find(' ') != std::string::npos);
    if (hasSpaces) {
        std::istringstream stream(pattern);
        std::string byteStr;
        while (stream >> byteStr) {
            if (byteStr == "?" || byteStr == "??") {
                bytes.push_back(-1);
            } else {
                try {
                    int val = std::stoi(byteStr, nullptr, 16);
                    bytes.push_back(val & 0xFF);
                } catch (...) {
                    bytes.push_back(-1);
                }
            }
        }
    } else {
        size_t i = 0;
        while (i < pattern.size()) {
            if (std::isspace(static_cast<unsigned char>(pattern[i]))) {
                i++;
                continue;
            }
            if (pattern[i] == '?') {
                bytes.push_back(-1);
                i++;
                if (i < pattern.size() && pattern[i] == '?') i++;
            } else if (i + 1 < pattern.size()) {
                std::string hexByte = pattern.substr(i, 2);
                try {
                    int val = std::stoi(hexByte, nullptr, 16);
                    bytes.push_back(val & 0xFF);
                } catch (...) {
                    bytes.push_back(-1);
                }
                i += 2;
            } else {
                break;
            }
        }
    }
    return bytes;
}

bool verifyPattern(uintptr_t addr, const std::string& patternStr) {
    if (!addr || patternStr.empty()) return false;
    auto pattern = parsePattern(patternStr);
    if (pattern.empty()) return false;

    if (!isMemoryReadable(reinterpret_cast<const void*>(addr), pattern.size())) {
        return false;
    }

    const uint8_t* memory = reinterpret_cast<const uint8_t*>(addr);
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] != -1 && memory[i] != static_cast<uint8_t>(pattern[i])) {
            return false;
        }
    }
    return true;
}

uintptr_t scanPattern(const std::string& patternStr, uintptr_t start, size_t length) {
    if (start == 0 || length == 0) return 0;
    auto pattern = parsePattern(patternStr);
    if (pattern.empty()) return 0;

    const uint8_t* memory = reinterpret_cast<const uint8_t*>(start);
    size_t patternSize = pattern.size();
    if (length < patternSize) return 0;

    // Find first non-wildcard byte for memchr acceleration
    size_t firstNonWildcardIdx = 0;
    while (firstNonWildcardIdx < patternSize && pattern[firstNonWildcardIdx] == -1) {
        firstNonWildcardIdx++;
    }
    if (firstNonWildcardIdx >= patternSize) return start;

    uint8_t firstByte = static_cast<uint8_t>(pattern[firstNonWildcardIdx]);
    const uint8_t* searchPtr = memory + firstNonWildcardIdx;
    size_t remainingLen = length - patternSize;

    while (remainingLen > 0) {
        const uint8_t* match = reinterpret_cast<const uint8_t*>(std::memchr(searchPtr, firstByte, remainingLen));
        if (!match) break;

        const uint8_t* candidateStart = match - firstNonWildcardIdx;
        bool found = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (pattern[j] != -1 && candidateStart[j] != static_cast<uint8_t>(pattern[j])) {
                found = false;
                break;
            }
        }
        if (found) {
            return reinterpret_cast<uintptr_t>(candidateStart);
        }

        size_t consumed = (match + 1) - searchPtr;
        if (consumed >= remainingLen) break;
        remainingLen -= consumed;
        searchPtr = match + 1;
    }
    return 0;
}

static std::unordered_map<std::string, uintptr_t> sScanCache;
static std::mutex sScanMutex;

uintptr_t scanMinecraft(const std::string& pattern) {
    {
        std::lock_guard<std::mutex> lock(sScanMutex);
        auto it = sScanCache.find(pattern);
        if (it != sScanCache.end()) return it->second;
    }

    uintptr_t base = getMinecraftBase();
    if (!base) return 0;
    // Scan text segments of libminecraftpe.so (~328MB on Android ARM64)
    uintptr_t res = scanPattern(pattern, base, 0x15000000);
    {
        std::lock_guard<std::mutex> lock(sScanMutex);
        sScanCache[pattern] = res;
    }
    return res;
}

static std::unordered_map<std::string, uintptr_t> sTextScanCache;
static std::mutex sTextScanMutex;

uintptr_t scanMinecraftText(const std::string& pattern) {
    {
        std::lock_guard<std::mutex> lock(sTextScanMutex);
        auto it = sTextScanCache.find(pattern);
        if (it != sTextScanCache.end()) return it->second;
    }

    uintptr_t base = getMinecraftBase();
    if (!base) return 0;
    // Android ARM64 1.26.52.3 .text boundaries: offset 0x064C64E0, size 0x0C44AFCC (~205.8MB)
    uintptr_t textStart = base + 0x064C64E0;
    size_t textSize = 0x0C44AFCC;
    uintptr_t res = scanPattern(pattern, textStart, textSize);
    {
        std::lock_guard<std::mutex> lock(sTextScanMutex);
        sTextScanCache[pattern] = res;
    }
    return res;
}

static std::unordered_map<std::string, uintptr_t> sResolvedCache;
static std::mutex sCacheMutex;

uintptr_t SignatureEngine::resolve(std::string_view name, uintptr_t fallbackRva, const std::string& pattern) {
    std::string key(name);
    {
        std::lock_guard<std::mutex> lock(sCacheMutex);
        auto it = sResolvedCache.find(key);
        if (it != sResolvedCache.end()) {
            return it->second;
        }
    }

    uintptr_t base = getMinecraftBase();
    if (!base) return 0;

    // 1. Fast-path: Check fallback RVA with pattern verification
    if (fallbackRva != 0) {
        uintptr_t targetAddr = base + fallbackRva;
        if (pattern.empty() || verifyPattern(targetAddr, pattern)) {
            std::lock_guard<std::mutex> lock(sCacheMutex);
            sResolvedCache[key] = targetAddr;
            return targetAddr;
        }
    }

    // 2. Localized window scan around fallback RVA (+/- 64KB)
    if (fallbackRva != 0 && !pattern.empty()) {
        uintptr_t winStart = (fallbackRva > 0x10000) ? (base + fallbackRva - 0x10000) : base;
        uintptr_t foundAddr = scanPattern(pattern, winStart, 0x20000);
        if (foundAddr != 0) {
            std::lock_guard<std::mutex> lock(sCacheMutex);
            sResolvedCache[key] = foundAddr;
            return foundAddr;
        }

        // Fast fallback to targetAddr: If bounded window scan failed, trust fallbackRva
        // rather than freezing the main thread for 15 seconds scanning 540MB linearly!
        uintptr_t targetAddr = base + fallbackRva;
        std::lock_guard<std::mutex> lock(sCacheMutex);
        sResolvedCache[key] = targetAddr;
        return targetAddr;
    }

    // 3. Fast .text section scan (205 MB) only when no fallback RVA is provided
    if (!pattern.empty()) {
        uintptr_t foundAddr = scanMinecraftText(pattern);
        if (foundAddr != 0) {
            std::lock_guard<std::mutex> lock(sCacheMutex);
            sResolvedCache[key] = foundAddr;
            return foundAddr;
        }
    }

    // 4. Fallback address if pattern scan failed but RVA was provided
    if (fallbackRva != 0) {
        uintptr_t targetAddr = base + fallbackRva;
        std::lock_guard<std::mutex> lock(sCacheMutex);
        sResolvedCache[key] = targetAddr;
        return targetAddr;
    }

    std::lock_guard<std::mutex> lock(sCacheMutex);
    sResolvedCache[key] = 0;
    return 0;
}


void SignatureEngine::clearCache() {
    std::lock_guard<std::mutex> lock(sCacheMutex);
    sResolvedCache.clear();
}

} // namespace ll::memory

