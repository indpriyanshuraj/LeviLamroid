#pragma once
#include <cstdint>
#include <vector>
#include "mc/world/item/ItemStack.h"

class Inventory {
public:
    virtual ~Inventory() = default;

    [[nodiscard]] virtual ItemStack const& getItem(int32_t slot) const;
    [[nodiscard]] virtual ItemStack& getItem(int32_t slot);
    virtual void setItem(int32_t slot, ItemStack const& item);
    [[nodiscard]] virtual int32_t getContainerSize() const noexcept;
};
