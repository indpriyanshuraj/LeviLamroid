#pragma once
#include <cstdint>
#include "mc/world/inventory/transaction/InventorySource.h"
#include "mc/world/item/ItemStack.h"

class InventoryAction {
public:
    InventorySource mSource{};
    uint32_t        mSlot{0};
    ItemStack       mFromItem{};
    ItemStack       mToItem{};

    InventoryAction() = default;

    InventoryAction(
        InventorySource const& source,
        uint32_t slot,
        ItemStack const& fromItem,
        ItemStack const& toItem
    )
        : mSource(source),
          mSlot(slot),
          mFromItem(fromItem),
          mToItem(toItem) {}

    [[nodiscard]] InventorySource const& getSource() const noexcept { return mSource; }
    [[nodiscard]] uint32_t getSlot() const noexcept { return mSlot; }
    [[nodiscard]] ItemStack const& getFromItem() const noexcept { return mFromItem; }
    [[nodiscard]] ItemStack const& getToItem() const noexcept { return mToItem; }
};
