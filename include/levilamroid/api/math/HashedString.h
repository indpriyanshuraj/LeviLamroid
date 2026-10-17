#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <cstddef>
#include "ll/api/Macros.h"

namespace levilamroid {

class HashedString {
public:
    static constexpr uint64_t FNV1A_OFFSET = 0xcbf29ce484222325ULL;
    static constexpr uint64_t FNV1A_PRIME  = 0x100000001b3ULL;

    static constexpr uint64_t computeHash(std::string_view str) noexcept {
        uint64_t hash = FNV1A_OFFSET;
        for (char c : str) {
            hash ^= static_cast<uint64_t>(static_cast<uint8_t>(c));
            hash *= FNV1A_PRIME;
        }
        return hash;
    }

private:
    uint64_t    mHash{0};
    std::string mStr;

public:
    HashedString() noexcept : mHash(computeHash("")), mStr("") {}

    HashedString(std::string_view str) 
        : mHash(computeHash(str)), mStr(str) {}

    HashedString(const char* str) 
        : mHash(computeHash(str ? str : "")), mStr(str ? str : "") {}

    HashedString(const std::string& str) 
        : mHash(computeHash(str)), mStr(str) {}

    HashedString(uint64_t hash, std::string_view str) 
        : mHash(hash), mStr(str) {}

    HashedString(const HashedString& other) = default;
    HashedString(HashedString&& other) noexcept = default;
    HashedString& operator=(const HashedString& other) = default;
    HashedString& operator=(HashedString&& other) noexcept = default;

    [[nodiscard]] uint64_t getHash() const noexcept { return mHash; }
    [[nodiscard]] const std::string& getString() const noexcept { return mStr; }
    [[nodiscard]] const char* c_str() const noexcept { return mStr.c_str(); }
    [[nodiscard]] bool empty() const noexcept { return mStr.empty(); }

    bool operator==(const HashedString& other) const noexcept {
        return mHash == other.mHash;
    }
    bool operator!=(const HashedString& other) const noexcept {
        return mHash != other.mHash;
    }
    bool operator<(const HashedString& other) const noexcept {
        return mHash < other.mHash;
    }
};

static_assert(sizeof(HashedString) == 32, "HashedString must be exactly 32 bytes on ARM64");

} // namespace levilamroid

namespace ll {
    using HashedString = ::levilamroid::HashedString;
}
