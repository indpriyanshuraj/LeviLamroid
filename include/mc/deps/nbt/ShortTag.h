#pragma once
#include "mc/deps/nbt/Tag.h"

class ShortTag : public Tag {
public:
    int16_t mVal{0};

    ShortTag() = default;
    explicit ShortTag(int16_t val) : mVal(val) {}
    operator int16_t() const { return mVal; }
    operator uint16_t() const { return static_cast<uint16_t>(mVal); }
    operator int() const { return mVal; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Short; }
    [[nodiscard]] std::string toString() const override { return std::to_string(mVal) + "s"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Short && static_cast<ShortTag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<ShortTag>(mVal);
    }
};
