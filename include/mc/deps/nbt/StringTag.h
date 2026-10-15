#pragma once
#include "mc/deps/nbt/Tag.h"

class StringTag : public Tag {
public:
    std::string mVal;

    StringTag() = default;
    explicit StringTag(std::string val) : mVal(std::move(val)) {}
    explicit StringTag(std::string_view val) : mVal(val) {}
    explicit StringTag(char const* val) : mVal(val ? val : "") {}

    [[nodiscard]] std::string const& data() const { return mVal; }
    [[nodiscard]] std::string& data() { return mVal; }
    operator std::string const&() const { return mVal; }
    operator std::string_view() const { return mVal; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::String; }
    [[nodiscard]] std::string toString() const override { return "\"" + mVal + "\""; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::String && static_cast<StringTag const&>(other).mVal == mVal;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<StringTag>(mVal);
    }
};
