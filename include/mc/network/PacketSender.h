#pragma once
#include <cstdint>
#include "mc/network/Packet.h"

class PacketSender {
public:
    virtual ~PacketSender() = default;

    virtual void send(Packet& packet) = 0;
    virtual void sendToServer(Packet& packet) = 0;
    virtual void sendToClient(void const* networkIdentifier, Packet const& packet, unsigned char subClientId) = 0;
    virtual void sendToClients(void const* list, Packet const& packet) = 0;
    virtual void sendBroadcast(Packet const& packet) = 0;
    virtual void flush(void const* networkIdentifier) = 0;
};
