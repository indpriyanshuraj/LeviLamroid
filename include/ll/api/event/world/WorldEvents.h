#pragma once
#include "ll/api/event/Event.h"
#include "mc/world/level/BlockPos.h"

class Level;
class BlockSource;
class Player;

namespace ll::event {

class LevelTickEvent : public Event {
private:
    Level* mLevel;

public:
    explicit LevelTickEvent(Level* level) : mLevel(level) {}

    Level* getLevel() const { return mLevel; }
};

class BlockBreakEvent : public Event {
private:
    BlockSource* mBlockSource;
    BlockPos mPos;
    Player* mPlayer;

public:
    BlockBreakEvent(BlockSource* region, const BlockPos& pos, Player* player)
        : mBlockSource(region), mPos(pos), mPlayer(player) {}

    BlockSource* getBlockSource() const { return mBlockSource; }
    const BlockPos& getPos() const { return mPos; }
    Player* getPlayer() const { return mPlayer; }
};

class BlockPlaceEvent : public Event {
private:
    BlockSource* mBlockSource;
    BlockPos mPos;
    Player* mPlayer;
    const void* mBlock;

public:
    BlockPlaceEvent(BlockSource* region, const BlockPos& pos, Player* player, const void* block)
        : mBlockSource(region), mPos(pos), mPlayer(player), mBlock(block) {}

    BlockSource* getBlockSource() const { return mBlockSource; }
    const BlockPos& getPos() const { return mPos; }
    Player* getPlayer() const { return mPlayer; }
    const void* getBlock() const { return mBlock; }
};

} // namespace ll::event

