#pragma once
#include "mc/network/PacketSender.h"

class LoopbackPacketSender : public PacketSender {
public:
    uint8_t mSubClientId{0};
    void*   mNetworkSystem{nullptr};

    LoopbackPacketSender() = default;
    ~LoopbackPacketSender() override = default;

    void send(Packet& packet) override;
    void sendToServer(Packet& packet) override;
    void sendToClient(void const* networkIdentifier, Packet const& packet, unsigned char subClientId) override;
    void sendToClients(void const* list, Packet const& packet) override;
    void sendBroadcast(Packet const& packet) override;
    void flush(void const* networkIdentifier) override;
};
