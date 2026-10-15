#pragma once
#include <cstdint>
#include <memory>
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/Facing.h"
#include "mc/world/item/HandSlot.h"
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/inventory/transaction/ComplexInventoryTransaction.h"

struct NetworkItemStackDescriptor {
    ItemStack mItem{};
    bool      mIncludeNetIds{false};
    int32_t   mStackNetId{0};

    NetworkItemStackDescriptor() = default;
    explicit NetworkItemStackDescriptor(ItemStack const& item) : mItem(item) {}
};

class ItemUseInventoryTransaction : public ComplexInventoryTransaction {
public:
    enum class ActionType : uint32_t {
        Place = 0,
        Use = 1,
        Destroy = 2
    };

    enum class TriggerType : uint32_t {
        Unknown = 0,
        PlayerInput = 1,
        Simulation = 2
    };

    enum class PredictedResult : uint32_t {
        Failure = 0,
        Success = 1
    };

    enum class ClientCooldownState : uint32_t {
        Off = 0,
        On = 1
    };

    struct ItemDescriptorWrapper {
        NetworkItemStackDescriptor mDescriptor{};
        [[nodiscard]] NetworkItemStackDescriptor const& get() const noexcept { return mDescriptor; }
        [[nodiscard]] NetworkItemStackDescriptor& get() noexcept { return mDescriptor; }
        [[nodiscard]] NetworkItemStackDescriptor const* operator->() const noexcept { return &mDescriptor; }
        [[nodiscard]] NetworkItemStackDescriptor* operator->() noexcept { return &mDescriptor; }
    };

    ActionType            mActionType{ActionType::Place};
    TriggerType           mTriggerType{TriggerType::PlayerInput};
    BlockPos              mPos{0, 0, 0};
    unsigned char         mFace{0};
    int32_t               mSlot{0};
    HandSlot              mHand{HandSlot::Mainhand};
    Vec3                  mFromPos{0.0f, 0.0f, 0.0f};
    Vec3                  mClickPos{0.0f, 0.0f, 0.0f};
    PredictedResult       mClientPredictedResult{PredictedResult::Success};
    ClientCooldownState   mClientCooldownState{ClientCooldownState::Off};
    uint32_t              mTargetBlockId{0};
    ItemDescriptorWrapper mItem{};

    ItemUseInventoryTransaction()
        : ComplexInventoryTransaction(Type::ItemUseTransaction) {}

    ItemUseInventoryTransaction(InventoryTransaction const& tx)
        : ComplexInventoryTransaction(Type::ItemUseTransaction, tx) {}

    ~ItemUseInventoryTransaction() override = default;

    void setSelectedItem(ItemStack const& item) {
        mItem.mDescriptor.mItem = item;
    }
};

inline std::unique_ptr<ComplexInventoryTransaction> ComplexInventoryTransaction::fromType(
    Type type,
    InventoryTransaction const& tx
) {
    if (type == Type::ItemUseTransaction) {
        return std::make_unique<ItemUseInventoryTransaction>(tx);
    }
    return std::make_unique<ComplexInventoryTransaction>(type, tx);
}
