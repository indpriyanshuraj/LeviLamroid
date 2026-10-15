#pragma once
#include <cstdint>

enum class ContainerID : int8_t {
    Invalid = -1,
    Inventory = 0,
    First = 1,
    Last = 100,
    Offhand = 119,
    Armor = 120,
    Creative = 121,
    Hotbar = 122,
    FixedInventory = 123,
    UI = 124,
    Container = 125
};
