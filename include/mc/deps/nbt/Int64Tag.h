#pragma once
#include "mc/deps/nbt/Tag.h"

class Int64Tag : public Tag {
public:
    int64_t mVal{0};

    Int64Tag() = default;
    explicit Int64Tag(int64_t val) : mVal(val) {}
    operator int64_t() const { return mVal; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Int64; }
    [[nodiscard]] std::string toString() const override { return std::to_string(mVal) + "L"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Int64 && static_cast<Int64Tag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<Int64Tag>(mVal);
    }
};
