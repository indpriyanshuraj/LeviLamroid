#pragma once
#include <string>
#include <vector>
#include <variant>
#include "mc/network/Packet.h"

enum class TextPacketType : uint8_t {
    Raw = 0,
    Chat = 1,
    Translate = 2,
    Popup = 3,
    JukeboxPopup = 4,
    Tip = 5,
    SystemMessage = 6,
    Whisper = 7,
    Announcement = 8,
    TextObject = 9,
    TextObjectWhisper = 10,
    TextObjectAnnouncement = 11
};

namespace TextPacketPayload {

struct StringWrapper {
    std::string mStr{};
    [[nodiscard]] std::string const& get() const noexcept { return mStr; }
    [[nodiscard]] std::string& get() noexcept { return mStr; }
    operator std::string const&() const noexcept { return mStr; }
};

struct AuthorAndMessage {
    TextPacketType mType{TextPacketType::Chat};
    StringWrapper  mAuthor{};
    StringWrapper  mMessage{};
    std::string    mXuid{};
    std::string    mPlatformId{};
};

struct RawMessage {
    TextPacketType mType{TextPacketType::Raw};
    StringWrapper  mMessage{};
    bool           mNeedsTranslation{false};
};

struct TranslatedMessage {
    TextPacketType           mType{TextPacketType::Translate};
    StringWrapper            mMessage{};
    std::vector<std::string> mParams{};
};

using PayloadVariant = std::variant<
    std::monostate,
    AuthorAndMessage,
    RawMessage,
    TranslatedMessage
>;

} // namespace TextPacketPayload

class TextPacket : public Packet {
public:
    struct BodyWrapper {
        TextPacketPayload::PayloadVariant mVariant{};
        [[nodiscard]] TextPacketPayload::PayloadVariant const& get() const noexcept { return mVariant; }
        [[nodiscard]] TextPacketPayload::PayloadVariant& get() noexcept { return mVariant; }
    };

    BodyWrapper mBody{};

    TextPacket() = default;
    ~TextPacket() override = default;

    [[nodiscard]] MinecraftPacketIds getId() const override {
        return MinecraftPacketIds::Text;
    }

    [[nodiscard]] ::std::string_view getName() const override {
        return "TextPacket";
    }

    static TextPacket createChatPacket(std::string author, std::string message) {
        TextPacket packet;
        TextPacketPayload::AuthorAndMessage payload;
        payload.mType = TextPacketType::Chat;
        payload.mAuthor.mStr = std::move(author);
        payload.mMessage.mStr = std::move(message);
        packet.mBody.mVariant = std::move(payload);
        return packet;
    }

    static TextPacket createRawPacket(std::string message) {
        TextPacket packet;
        TextPacketPayload::RawMessage payload;
        payload.mType = TextPacketType::Raw;
        payload.mMessage.mStr = std::move(message);
        packet.mBody.mVariant = std::move(payload);
        return packet;
    }
};
