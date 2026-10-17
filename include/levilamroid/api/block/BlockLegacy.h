#pragma once
#include "levilamroid/api/math/HashedString.h"
#include "levilamroid/api/item/CreativeItemCategory.h"
#include <string>
#include <cstdint>

namespace levilamroid::block {

class Block;

class BlockLegacy {
public:
    virtual ~BlockLegacy() = default;

    /// Get raw identifier without namespace (e.g. "stone", "astral_block")
    [[nodiscard]] inline const HashedString& getRawNameId() const noexcept {
        return *reinterpret_cast<const HashedString*>(reinterpret_cast<const uint8_t*>(this) + 0x08);
    }

    /// Get complete identifier (e.g. "minecraft:stone", "levitest:astral_block")
    [[nodiscard]] inline const HashedString& getFullName() const noexcept {
        return *reinterpret_cast<const HashedString*>(reinterpret_cast<const uint8_t*>(this) + 0x28);
    }

    /// Get description/translation identifier (e.g. "tile.stone.name")
    [[nodiscard]] inline std::string getDescriptionId() const {
        return getFullName().getString();
    }

    /// Get creative inventory category
    [[nodiscard]] inline item::CreativeItemCategory getCreativeCategory() const noexcept {
        return static_cast<item::CreativeItemCategory>(
            *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x48)
        );
    }

    /// Set creative inventory category
    inline void setCreativeCategory(item::CreativeItemCategory category) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x48) = static_cast<uint8_t>(category);
    }

    /// Get creative item group
    [[nodiscard]] inline const HashedString& getCreativeGroup() const noexcept {
        return *reinterpret_cast<const HashedString*>(reinterpret_cast<const uint8_t*>(this) + 0x50);
    }

    /// Set creative item group
    inline void setCreativeGroup(const HashedString& group) noexcept {
        *reinterpret_cast<HashedString*>(reinterpret_cast<uint8_t*>(this) + 0x50) = group;
    }

    /// Blast resistance against explosions
    [[nodiscard]] inline float getExplosionResistance() const noexcept {
        return *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(this) + 0x74);
    }

    inline void setExplosionResistance(float val) noexcept {
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(this) + 0x74) = val;
    }

    /// Surface friction multiplier (0.6 default, 0.98 ice)
    [[nodiscard]] inline float getFriction() const noexcept {
        return *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(this) + 0x78);
    }

    inline void setFriction(float val) noexcept {
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(this) + 0x78) = val;
    }

    /// Emitted light level [0..15]
    [[nodiscard]] inline uint8_t getLightEmission() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x88);
    }

    inline void setLightEmission(uint8_t light) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x88) = light;
    }

    /// Hardness / Destroy speed in seconds
    [[nodiscard]] inline float getDestroySpeed() const noexcept {
        return *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(this) + 0x7C);
    }

    inline void setDestroyTime(float seconds) noexcept {
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(this) + 0x7C) = seconds;
    }

    /// Whether the block is solid
    [[nodiscard]] inline bool isSolid() const noexcept {
        return (*reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x8A) & 0x01) != 0;
    }

    inline void setSolid(bool solid) noexcept {
        uint8_t* flag = reinterpret_cast<uint8_t*>(this) + 0x8A;
        if (solid) *flag |= 0x01; else *flag &= ~0x01;
    }
};

} // namespace levilamroid::block

namespace ll::block {
    using BlockLegacy = ::levilamroid::block::BlockLegacy;
}
