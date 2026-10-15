#pragma once
#include <cstdint>
#include <memory>
#include "mc/world/inventory/transaction/InventoryTransaction.h"

class ComplexInventoryTransaction {
public:
    enum class Type : uint32_t {
        NormalTransaction = 0,
        InventoryMismatch = 1,
        ItemUseTransaction = 2,
        ItemUseOnActorTransaction = 3,
        ItemReleaseTransaction = 4
    };

    struct TransactionWrapper {
        InventoryTransaction mTx{};
        [[nodiscard]] InventoryTransaction const& get() const noexcept { return mTx; }
        [[nodiscard]] InventoryTransaction& get() noexcept { return mTx; }
        [[nodiscard]] InventoryTransaction const* operator->() const noexcept { return &mTx; }
        [[nodiscard]] InventoryTransaction* operator->() noexcept { return &mTx; }
    };

    Type               mType{Type::NormalTransaction};
    TransactionWrapper mTransaction{};

    ComplexInventoryTransaction() = default;
    explicit ComplexInventoryTransaction(Type type) : mType(type) {}
    ComplexInventoryTransaction(Type type, InventoryTransaction const& tx)
        : mType(type), mTransaction{tx} {}

    virtual ~ComplexInventoryTransaction() = default;

    [[nodiscard]] Type getType() const noexcept { return mType; }

    static std::unique_ptr<ComplexInventoryTransaction> fromType(
        Type type,
        InventoryTransaction const& tx
    );
};
