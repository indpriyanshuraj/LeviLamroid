#pragma once
#include "levilamroid/api/item/Item.h"
#include "ll/api/Macros.h"
#include <cstdint>
#include <string>

namespace levilamroid::item {

struct BedrockControlBlock {
    Item*    mItem{nullptr};     // +0x00
    uint32_t mWeakRef{1};        // +0x08
    uint32_t mStrongRef{1};      // +0x0C
};

class LL_API ItemStack {
public:
    virtual ~ItemStack() = default;

    [[nodiscard]] inline BedrockControlBlock* getControlBlock() const noexcept {
        return *reinterpret_cast<BedrockControlBlock* const*>(reinterpret_cast<const uint8_t*>(this) + 0x08);
    }

    [[nodiscard]] inline Item* getItem() const noexcept {
        auto* ctrl = getControlBlock();
        return ctrl ? ctrl->mItem : nullptr;
    }

    [[nodiscard]] inline short getId() const noexcept {
        auto* item = getItem();
        return item ? item->getId() : 0;
    }

    [[nodiscard]] inline uint16_t getAuxValue() const noexcept {
        return *reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(this) + 0x20);
    }

    inline void setAuxValue(uint16_t aux) noexcept {
        *reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(this) + 0x20) = aux;
    }

    [[nodiscard]] inline uint8_t getCount() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x22);
    }

    inline void setCount(uint8_t count) noexcept {
        *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x22) = count;
        if (count > 0) {
            *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(this) + 0x23) = 1; // mValid = true
        }
    }

    [[nodiscard]] inline bool isValid() const noexcept {
        return *reinterpret_cast<const uint8_t*>(reinterpret_cast<const uint8_t*>(this) + 0x23) != 0 && getCount() > 0;
    }

    [[nodiscard]] inline bool isNull() const noexcept {
        return !isValid() || getItem() == nullptr;
    }

    [[nodiscard]] inline void* getUserData() const noexcept {
        return *reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(this) + 0x10);
    }
};

using ItemInstance = ItemStack;

} // namespace levilamroid::item

namespace ll::item {
    using ItemStack = ::levilamroid::item::ItemStack;
    using ItemInstance = ::levilamroid::item::ItemInstance;
    using BedrockControlBlock = ::levilamroid::item::BedrockControlBlock;
}

namespace ll {
    using ItemStack = ::levilamroid::item::ItemStack;
    using ItemInstance = ::levilamroid::item::ItemInstance;
}
