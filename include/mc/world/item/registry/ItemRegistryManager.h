#pragma once
#include "mc/world/item/registry/ItemRegistry.h"

class ItemRegistryManager {
public:
    static ItemRegistry& getItemRegistry() {
        static ItemRegistry sRegistry{};
        return sRegistry;
    }
};
