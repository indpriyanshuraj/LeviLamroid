#pragma once
#include "mc/deps/nbt/Tag.h"
#include "mc/deps/nbt/ByteTag.h"
#include "mc/deps/nbt/ShortTag.h"
#include "mc/deps/nbt/IntTag.h"
#include "mc/deps/nbt/Int64Tag.h"
#include "mc/deps/nbt/FloatTag.h"
#include "mc/deps/nbt/DoubleTag.h"
#include "mc/deps/nbt/ByteArrayTag.h"
#include "mc/deps/nbt/StringTag.h"
#include "mc/deps/nbt/IntArrayTag.h"
#include "mc/deps/nbt/EndTag.h"
#include "mc/deps/nbt/ListTag.h"

#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <zlib.h>
#include <cstring>
#include <functional>

class CompoundTag : public Tag {
public:
    std::map<std::string, CompoundTagVariant, std::less<>> mTags;

    CompoundTag() = default;

    [[nodiscard]] size_t size() const { return mTags.size(); }
    [[nodiscard]] bool empty() const { return mTags.empty(); }
    [[nodiscard]] auto const& get() const noexcept { return mTags; }

    [[nodiscard]] auto begin() const { return mTags.begin(); }
    [[nodiscard]] auto end() const { return mTags.end(); }
    [[nodiscard]] auto begin() { return mTags.begin(); }
    [[nodiscard]] auto end() { return mTags.end(); }

    [[nodiscard]] bool contains(std::string_view name) const {
        return mTags.find(name) != mTags.end();
    }

    Tag* put(std::string name, std::unique_ptr<Tag> tag) {
        if (!tag) return nullptr;
        Tag* raw = tag.get();
        mTags[std::move(name)] = CompoundTagVariant(std::shared_ptr<Tag>(std::move(tag)));
        return raw;
    }

    void putInt(std::string name, int32_t val) {
        put(std::move(name), std::make_unique<IntTag>(val));
    }
    void putString(std::string name, std::string val) {
        put(std::move(name), std::make_unique<StringTag>(std::move(val)));
    }
    void putByte(std::string name, int8_t val) {
        put(std::move(name), std::make_unique<ByteTag>(val));
    }
    void putShort(std::string name, int16_t val) {
        put(std::move(name), std::make_unique<ShortTag>(val));
    }
    void putFloat(std::string name, float val) {
        put(std::move(name), std::make_unique<FloatTag>(val));
    }
    void putDouble(std::string name, double val) {
        put(std::move(name), std::make_unique<DoubleTag>(val));
    }
    void putCompound(std::string name, std::unique_ptr<CompoundTag> tag) {
        put(std::move(name), std::unique_ptr<Tag>(tag.release()));
    }

    [[nodiscard]] Tag::Type getId() const override { return Tag::Type::Compound; }
    [[nodiscard]] std::string toString() const override { return "TAG_Compound[" + std::to_string(mTags.size()) + "]"; }
    [[nodiscard]] bool equals(Tag const& other) const override {
        return other.getId() == Tag::Type::Compound;
    }
    [[nodiscard]] std::unique_ptr<Tag> copy() const override {
        return clone();
    }
    [[nodiscard]] std::unique_ptr<CompoundTag> clone() const {
        auto res = std::make_unique<CompoundTag>();
        for (auto const& [k, v] : mTags) {
            if (v.mTag) {
                res->put(k, v.mTag->copy());
            }
        }
        return res;
    }

    struct BinaryNbtError {
        std::string mMessage{"Invalid NBT format"};
        [[nodiscard]] std::string const& message() const noexcept { return mMessage; }
    };

    struct BinaryNbtResult {
        std::unique_ptr<CompoundTag> mTag;
        BinaryNbtError mErr;
        bool mOk{false};

        BinaryNbtResult() = default;
        BinaryNbtResult(std::unique_ptr<CompoundTag> tag) : mTag(std::move(tag)), mOk(mTag != nullptr) {}
        BinaryNbtResult(std::string errMsg) : mErr{std::move(errMsg)}, mOk(false) {}

        explicit operator bool() const noexcept { return mOk && mTag != nullptr; }
        CompoundTag& operator*() noexcept { return *mTag; }
        CompoundTag const& operator*() const noexcept { return *mTag; }
        CompoundTag* operator->() noexcept { return mTag.get(); }
        CompoundTag const* operator->() const noexcept { return mTag.get(); }
        CompoundTag* get() noexcept { return mTag.get(); }
        CompoundTag const* get() const noexcept { return mTag.get(); }
        [[nodiscard]] BinaryNbtError const& error() const noexcept { return mErr; }
        std::unique_ptr<CompoundTag> release() { return std::move(mTag); }

        operator std::unique_ptr<CompoundTag>() && { return std::move(mTag); }
    };

    // Binary NBT Deserializer with automatic gzip/zlib decompression
    static BinaryNbtResult fromBinaryNbt(std::string_view bytes, bool isLittleEndian = true) {
        if (bytes.size() < 2) return BinaryNbtResult("Buffer too short");

        std::string uncompressed;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(bytes.data());
        size_t len = bytes.size();

        // Gzip header (0x1F, 0x8B) or Zlib header (0x78)
        if ((data[0] == 0x1F && data[1] == 0x8B) || (data[0] == 0x78)) {
            z_stream strm{};
            int windowBits = (data[0] == 0x1F) ? (16 + MAX_WBITS) : MAX_WBITS;
            if (inflateInit2(&strm, windowBits) == Z_OK) {
                strm.next_in = const_cast<Bytef*>(data);
                strm.avail_in = static_cast<uInt>(len);
                char buffer[32768];
                int ret = Z_OK;
                while (ret == Z_OK) {
                    strm.next_out = reinterpret_cast<Bytef*>(buffer);
                    strm.avail_out = sizeof(buffer);
                    ret = inflate(&strm, Z_NO_FLUSH);
                    uncompressed.append(buffer, sizeof(buffer) - strm.avail_out);
                }
                inflateEnd(&strm);
                if (ret == Z_STREAM_END || ret == Z_OK) {
                    data = reinterpret_cast<const uint8_t*>(uncompressed.data());
                    len = uncompressed.size();
                }
            }
        }

        auto root = std::make_unique<CompoundTag>();
        if (len == 0) return root;

        size_t offset = 0;
        auto readU8 = [&]() -> uint8_t {
            return (offset < len) ? data[offset++] : 0;
        };
        auto readU16 = [&]() -> uint16_t {
            if (offset + 2 > len) return 0;
            uint16_t v = isLittleEndian
                ? static_cast<uint16_t>(data[offset] | (data[offset + 1] << 8))
                : static_cast<uint16_t>((data[offset] << 8) | data[offset + 1]);
            offset += 2;
            return v;
        };
        auto readU32 = [&]() -> uint32_t {
            if (offset + 4 > len) return 0;
            uint32_t v = isLittleEndian
                ? static_cast<uint32_t>(data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24))
                : static_cast<uint32_t>((data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3]);
            offset += 4;
            return v;
        };
        auto readString = [&]() -> std::string {
            uint16_t slen = readU16();
            if (offset + slen > len) return "";
            std::string s(reinterpret_cast<const char*>(data + offset), slen);
            offset += slen;
            return s;
        };

        std::function<std::unique_ptr<Tag>(uint8_t)> readPayload;
        readPayload = [&](uint8_t type) -> std::unique_ptr<Tag> {
            switch (static_cast<Tag::Type>(type)) {
                case Tag::Type::Byte:
                    return std::make_unique<ByteTag>(static_cast<int8_t>(readU8()));
                case Tag::Type::Short:
                    return std::make_unique<ShortTag>(static_cast<int16_t>(readU16()));
                case Tag::Type::Int:
                    return std::make_unique<IntTag>(static_cast<int32_t>(readU32()));
                case Tag::Type::Int64: {
                    uint64_t low = readU32();
                    uint64_t high = readU32();
                    uint64_t v = isLittleEndian ? (low | (high << 32)) : ((low << 32) | high);
                    return std::make_unique<Int64Tag>(static_cast<int64_t>(v));
                }
                case Tag::Type::Float: {
                    uint32_t u = readU32();
                    float f;
                    std::memcpy(&f, &u, 4);
                    return std::make_unique<FloatTag>(f);
                }
                case Tag::Type::Double: {
                    uint64_t low = readU32();
                    uint64_t high = readU32();
                    uint64_t u = isLittleEndian ? (low | (high << 32)) : ((low << 32) | high);
                    double d;
                    std::memcpy(&d, &u, 8);
                    return std::make_unique<DoubleTag>(d);
                }
                case Tag::Type::ByteArray: {
                    uint32_t count = readU32();
                    std::vector<int8_t> arr(count);
                    for (uint32_t i = 0; i < count && offset < len; ++i) {
                        arr[i] = static_cast<int8_t>(readU8());
                    }
                    return std::make_unique<ByteArrayTag>(std::move(arr));
                }
                case Tag::Type::String:
                    return std::make_unique<StringTag>(readString());
                case Tag::Type::List: {
                    uint8_t elemType = readU8();
                    uint32_t count = readU32();
                    auto list = std::make_unique<ListTag>();
                    for (uint32_t i = 0; i < count && offset < len; ++i) {
                        auto elem = readPayload(elemType);
                        if (elem) list->add(std::move(elem));
                    }
                    return list;
                }
                case Tag::Type::Compound: {
                    auto comp = std::make_unique<CompoundTag>();
                    while (offset < len) {
                        uint8_t t = readU8();
                        if (t == 0) break;
                        std::string k = readString();
                        auto v = readPayload(t);
                        if (v) comp->put(std::move(k), std::move(v));
                    }
                    return comp;
                }
                case Tag::Type::IntArray: {
                    uint32_t count = readU32();
                    std::vector<int32_t> arr(count);
                    for (uint32_t i = 0; i < count && offset + 4 <= len; ++i) {
                        arr[i] = static_cast<int32_t>(readU32());
                    }
                    return std::make_unique<IntArrayTag>(std::move(arr));
                }
                default:
                    return nullptr;
            }
        };

        // Read root tag
        uint8_t rootType = readU8();
        if (rootType == static_cast<uint8_t>(Tag::Type::Compound)) {
            (void)readString(); // root name
            while (offset < len) {
                uint8_t t = readU8();
                if (t == 0) break;
                std::string k = readString();
                auto v = readPayload(t);
                if (v) root->put(std::move(k), std::move(v));
            }
        }
        return root;
    }

    [[nodiscard]] std::string toBinaryNbt(bool isLittleEndian = true) const {
        (void)isLittleEndian;
        return "";
    }
};
