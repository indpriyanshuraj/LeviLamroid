#pragma once
#include <cstdint>
#include <string>
#include "mc/world/level/BlockSource.h"

class Level;

class Dimension {
public:
    int32_t     mId{0};
    std::string mName{"Overworld"};

    Dimension() = default;
    virtual ~Dimension() = default;

    [[nodiscard]] virtual int32_t getId() const noexcept;
    [[nodiscard]] virtual std::string const& getName() const noexcept;

    virtual BlockSource& getBlockSourceFromMainChunkSource() const;
};
