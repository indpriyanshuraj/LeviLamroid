#pragma once
#include <cstdint>
#include <memory>
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/item/ItemStack.h"

class InventoryTransactionItemGroup {
public:
    int32_t                      mItemId{0};
    int32_t                      mItemAux{0};
    int32_t                      mCount{0};
    std::unique_ptr<CompoundTag> mUserData{};

    InventoryTransactionItemGroup() = default;

    InventoryTransactionItemGroup(int32_t id, int32_t aux, int32_t count)
        : mItemId(id), mItemAux(aux), mCount(count) {}

    InventoryTransactionItemGroup(ItemStack const& item)
        : mItemId(0), mItemAux(item.getAuxValue()), mCount(item.getCount()) {
        if (item.getUserData()) {
            mUserData = item.getUserData()->clone();
        }
    }
};
