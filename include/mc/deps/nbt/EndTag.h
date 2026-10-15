#pragma once
#include "mc/deps/nbt/Tag.h"

class EndTag : public Tag {
public:
    EndTag() = default;

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::End; }
    [[nodiscard]] std::string toString() const override { return "TAG_End"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::End;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<EndTag>();
    }
};
