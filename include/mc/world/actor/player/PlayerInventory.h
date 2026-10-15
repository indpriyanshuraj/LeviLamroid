#pragma once
#include "mc/world/item/ItemStack.h"
#include "mc/enums/Enums.h"

class PlayerInventory {
public:
    virtual ~PlayerInventory() = default;

    virtual void setItem(int slot, const ItemStack& item, ContainerID container);
    virtual void clearSlot(int slot, ContainerID container);
};
