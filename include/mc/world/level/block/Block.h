#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/level/block/BlockType.h"

enum class BlockProperty : uint64_t {
    None = 0,
    Door = 1ULL << 0,
    Stair = 1ULL << 1,
    Slab = 1ULL << 2,
    Trapdoor = 1ULL << 3,
    Button = 1ULL << 4,
    Sign = 1ULL << 5,
    PressurePlate = 1ULL << 6
};

class Block {
public:
    uint32_t         mRuntimeId{0};
    uint32_t         mNetworkId{0};
    HashedString     mTypeName{};
    BlockType const* mBlockType{nullptr};
    bool             mIsAir{false};
    CompoundTag      mSerializationId{};

    Block() = default;
    ~Block() = default;

    [[nodiscard]] bool hasProperty(BlockProperty prop) const noexcept {
        (void)prop;
        return false;
    }

    bool operator==(Block const& other) const noexcept {
        return mRuntimeId == other.mRuntimeId;
    }

    bool operator!=(Block const& other) const noexcept {
        return mRuntimeId != other.mRuntimeId;
    }

    [[nodiscard]] std::string const& getTypeName() const noexcept {
        return mTypeName.getString();
    }

    [[nodiscard]] HashedString const& getHashedTypeName() const noexcept {
        return mTypeName;
    }

    [[nodiscard]] bool isAir() const noexcept {
        return mIsAir;
    }

    [[nodiscard]] uint32_t getRuntimeId() const noexcept {
        return mRuntimeId;
    }

    [[nodiscard]] uint32_t getNetworkId() const noexcept {
        return mNetworkId;
    }

    [[nodiscard]] int16_t getBlockItemId() const noexcept {
        return 0;
    }

    [[nodiscard]] std::string buildDescriptionName() const {
        return getTypeName();
    }

    [[nodiscard]] std::string getDisplayName() const {
        return getTypeName();
    }

    [[nodiscard]] BlockType const& getBlockType() const noexcept {
        static BlockType const sDefaultType{};
        return mBlockType ? *mBlockType : sDefaultType;
    }

    template <typename T, typename StateType>
    [[nodiscard]] std::optional<T> getState(StateType const& stateDesc) const {
        (void)stateDesc;
        return std::nullopt;
    }

    static Block const* tryGetFromRegistry(HashedString const& name) {
        static std::unordered_map<std::string, std::unique_ptr<Block>> sRegistry;
        auto const& str = name.getString();
        auto it = sRegistry.find(str);
        if (it != sRegistry.end()) return it->second.get();
        auto block = std::make_unique<Block>();
        block->mTypeName = name;
        auto* ptr = block.get();
        sRegistry.emplace(str, std::move(block));
        return ptr;
    }
};

inline Block const& BlockType::getPlacementBlock(
    Actor const& by,
    BlockPos const& pos,
    unsigned char face,
    Vec3 const& clickPos,
    int itemAux
) const {
    (void)by;
    (void)pos;
    (void)face;
    (void)clickPos;
    (void)itemAux;
    static Block const sDefault{};
    return sDefault;
}

#include "mc/world/item/ItemInstance.h"
#include "mc/world/item/ItemStack.h"

inline ItemInstance::ItemInstance(Block const& block, int count, int aux)
    : mItem(nullptr),
      mUserData(nullptr),
      mBlock(&block),
      mAuxValue(static_cast<uint16_t>(aux)),
      mCount(static_cast<uint8_t>(count)),
      mValid(true),
      mTypeName(block.getTypeName()) {}

inline ItemStack::ItemStack(Block const& block, int count, int aux)
    : ItemInstance(block, count, aux) {}

inline ItemInstance BlockType::asItemInstance(Block const& block, void* actor) const {
    (void)actor;
    return ItemInstance(block, 1, 0);
}

