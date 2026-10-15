#pragma once
#include "mc/deps/nbt/Tag.h"

class DoubleTag : public Tag {
public:
    double mVal{0.0};

    DoubleTag() = default;
    explicit DoubleTag(double val) : mVal(val) {}
    operator double() const { return mVal; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Double; }
    [[nodiscard]] std::string toString() const override { return std::to_string(mVal) + "d"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Double && static_cast<DoubleTag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<DoubleTag>(mVal);
    }
};
