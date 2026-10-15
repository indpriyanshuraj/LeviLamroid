#pragma once
#include <memory>
#include "mc/network/Packet.h"
#include "mc/world/inventory/transaction/ComplexInventoryTransaction.h"

class InventoryTransactionPacketPayload {
public:
    std::unique_ptr<ComplexInventoryTransaction> mTransaction{};
    bool                                         mIsClientSide{false};

    InventoryTransactionPacketPayload() = default;

    InventoryTransactionPacketPayload(
        std::unique_ptr<ComplexInventoryTransaction> transaction,
        bool isClientSide = false
    ) : mTransaction(std::move(transaction)), mIsClientSide(isClientSide) {}
};

class InventoryTransactionPacket : public Packet {
public:
    InventoryTransactionPacketPayload mPayload{};

    InventoryTransactionPacket() = default;

    explicit InventoryTransactionPacket(InventoryTransactionPacketPayload payload)
        : mPayload(std::move(payload)) {}

    explicit InventoryTransactionPacket(
        std::unique_ptr<ComplexInventoryTransaction> transaction,
        bool isClientSide = false
    ) : mPayload(std::move(transaction), isClientSide) {}

    ~InventoryTransactionPacket() override = default;

    [[nodiscard]] MinecraftPacketIds getId() const override {
        return MinecraftPacketIds::InventoryTransaction;
    }

    [[nodiscard]] std::string getName() const override {
        return "InventoryTransactionPacket";
    }

    [[nodiscard]] ComplexInventoryTransaction* getTransaction() const noexcept {
        return mPayload.mTransaction.get();
    }
};
