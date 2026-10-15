#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <memory>

class Item {
public:
    virtual ~Item() = default;
};

class ItemRegistry {
public:
    struct ItemPtrWrapper {
        Item* mPtr{nullptr};
        [[nodiscard]] Item* get() const noexcept { return mPtr; }
        [[nodiscard]] Item* operator->() const noexcept { return mPtr; }
        explicit operator bool() const noexcept { return mPtr != nullptr; }
    };

    virtual ~ItemRegistry() = default;

    virtual ItemPtrWrapper getItem(int16_t id) const { (void)id; return {}; }
    virtual ItemPtrWrapper getItem(std::string_view name) const { (void)name; return {}; }
};
