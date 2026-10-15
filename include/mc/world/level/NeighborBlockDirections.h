#pragma once
#include <set>
#include "mc/world/level/block/NeighborDirection.h"

class NeighborBlockDirections {
public:
    struct DirectionSetWrapper {
        std::set<NeighborDirection> mSet{};
        [[nodiscard]] std::set<NeighborDirection> const& get() const noexcept { return mSet; }
        [[nodiscard]] std::set<NeighborDirection>& get() noexcept { return mSet; }
    };

    DirectionSetWrapper mDirections{};

    NeighborBlockDirections() = default;
};
