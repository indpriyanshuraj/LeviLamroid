#pragma once
#include <cstdint>

class BlockChangeContext {
public:
    uint32_t mFlags{0};

    constexpr BlockChangeContext() noexcept = default;
    constexpr explicit BlockChangeContext(uint32_t flags) noexcept : mFlags(flags) {}
};
