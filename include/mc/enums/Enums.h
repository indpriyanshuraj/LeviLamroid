#pragma once
#include <cstdint>

enum class EquipmentSlot : int {
    None = -1,
    Head = 0,
    Torso = 1,
    Legs = 2,
    Feet = 3,
    Offhand = 4
};

#include "mc/world/ContainerID.h"
#include "mc/world/item/HandSlot.h"
#include "mc/client/renderer/texture/TextureEnums.h"

enum class ActorDamageCause : int {
    None = -1,
    Override = 0,
    Contact = 1,
    EntityAttack = 2,
    Projectile = 3,
    Suffocation = 4,
    Fall = 5,
    Fire = 6,
    FireTick = 7,
    Lava = 8,
    Drowning = 9,
    BlockExplosion = 10,
    EntityExplosion = 11,
    Void = 12,
    Suicide = 13,
    Magic = 14,
    Wither = 15,
    Starve = 16,
    Anvil = 17,
    Thorns = 18,
    FallingBlock = 19,
    Piston = 20,
    FlyIntoWall = 21,
    Magma = 22,
    Fireworks = 23,
    Lightning = 24,
    Charging = 25,
    Temperature = 26,
    Freezing = 27,
    SonicBoom = 28
};

enum class ButtonBehavior : int {
    Click = 0,
    Toggle = 1,
    Hold = 2
};

enum class ButtonEvent : int {
    Down = 0,
    Up = 1,
    Click = 2,
    Drag = 3
};

enum class ButtonStylePreset : int {
    Default = 0,
    Glass = 1,
    Neon = 2,
    Minimal = 3
};

enum class ButtonIconFormat : int {
    None = 0,
    RGBA = 1,
    PNG = 2,
    WebP = 3
};
