#pragma once
#include "mc/world/item/ItemInstance.h"

class ItemStack : public ItemInstance {
public:
    ItemStack() = default;
    ~ItemStack() override = default;

    ItemStack(Block const& block, int count = 1, int aux = 0);

    ItemStack(ItemStack const& other) : ItemInstance(other) {}
    ItemStack(ItemStack&&) noexcept = default;
    ItemStack& operator=(ItemStack const& other) {
        ItemInstance::operator=(other);
        return *this;
    }
    ItemStack& operator=(ItemStack&&) noexcept = default;

    void reinit(std::string_view name, uint8_t count = 1, uint16_t aux = 0) {
        ItemInstance::reinit(name, count, aux);
    }

    void reinit(void* item, uint8_t count, uint16_t aux) noexcept {
        mItem = item;
        mCount = count;
        mAuxValue = aux;
    }

    void reinit(class Item const& item, uint8_t count, uint16_t aux) noexcept {
        mItem = const_cast<void*>(reinterpret_cast<void const*>(&item));
        mCount = count;
        mAuxValue = aux;
    }

    [[nodiscard]] std::string getName() const {
        return mTypeName;
    }

    static ItemStack const& getEmptyItem() {
        static ItemStack const sEmpty{};
        return sEmpty;
    }
};
