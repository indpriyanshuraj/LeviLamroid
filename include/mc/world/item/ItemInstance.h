#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include "mc/deps/nbt/CompoundTag.h"

class Item;
class Block;

class ItemInstance {
public:
    void*                         mItem{nullptr};
    std::unique_ptr<CompoundTag>  mUserData{};
    Block const*                  mBlock{nullptr};
    uint16_t                      mAuxValue{0};
    uint8_t                       mCount{0};
    bool                          mValid{false};

    ItemInstance() = default;
    virtual ~ItemInstance() = default;

    ItemInstance(Block const& block, int count = 1, int aux = 0);

    ItemInstance(ItemInstance const& other)
        : mItem(other.mItem),
          mUserData(other.mUserData ? other.mUserData->clone() : nullptr),
          mBlock(other.mBlock),
          mAuxValue(other.mAuxValue),
          mCount(other.mCount),
          mValid(other.mValid) {}

    ItemInstance(ItemInstance&&) noexcept = default;

    ItemInstance& operator=(ItemInstance const& other) {
        if (this != &other) {
            mItem = other.mItem;
            mUserData = other.mUserData ? other.mUserData->clone() : nullptr;
            mBlock = other.mBlock;
            mAuxValue = other.mAuxValue;
            mCount = other.mCount;
            mValid = other.mValid;
        }
        return *this;
    }

    ItemInstance& operator=(ItemInstance&&) noexcept = default;

    [[nodiscard]] bool isNull() const noexcept {
        return mCount <= 0 || mItem == nullptr;
    }

    [[nodiscard]] uint8_t getCount() const noexcept { return mCount; }
    void setCount(uint8_t count) noexcept { mCount = count; }

    [[nodiscard]] uint16_t getAuxValue() const noexcept { return mAuxValue; }
    void setAuxValue(uint16_t aux) noexcept { mAuxValue = aux; }

    [[nodiscard]] CompoundTag const* getUserData() const noexcept { return mUserData.get(); }
    [[nodiscard]] bool hasUserData() const noexcept { return mUserData != nullptr; }

    [[nodiscard]] Block const* getBlock() const noexcept { return mBlock; }
    void setBlock(Block const* block) noexcept { mBlock = block; }

    std::string mTypeName{};
    [[nodiscard]] std::string const& getTypeName() const noexcept { return mTypeName; }

    void reinit(std::string_view name, uint8_t count = 1, uint16_t aux = 0) {
        mTypeName = std::string(name);
        mCount = count;
        mAuxValue = aux;
        mValid = !name.empty();
    }

    [[nodiscard]] int getMaxStackSize() const noexcept { return 64; }
    [[nodiscard]] std::string getHoverName() const {
        return mTypeName.empty() ? "Item" : mTypeName;
    }
    [[nodiscard]] std::string getName() const { return getHoverName(); }
    [[nodiscard]] std::string getRawNameId() const { return mTypeName; }
    [[nodiscard]] std::string getCustomName() const { return getHoverName(); }
    [[nodiscard]] int getId() const noexcept { return 0; }
    [[nodiscard]] int getIdAux() const noexcept {
        return (static_cast<int>(getId()) << 16) | (static_cast<int>(mAuxValue) & 0xFFFF);
    }
    [[nodiscard]] bool isBlock() const noexcept { return mBlock != nullptr; }
    [[nodiscard]] bool isStackable() const noexcept { return true; }

    static ItemInstance const& getEmptyItem() {
        static ItemInstance const sEmpty{};
        return sEmpty;
    }
};
