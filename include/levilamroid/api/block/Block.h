#pragma once
#include "levilamroid/api/block/BlockLegacy.h"
#include "levilamroid/api/math/HashedString.h"
#include <string>
#include <cstdint>

namespace levilamroid::block {

class Block {
public:
    virtual ~Block() = default;

    /// Get underlying legacy block definition
    [[nodiscard]] inline const BlockLegacy* getLegacyBlock() const noexcept {
        return *reinterpret_cast<const BlockLegacy* const*>(reinterpret_cast<const uint8_t*>(this) + 0x08);
    }

    /// Get network runtime ID used in chunk packet serialization
    [[nodiscard]] inline uint32_t getRuntimeId() const noexcept {
        return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(this) + 0x24);
    }

    /// Get network identifier
    [[nodiscard]] inline uint32_t getNetworkId() const noexcept {
        return getRuntimeId();
    }

    /// Get full block name string (e.g. "minecraft:stone", "levitest:astral_block")
    [[nodiscard]] inline const std::string& getTypeName() const noexcept {
        const auto* legacy = getLegacyBlock();
        if (legacy) {
            return legacy->getFullName().getString();
        }
        static const std::string sEmpty;
        return sEmpty;
    }

    /// Get 64-bit hashed string name
    [[nodiscard]] inline const HashedString& getHashedTypeName() const noexcept {
        const auto* legacy = getLegacyBlock();
        if (legacy) {
            return legacy->getFullName();
        }
        static const HashedString sEmpty("");
        return sEmpty;
    }

    /// Whether this block permutation is air
    [[nodiscard]] inline bool isAir() const noexcept {
        return getRuntimeId() == 0 || getTypeName() == "minecraft:air";
    }

    /// Whether this block permutation is solid
    [[nodiscard]] inline bool isSolid() const noexcept {
        const auto* legacy = getLegacyBlock();
        return legacy ? legacy->isSolid() : false;
    }
};

} // namespace levilamroid::block

namespace ll::block {
    using Block = ::levilamroid::block::Block;
}
