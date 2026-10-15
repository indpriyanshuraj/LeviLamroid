#pragma once
#include "mc/deps/core/string/HashedString.h"

class VanillaBlockTypeIds {
public:
    static HashedString const& Air() {
        static HashedString const sId{"minecraft:air"};
        return sId;
    }
    static HashedString const& PistonArmCollision() {
        static HashedString const sId{"minecraft:piston_arm_collision"};
        return sId;
    }
    static HashedString const& StickyPistonArmCollision() {
        static HashedString const sId{"minecraft:sticky_piston_arm_collision"};
        return sId;
    }
    static HashedString const& Water() {
        static HashedString const sId{"minecraft:water"};
        return sId;
    }
    static HashedString const& FlowingWater() {
        static HashedString const sId{"minecraft:flowing_water"};
        return sId;
    }
    static HashedString const& BubbleColumn() {
        static HashedString const sId{"minecraft:bubble_column"};
        return sId;
    }
};
