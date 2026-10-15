#pragma once
#include "mc/world/actor/Actor.h"
#include "mc/world/item/ItemStack.h"
#include "mc/enums/Enums.h"

class Mob : public Actor {
public:
    virtual ~Mob() override = default;

    virtual void setEquippedSlot(EquipmentSlot slot, const ItemStack& item);
    [[nodiscard]] virtual const ItemStack& getCarriedItem() const noexcept;
};
