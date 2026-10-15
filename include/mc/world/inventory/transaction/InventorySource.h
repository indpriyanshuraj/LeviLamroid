#pragma once
#include <cstdint>
#include "mc/world/ContainerID.h"
#include "mc/world/inventory/transaction/InventorySourceType.h"

class InventorySource {
public:
    enum class InventorySourceFlags : uint32_t {
        NoFlag = 0,
        WorldInteraction_Random = 1
    };

    InventorySourceType  mType{InventorySourceType::Invalid};
    ContainerID          mContainerId{ContainerID::Invalid};
    InventorySourceFlags mFlags{InventorySourceFlags::NoFlag};

    constexpr InventorySource() noexcept = default;

    constexpr InventorySource(
        InventorySourceType type,
        ContainerID containerId = ContainerID::Invalid,
        InventorySourceFlags flags = InventorySourceFlags::NoFlag
    ) noexcept
        : mType(type), mContainerId(containerId), mFlags(flags) {}

    [[nodiscard]] constexpr InventorySourceType getType() const noexcept { return mType; }
    [[nodiscard]] constexpr ContainerID getContainerId() const noexcept { return mContainerId; }
    [[nodiscard]] constexpr InventorySourceFlags getFlags() const noexcept { return mFlags; }

    [[nodiscard]] constexpr bool operator==(InventorySource const& rhs) const noexcept {
        return mType == rhs.mType && mContainerId == rhs.mContainerId && mFlags == rhs.mFlags;
    }

    [[nodiscard]] constexpr bool operator!=(InventorySource const& rhs) const noexcept {
        return !(*this == rhs);
    }
};
