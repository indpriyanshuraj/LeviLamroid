#pragma once
#include "ll/api/event/Event.h"
#if __has_include("mc/deps/core/math/Vec3.h")
#include "mc/deps/core/math/Vec3.h"
#else
struct Vec3 { float x, y, z; };
#endif

class Player;
class LocalPlayer;

namespace ll::event {

class PlayerMoveEvent : public Event {
private:
    Player* mPlayer;
    Vec3 mDelta;

public:
    PlayerMoveEvent(Player* player, const Vec3& delta)
        : mPlayer(player), mDelta(delta) {}

    Player* getPlayer() const { return mPlayer; }
    const Vec3& getDelta() const { return mDelta; }
    void setDelta(const Vec3& delta) { mDelta = delta; }
};

class PlayerTickEvent : public Event {
private:
    Player* mPlayer;

public:
    explicit PlayerTickEvent(Player* player) : mPlayer(player) {}

    Player* getPlayer() const { return mPlayer; }
};

class PlayerRespawnEvent : public Event {
private:
    Player* mPlayer;

public:
    explicit PlayerRespawnEvent(Player* player) : mPlayer(player) {}

    Player* getPlayer() const { return mPlayer; }
};

class PlayerUseItemEvent : public Event {
private:
    Player* mPlayer;
    void* mItemStack;

public:
    PlayerUseItemEvent(Player* player, void* itemStack)
        : mPlayer(player), mItemStack(itemStack) {}

    Player* getPlayer() const { return mPlayer; }
    void* getItemStack() const { return mItemStack; }
};

} // namespace ll::event
