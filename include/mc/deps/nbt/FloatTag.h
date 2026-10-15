#pragma once
#include "mc/deps/nbt/Tag.h"

class FloatTag : public Tag {
public:
    float mVal{0.0f};

    FloatTag() = default;
    explicit FloatTag(float val) : mVal(val) {}
    operator float() const { return mVal; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Float; }
    [[nodiscard]] std::string toString() const override { return std::to_string(mVal) + "f"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Float && static_cast<FloatTag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<FloatTag>(mVal);
    }
};
