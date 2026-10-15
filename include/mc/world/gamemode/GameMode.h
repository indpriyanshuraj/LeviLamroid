#pragma once
#include "mc/world/item/ItemStack.h"
#include "mc/world/level/BlockPos.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/actor/Actor.h"

#include "mc/world/item/HandSlot.h"

class GameMode {
public:
    Player& mPlayer;

    GameMode() : mPlayer(*static_cast<Player*>(nullptr)) {}
    explicit GameMode(Player& player) : mPlayer(player) {}
    ~GameMode() = default;

    bool useItem(ItemStack& item, HandSlot handSlot = HandSlot::MainHand) {
        auto vtable = *reinterpret_cast<uintptr_t const* const*>(this);
        using Fn = bool (*)(GameMode*, ItemStack&, HandSlot);
        return reinterpret_cast<Fn>(vtable[13])(this, item, handSlot);
    }
    void stopBuildBlock() {}
    bool buildBlock(BlockPos const& pos, unsigned char face, HandSlot handSlot = HandSlot::MainHand, bool isSimTick = false) {
        auto vtable = *reinterpret_cast<uintptr_t const* const*>(this);
        using Fn = bool (*)(GameMode*, BlockPos const&, unsigned char, HandSlot, bool);
        return reinterpret_cast<Fn>(vtable[12])(this, pos, face, handSlot, isSimTick);
    }
    void startBuildBlock(BlockPos const& pos, unsigned char face) { (void)pos; (void)face; }

    // Aliases for Hook macros
    bool $useItem(ItemStack& item, HandSlot handSlot = HandSlot::MainHand) { return useItem(item, handSlot); }
    void $stopBuildBlock() { stopBuildBlock(); }
    bool $buildBlock(BlockPos const& pos, unsigned char face, HandSlot handSlot = HandSlot::MainHand, bool isSimTick = false) { return buildBlock(pos, face, handSlot, isSimTick); }
    void $startBuildBlock(BlockPos const& pos, unsigned char face) { startBuildBlock(pos, face); }

    bool useItemOn(
        ItemStack& item,
        BlockPos const& pos,
        uint8_t face,
        Vec3 const& hitPos,
        void const* block
    );
    bool attack(Player& player, Actor& target);
};
