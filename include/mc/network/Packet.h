#pragma once
#include <string>
#include <cstdint>
#include "mc/network/MinecraftPacketIds.h"

class Packet {
public:
    uint32_t mSenderSubClientId{0};
    bool     mIsReliable{true};

    virtual ~Packet() = default;

    [[nodiscard]] virtual MinecraftPacketIds getId() const = 0;
    [[nodiscard]] virtual std::string getName() const = 0;
    virtual void write(void* stream) const { (void)stream; }
    virtual int read(void* stream) { (void)stream; return 0; }
    [[nodiscard]] virtual bool disallowBatching() const { return false; }
    [[nodiscard]] virtual bool isValid() const { return true; }
};
