#pragma once
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/chunk/ChunkViewSource.h"
#include "mc/world/level/Tick.h"

class LocalPlayer : public Player {
public:
    virtual ~LocalPlayer() override = default;

    virtual void move(const Vec3& delta);
    virtual void tickWorld(Tick const& currentTick);
    virtual void $tickWorld(Tick const& currentTick);
    virtual std::shared_ptr<ChunkViewSource> $_createChunkSource(ChunkSource& chunkSource);

    void setSelectedSlot(int slot) noexcept {
        (void)slot;
    }

    [[nodiscard]] Vec3 getEyePos() const noexcept {
        return Vec3{0, 0, 0};
    }

    [[nodiscard]] Vec3 getViewVector(float a = 1.0f) const noexcept {
        (void)a;
        return Vec3{0, 0, 1};
    }

    [[nodiscard]] float getPickRange() const noexcept {
        return 5.0f;
    }

    [[nodiscard]] BlockPos getFeetBlockPos() const noexcept {
        return BlockPos{static_cast<int>(mPosition.x), static_cast<int>(mPosition.y), static_cast<int>(mPosition.z)};
    }

    [[nodiscard]] std::string getLocaleCode() const {
        return "en_US";
    }
};
