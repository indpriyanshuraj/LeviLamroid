#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <string_view>

class IDataOutput;
class IDataInput;

class Tag {
public:
    enum class Type : uint8_t {
        End = 0,
        Byte = 1,
        Short = 2,
        Int = 3,
        Int64 = 4,
        Float = 5,
        Double = 6,
        ByteArray = 7,
        String = 8,
        List = 9,
        Compound = 10,
        IntArray = 11,
    };

    virtual ~Tag() = default;
    [[nodiscard]] virtual Type getId() const = 0;
    [[nodiscard]] virtual std::string toString() const = 0;
    [[nodiscard]] virtual bool equals(Tag const& other) const = 0;
    [[nodiscard]] virtual std::unique_ptr<Tag> copy() const = 0;
    virtual void write(IDataOutput& output) const { (void)output; }
    virtual void load(IDataInput& input) { (void)input; }

    bool operator==(Tag const& other) const { return equals(other); }
    bool operator!=(Tag const& other) const { return !equals(other); }
};
