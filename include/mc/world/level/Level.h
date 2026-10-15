#pragma once
#include "mc/world/level/ILevel.h"
#include "mc/world/level/LevelListener.h"
#include "mc/network/PacketSender.h"

class Level : public ILevel {
public:
    ~Level() override = default;

    void* getUnknownBlockTypeRegistry() const override { return nullptr; }
    PacketSender* getPacketSender() const override { return nullptr; }

    virtual void addListener(LevelListener& listener) { (void)listener; }
    virtual void removeListener(LevelListener& listener) { (void)listener; }
};
