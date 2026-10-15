#pragma once
#include <vector>
#include "mc/world/inventory/transaction/InventoryAction.h"

class InventoryTransaction {
public:
    std::vector<InventoryAction> mActions{};

    InventoryTransaction() = default;

    void addAction(InventoryAction const& action) {
        mActions.push_back(action);
    }

    void addAction(InventoryAction&& action) {
        mActions.push_back(std::move(action));
    }

    [[nodiscard]] std::vector<InventoryAction> const& getActions() const noexcept {
        return mActions;
    }

    [[nodiscard]] std::vector<InventoryAction>& getActions() noexcept {
        return mActions;
    }

    [[nodiscard]] size_t size() const noexcept { return mActions.size(); }
    [[nodiscard]] bool empty() const noexcept { return mActions.empty(); }
};
