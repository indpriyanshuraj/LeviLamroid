#pragma once
#include "mc/deps/nbt/Tag.h"
#include <vector>

class IntArrayTag : public Tag {
public:
    std::vector<int32_t> mVal;

    IntArrayTag() = default;
    explicit IntArrayTag(std::vector<int32_t> val) : mVal(std::move(val)) {}

    [[nodiscard]] size_t size() const { return mVal.size(); }
    [[nodiscard]] bool empty() const { return mVal.empty(); }
    [[nodiscard]] int32_t operator[](size_t index) const { return mVal[index]; }
    [[nodiscard]] auto begin() const { return mVal.begin(); }
    [[nodiscard]] auto end() const { return mVal.end(); }
    [[nodiscard]] auto begin() { return mVal.begin(); }
    [[nodiscard]] auto end() { return mVal.end(); }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::IntArray; }
    [[nodiscard]] std::string toString() const override { return "[I; " + std::to_string(mVal.size()) + " ints]"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::IntArray && static_cast<IntArrayTag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<IntArrayTag>(mVal);
    }
};
