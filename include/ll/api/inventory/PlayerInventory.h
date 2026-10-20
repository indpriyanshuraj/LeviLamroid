#pragma once
#include "ll/api/Macros.h"
#include "ll/api/inventory/Container.h"

extern "C" {
LL_API void* ll_player_get_inventory(void* player);
LL_API bool ll_player_get_armor(void* player, int slot, void* outItemStack);
LL_API bool ll_player_set_armor(void* player, int slot, const void* itemStack);
LL_API bool ll_player_get_offhand(void* player, void* outItemStack);
LL_API bool ll_player_set_offhand(void* player, const void* itemStack);
LL_API int ll_player_get_selected_slot(void* player);
LL_API bool ll_player_set_selected_slot(void* player, int slot);
}

namespace ll::inventory {

class LL_API PlayerInventory : public Container {
private:
    void* mPlayerPtr{nullptr};

public:
    explicit PlayerInventory(void* player = nullptr)
        : Container(player ? ll_player_get_inventory(player) : nullptr), mPlayerPtr(player) {}

    [[nodiscard]] void* getPlayer() const noexcept { return mPlayerPtr; }

    [[nodiscard]] ItemStack getArmor(int slot) const {
        if (!mPlayerPtr || slot < 0 || slot > 3) return {};
        alignas(8) uint8_t rawStack[160] = {0};
        if (ll_player_get_armor(mPlayerPtr, slot, rawStack)) {
            uint8_t count = rawStack[34];
            int16_t aux = *reinterpret_cast<int16_t*>(rawStack + 36);
            return ItemStack("minecraft:armor", count, aux);
        }
        return {};
    }

    bool setArmor(int slot, const ItemStack& item) {
        if (!mPlayerPtr || slot < 0 || slot > 3) return false;
        alignas(8) uint8_t rawStack[160] = {0};
        rawStack[34] = item.getCount();
        *reinterpret_cast<int16_t*>(rawStack + 36) = item.getAuxValue();
        return ll_player_set_armor(mPlayerPtr, slot, rawStack);
    }

    [[nodiscard]] ItemStack getOffhand() const {
        if (!mPlayerPtr) return {};
        alignas(8) uint8_t rawStack[160] = {0};
        if (ll_player_get_offhand(mPlayerPtr, rawStack)) {
            uint8_t count = rawStack[34];
            int16_t aux = *reinterpret_cast<int16_t*>(rawStack + 36);
            return ItemStack("minecraft:offhand", count, aux);
        }
        return {};
    }

    bool setOffhand(const ItemStack& item) {
        if (!mPlayerPtr) return false;
        alignas(8) uint8_t rawStack[160] = {0};
        rawStack[34] = item.getCount();
        *reinterpret_cast<int16_t*>(rawStack + 36) = item.getAuxValue();
        return ll_player_set_offhand(mPlayerPtr, rawStack);
    }

    [[nodiscard]] int getSelectedSlot() const {
        if (!mPlayerPtr) return 0;
        return ll_player_get_selected_slot(mPlayerPtr);
    }

    bool setSelectedSlot(int slot) {
        if (!mPlayerPtr || slot < 0 || slot > 8) return false;
        return ll_player_set_selected_slot(mPlayerPtr, slot);
    }
};

} // namespace ll::inventory
