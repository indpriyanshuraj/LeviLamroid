#pragma once
#include "mc/deps/nbt/Tag.h"

class IntTag : public Tag {
public:
    int32_t data{0};

    IntTag() = default;
    explicit IntTag(int32_t val) : data(val) {}
    operator int32_t() const { return data; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Int; }
    [[nodiscard]] std::string toString() const override { return std::to_string(data); }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Int && static_cast<IntTag const&>(other).data == data;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<IntTag>(data);
    }
};
