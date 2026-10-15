#pragma once
#include <cstdint>
#include <string>

class PacketSender;
class Dimension;

class ILevel {
public:
    virtual ~ILevel() = default;

    virtual void* getUnknownBlockTypeRegistry() const { return nullptr; }
    virtual PacketSender* getPacketSender() const { return nullptr; }
};
