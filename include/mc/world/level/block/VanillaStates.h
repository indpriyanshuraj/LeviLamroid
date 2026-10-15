#pragma once
#include <string>

template <typename T>
struct VanillaBlockState {
    using Type = T;
    std::string name;

    VanillaBlockState(const char* n) : name(n) {}
    operator std::string const&() const { return name; }
    bool operator==(VanillaBlockState const& o) const { return name == o.name; }
};

class VanillaStates {
public:
    static VanillaBlockState<bool> const& UpperBlockBit() {
        static VanillaBlockState<bool> const sVal{"upper_block_bit"};
        return sVal;
    }
    static VanillaBlockState<bool> const& DoorHingeBit() {
        static VanillaBlockState<bool> const sVal{"door_hinge_bit"};
        return sVal;
    }
    static VanillaBlockState<int> const& Direction() {
        static VanillaBlockState<int> const sVal{"direction"};
        return sVal;
    }
    static VanillaBlockState<bool> const& OpenBit() {
        static VanillaBlockState<bool> const sVal{"open_bit"};
        return sVal;
    }
    static VanillaBlockState<bool> const& UpsideDownBit() {
        static VanillaBlockState<bool> const sVal{"upside_down_bit"};
        return sVal;
    }
    static VanillaBlockState<int> const& FacingDirection() {
        static VanillaBlockState<int> const sVal{"facing_direction"};
        return sVal;
    }
};
