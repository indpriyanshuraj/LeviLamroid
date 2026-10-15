#pragma once
#include <cstdint>

struct Tick {
    uint64_t mTick{0};

    constexpr Tick() noexcept = default;
    constexpr explicit Tick(uint64_t tick) noexcept : mTick(tick) {}
    constexpr operator uint64_t() const noexcept { return mTick; }
    constexpr bool operator==(Tick const& rhs) const noexcept { return mTick == rhs.mTick; }
    constexpr bool operator!=(Tick const& rhs) const noexcept { return mTick != rhs.mTick; }
    constexpr bool operator<(Tick const& rhs) const noexcept { return mTick < rhs.mTick; }
};
