#pragma once
#include "mc/deps/nbt/Tag.h"
#include <vector>

class ByteArrayTag : public Tag {
public:
    std::vector<int8_t> mVal;

    ByteArrayTag() = default;
    explicit ByteArrayTag(std::vector<int8_t> val) : mVal(std::move(val)) {}

    [[nodiscard]] size_t size() const { return mVal.size(); }
    [[nodiscard]] bool empty() const { return mVal.empty(); }
    [[nodiscard]] int8_t operator[](size_t index) const { return mVal[index]; }
    [[nodiscard]] auto begin() const { return mVal.begin(); }
    [[nodiscard]] auto end() const { return mVal.end(); }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::ByteArray; }
    [[nodiscard]] std::string toString() const override { return "[B; " + std::to_string(mVal.size()) + " bytes]"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::ByteArray && static_cast<ByteArrayTag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<ByteArrayTag>(mVal);
    }
};
