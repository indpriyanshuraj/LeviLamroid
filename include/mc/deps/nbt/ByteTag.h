#pragma once
#include "mc/deps/nbt/Tag.h"

class ByteTag : public Tag {
public:
    int8_t data{0};

    ByteTag() = default;
    explicit ByteTag(int8_t val) : data(val) {}
    explicit ByteTag(int val) : data(static_cast<int8_t>(val)) {}
    operator int8_t() const { return data; }
    operator uint8_t() const { return static_cast<uint8_t>(data); }
    operator int() const { return data; }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Byte; }
    [[nodiscard]] std::string toString() const override { return std::to_string(data) + "b"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Byte && static_cast<ByteTag const&>(other).data == data;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return std::make_unique<ByteTag>(data);
    }
};
