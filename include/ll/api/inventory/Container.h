#pragma once
#include "ll/api/Macros.h"
#include "ll/api/inventory/ItemStack.h"
#include <string>
#include <dlfcn.h>

extern "C" {
LL_API int ll_container_get_size(void* container);
LL_API bool ll_container_get_item(void* container, int slot, void* outItemStack);
LL_API bool ll_container_set_item(void* container, int slot, const void* itemStack);
LL_API bool ll_container_clear_slot(void* container, int slot);
LL_API bool ll_container_has_item(void* container, const char* typeName, int count);
}

namespace ll::inventory {

class LL_API Container {
protected:
    void* mNativePtr{nullptr};

public:
    explicit Container(void* ptr = nullptr) : mNativePtr(ptr) {}
    virtual ~Container() = default;

    [[nodiscard]] void* getNativeHandle() const noexcept { return mNativePtr; }
    [[nodiscard]] bool isValid() const noexcept { return mNativePtr != nullptr; }

    [[nodiscard]] int getSize() const {
        if (!mNativePtr) return 0;
        return ll_container_get_size(mNativePtr);
    }

    [[nodiscard]] ItemStack getItem(int slot) const {
        if (!mNativePtr || slot < 0) return {};
        alignas(8) uint8_t rawStack[160] = {0};
        if (ll_container_get_item(mNativePtr, slot, rawStack)) {
            // Unpack basic details
            uint8_t count = rawStack[34];
            int16_t aux = *reinterpret_cast<int16_t*>(rawStack + 36);
            return ItemStack("minecraft:item", count, aux);
        }
        return {};
    }

    bool setItem(int slot, const ItemStack& item) {
        if (!mNativePtr || slot < 0) return false;
        alignas(8) uint8_t rawStack[160] = {0};
        rawStack[34] = item.getCount();
        *reinterpret_cast<int16_t*>(rawStack + 36) = item.getAuxValue();
        return ll_container_set_item(mNativePtr, slot, rawStack);
    }

    bool clearSlot(int slot) {
        if (!mNativePtr || slot < 0) return false;
        return ll_container_clear_slot(mNativePtr, slot);
    }

    [[nodiscard]] bool hasItem(const std::string& typeName, int count = 1) const {
        if (!mNativePtr) return false;
        return ll_container_has_item(mNativePtr, typeName.c_str(), count);
    }
};

} // namespace ll::inventory
