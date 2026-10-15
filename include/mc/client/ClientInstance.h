#pragma once
#include "mc/world/actor/player/LocalPlayer.h"
#include "mc/network/PacketSender.h"
class Level;
class LevelRenderer;

class IClientInstance {
public:
    virtual ~IClientInstance() = default;
    virtual LocalPlayer* getLocalPlayer() const = 0;
    virtual bool isShowingMenu() const = 0;
    [[nodiscard]] virtual bool isInGameInputEnabled() const noexcept = 0;
    virtual LevelRenderer* getLevelRenderer() const = 0;
    virtual PacketSender& getPacketSender() const = 0;
    virtual Level* getLevel() const = 0;
};

class ClientInstance : public IClientInstance {
public:
    virtual ~ClientInstance() override = default;

    // Slot 32 on Android ARM64 Bedrock
    virtual LocalPlayer* getLocalPlayer() const override { return nullptr; }
    virtual bool isShowingMenu() const override { return false; }
    [[nodiscard]] virtual bool isInGameInputEnabled() const noexcept override { return true; }

    virtual LevelRenderer* getLevelRenderer() const override { return nullptr; }
    virtual PacketSender& getPacketSender() const override {
        class DummySender : public PacketSender {
        public:
            void send(Packet&) override {}
            void sendToServer(Packet&) override {}
            void sendToClient(void const*, Packet const&, unsigned char) override {}
            void sendToClients(void const*, Packet const&) override {}
            void sendBroadcast(Packet const&) override {}
            void flush(void const*) override {}
        };
        static DummySender sDummy;
        return sDummy;
    }
    virtual Level* getLevel() const override { return nullptr; }
};
