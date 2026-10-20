#pragma once
#include "ll/api/Macros.h"
#include <string>
#include <cstdint>

namespace ll::inventory {

class LL_API ItemStack {
public:
    std::string mTypeName;
    uint8_t     mCount{0};
    int16_t     mAuxValue{0};
    std::string mCustomName;

    ItemStack() = default;
    ItemStack(std::string typeName, uint8_t count = 1, int16_t aux = 0, std::string customName = "")
        : mTypeName(std::move(typeName)), mCount(count), mAuxValue(aux), mCustomName(std::move(customName)) {}

    [[nodiscard]] bool isNull() const noexcept {
        return mCount == 0 || mTypeName.empty() || mTypeName == "minecraft:air";
    }

    [[nodiscard]] const std::string& getTypeName() const noexcept { return mTypeName; }
    void setTypeName(std::string name) { mTypeName = std::move(name); }

    [[nodiscard]] uint8_t getCount() const noexcept { return mCount; }
    void setCount(uint8_t count) noexcept { mCount = count; }

    [[nodiscard]] int16_t getAuxValue() const noexcept { return mAuxValue; }
    void setAuxValue(int16_t aux) noexcept { mAuxValue = aux; }

    [[nodiscard]] const std::string& getCustomName() const noexcept { return mCustomName; }
    void setCustomName(std::string name) { mCustomName = std::move(name); }
};

} // namespace ll::inventory
