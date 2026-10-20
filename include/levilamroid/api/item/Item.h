#pragma once
#include "levilamroid/api/math/HashedString.h"
#include "levilamroid/api/item/CreativeItemCategory.h"
#include "levilamroid/api/item/CreativeGroup.h"
#include "levilamroid/api/item/IconInfo.h"
#include "ll/api/Macros.h"
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <cstring>

namespace levilamroid::item {

class LL_API Item {
public:
    // Virtual destructor / vtable pointer at +0x00
    virtual ~Item() = default;

    // Verified 1.26.52.3 ARM64 Field Accessors
    [[nodiscard]] inline short getId() const noexcept {
        return *reinterpret_cast<const short*>(reinterpret_cast<const uint8_t*>(this) + 0x8A);
    }

    [[nodiscard]] inline const std::string& getRawNameId() const noexcept {
        return *reinterpret_cast<const std::string*>(reinterpret_cast<const uint8_t*>(this) + 0xD0);
    }

    [[nodiscard]] inline uint8_t getMaxStackSize() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x88);
    }

    inline void setMaxStackSize(uint8_t size) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x88) = size;
    }

    [[nodiscard]] inline CreativeItemCategory getCreativeCategory() const noexcept {
        return static_cast<CreativeItemCategory>(*reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x140));
    }

    inline void setCreativeCategory(CreativeItemCategory category) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x140) = static_cast<uint8_t>(category);
    }

    [[nodiscard]] inline const HashedString& getCreativeGroup() const noexcept {
        return *reinterpret_cast<const HashedString*>(reinterpret_cast<const uint8_t*>(this) + 0x148);
    }

    inline void setCreativeGroup(const HashedString& group) noexcept {
        *reinterpret_cast<HashedString*>(reinterpret_cast<uint8_t*>(this) + 0x148) = group;
    }

    inline void setCreativeGroup(std::string_view group) noexcept {
        *reinterpret_cast<HashedString*>(reinterpret_cast<uint8_t*>(this) + 0x148) = HashedString(group);
    }

    inline void setCreativeGroup(const char* group) noexcept {
        *reinterpret_cast<HashedString*>(reinterpret_cast<uint8_t*>(this) + 0x148) = HashedString(group ? group : "");
    }

    inline void setCreativeGroup(CreativeGroup group) noexcept {
        setCreativeGroup(getGroupName(group));
    }

    [[nodiscard]] inline bool isOffhandAllowed() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x1C8) != 0;
    }

    inline void setAllowOffhand(bool allow) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x1C8) = allow ? 1 : 0;
    }

    [[nodiscard]] inline int getIconFrame() const noexcept {
        return *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(this) + 0x58);
    }

    inline void setIconFrame(int frame) noexcept {
        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(this) + 0x58) = frame;
    }

    [[nodiscard]] inline bool isHandEquipped() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x1CA) != 0;
    }

    inline void setHandEquipped(bool handEquipped = true) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x1CA) = handEquipped ? 1 : 0;
    }

    [[nodiscard]] inline bool isMirroredArt() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x1CB) != 0;
    }

    inline void setIsMirroredArt(bool mirrored = true) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x1CB) = mirrored ? 1 : 0;
    }

    [[nodiscard]] inline short getMaxDamage() const noexcept {
        return *reinterpret_cast<const short*>(reinterpret_cast<const uint8_t*>(this) + 0x1D0);
    }

    inline void setMaxDamage(short damage) noexcept {
        *reinterpret_cast<short*>(reinterpret_cast<uint8_t*>(this) + 0x1D0) = damage;
    }

    // ── Phase 1: Food / Tool / Weapon Accessors (1.26.52.3 ARM64) ──

    // Item::mUseAnimation at +0x1C4 (UseAnimation enum: 0=none, 1=eat, 2=bow, 3=spear, 4=crossbow, 5=spyglass)
    [[nodiscard]] inline uint8_t getUseAnimation() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x1C4);
    }
    inline void setUseAnimation(uint8_t anim) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x1C4) = anim;
    }

    // Item::mUseDuration at +0x1CC (in ticks: 32 = food, 72000 = bow)
    [[nodiscard]] inline int getUseDuration() const noexcept {
        return *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(this) + 0x1CC);
    }
    inline void setUseDuration(int ticks) noexcept {
        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(this) + 0x1CC) = ticks;
    }

    // Item::mAttackDamage at +0x1D4 (float, bonus melee damage)
    [[nodiscard]] inline float getAttackDamage() const noexcept {
        return *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(this) + 0x1D4);
    }
    inline void setAttackDamage(float dmg) noexcept {
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(this) + 0x1D4) = dmg;
    }

    // Item::mNutrition at +0x1E0 (int, hunger points restored; 0 = not food)
    [[nodiscard]] inline int getNutrition() const noexcept {
        return *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(this) + 0x1E0);
    }
    inline void setNutrition(int value) noexcept {
        *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(this) + 0x1E0) = value;
    }

    // Item::mSaturationModifier at +0x1E4 (float, saturation ratio)
    [[nodiscard]] inline float getSaturationModifier() const noexcept {
        return *reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(this) + 0x1E4);
    }
    inline void setSaturationModifier(float value) noexcept {
        *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(this) + 0x1E4) = value;
    }

    // ── End Phase 1 ──

    // Engine Virtual setIconInfo (RVA 0x0FF7C808 in 1.26.52.3)
    Item& setIconInfo(const std::string& name, int frame = 0);
};

} // namespace levilamroid::item

namespace ll::item {
    using Item = ::levilamroid::item::Item;
}

namespace ll {
    using Item = ::levilamroid::item::Item;
}
