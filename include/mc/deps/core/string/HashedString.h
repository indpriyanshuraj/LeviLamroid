#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

class HashedString {
public:
    uint64_t    mStrHash{0};
    std::string mStr{};

    constexpr HashedString() noexcept = default;

    HashedString(std::string const& str)
        : mStrHash(computeHash(str)), mStr(str) {}

    HashedString(std::string&& str) noexcept
        : mStrHash(computeHash(str)), mStr(std::move(str)) {}

    HashedString(char const* str)
        : mStrHash(str ? computeHash(str) : 0), mStr(str ? str : "") {}

    HashedString(std::string_view sv)
        : mStrHash(computeHash(sv)), mStr(sv) {}

    HashedString(uint64_t hash, char const* str)
        : mStrHash(hash), mStr(str ? str : "") {}

    HashedString(uint64_t hash, std::string const& str)
        : mStrHash(hash), mStr(str) {}

    [[nodiscard]] std::string const& getString() const noexcept { return mStr; }
    [[nodiscard]] char const* c_str() const noexcept { return mStr.c_str(); }
    [[nodiscard]] uint64_t getHash() const noexcept { return mStrHash; }
    [[nodiscard]] bool isEmpty() const noexcept { return mStr.empty(); }
    [[nodiscard]] bool empty() const noexcept { return mStr.empty(); }

    void clear() noexcept {
        mStr.clear();
        mStrHash = 0;
    }

    [[nodiscard]] bool operator==(HashedString const& rhs) const noexcept {
        return mStrHash == rhs.mStrHash && mStr == rhs.mStr;
    }

    [[nodiscard]] bool operator!=(HashedString const& rhs) const noexcept {
        return !(*this == rhs);
    }

    [[nodiscard]] bool operator<(HashedString const& rhs) const noexcept {
        if (mStrHash < rhs.mStrHash) return true;
        if (mStrHash > rhs.mStrHash) return false;
        return mStr < rhs.mStr;
    }

    [[nodiscard]] explicit operator std::string_view() const noexcept {
        return mStr;
    }

    static constexpr uint64_t computeHash(std::string_view sv) noexcept {
        // 64-bit FNV-1a hash algorithm matching Bedrock Engine
        uint64_t hash = 0xCBF29CE484222325ULL;
        for (char c : sv) {
            hash ^= static_cast<uint64_t>(static_cast<uint8_t>(c));
            hash *= 0x100000001B3ULL;
        }
        return hash;
    }

    static HashedString const& getEmpty() noexcept {
        static HashedString const empty{};
        return empty;
    }
};

namespace std {
template <>
struct hash<HashedString> {
    size_t operator()(HashedString const& hs) const noexcept {
        return static_cast<size_t>(hs.getHash());
    }
};
} // namespace std
