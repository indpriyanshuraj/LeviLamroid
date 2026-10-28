#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include <utility>
#include <optional>
#include "mc/network/MinecraftPacketIds.h"
#include "mc/network/packet/cerealize/core/SerializationMode.h"
#include "mc/platform/Result.h"

class BinaryStream;
class ReadOnlyBinaryStream;
namespace cereal { struct ReflectionCtx; }

class Packet {
public:
    uint32_t mSenderSubClientId{0};
    bool     mIsReliable{true};

    virtual ~Packet() = default;

    [[nodiscard]] virtual MinecraftPacketIds getId() const = 0;
    [[nodiscard]] virtual ::std::string_view getName() const = 0;

    virtual ::SerializationMode getSerializationMode() const { return ::SerializationMode::ManualOnly; }
    virtual void setSerializationMode(::SerializationMode) {}

    virtual void writeWithSerializationMode(
        ::BinaryStream&,
        ::cereal::ReflectionCtx const&,
        ::std::optional<::SerializationMode>
    ) const {}

    virtual void write(::BinaryStream&, ::cereal::ReflectionCtx const&) const {}
    virtual void write(::BinaryStream&) const {}

    virtual ::Bedrock::Result<void> read(::ReadOnlyBinaryStream&, ::cereal::ReflectionCtx const&) { return {}; }
    [[nodiscard]] virtual bool disallowBatching() const { return false; }
    [[nodiscard]] virtual bool isValid() const { return true; }
    virtual unsigned long long getMaxSize() const { return 1024 * 1024; }
    virtual ::std::string toString() const { return ""; }
    virtual ::Bedrock::Result<void> _read(::ReadOnlyBinaryStream&) { return {}; }
    virtual ::Bedrock::Result<void> _read(::ReadOnlyBinaryStream&, ::cereal::ReflectionCtx const&) { return {}; }
};

namespace ll {
template <typename Payload>
class PayloadPacket : public ::Packet {
public:
    using ::Packet::write;
    using ::Packet::read;
    Payload mPayload;

    PayloadPacket() = default;
    template <typename... Args>
    explicit PayloadPacket(Args&&... args) : mPayload(std::forward<Args>(args)...) {}
    virtual ~PayloadPacket() override = default;
};
} // namespace ll
