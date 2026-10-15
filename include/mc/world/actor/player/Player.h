#pragma once
#include "mc/world/actor/Mob.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/dimension/Dimension.h"
#include "mc/world/level/Level.h"
#include "mc/world/actor/player/Inventory.h"
#include "mc/enums/Enums.h"

class Player : public Mob {
public:
    virtual ~Player() override = default;

    virtual void normalTick();
    virtual void setEquippedSlot(EquipmentSlot slot, const ItemStack& item) override;

    ItemStack& getOffhandSlot() { static ItemStack sItem; return sItem; }
    const ItemStack& getSelectedItem() const { static ItemStack sItem; return sItem; }
    [[nodiscard]] int getSelectedItemSlot() const noexcept { return 0; }
    bool isOffhandItemAllowed(const ItemStack& item) const { (void)item; return true; }

    virtual BlockSource& getDimensionBlockSource() const;
    virtual Dimension& getDimension() const override;

    virtual int32_t getDimensionId() const;
    virtual Level& getLevel() const override;
    virtual class Inventory& getInventory() const;
};
