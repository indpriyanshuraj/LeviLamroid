#pragma once
#include <string>
#include <string_view>
#include <memory>
#include "mc/deps/core/string/HashedString.h"
#include "mc/world/level/block/Block.h"

class BlockTypeRegistryReadHandle {
public:
    void* mPtr{nullptr};
    BlockTypeRegistryReadHandle() = default;
    explicit BlockTypeRegistryReadHandle(void* ptr) : mPtr(ptr) {}
    [[nodiscard]] void* get() const noexcept { return mPtr; }
};

class BlockTypeRegistry {
public:
    virtual ~BlockTypeRegistry() = default;

    static BlockTypeRegistry& getInstance() {
        static BlockTypeRegistry sInstance{};
        return sInstance;
    }

    static Block const* getBlock(HashedString const& name) {
        (void)name;
        static Block const sAir{};
        return &sAir;
    }

    static Block const* getBlock(std::string_view name) {
        (void)name;
        static Block const sAir{};
        return &sAir;
    }

    static BlockTypeRegistryReadHandle computeReadHandle() {
        return BlockTypeRegistryReadHandle{nullptr};
    }
};
