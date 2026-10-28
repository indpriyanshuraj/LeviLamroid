#pragma once

#include "mc/_HeaderOutputPredefine.h"

// auto generated inclusion list
#include "mc/deps/nbt/CompoundTag.h"

struct ServerBlockProperty {
public:
    // member variables
    // NOLINTBEGIN
    ::ll::TypedStorage<8, 32, ::std::string> mName;
    ::ll::TypedStorage<8, 24, ::CompoundTag> mTag;
    // NOLINTEND

public:
    ServerBlockProperty() = default;
    ServerBlockProperty(::ServerBlockProperty const& other) = default;
    ServerBlockProperty(::ServerBlockProperty&& other) noexcept = default;
    ::ServerBlockProperty& operator=(::ServerBlockProperty const& other) = default;
    ::ServerBlockProperty& operator=(::ServerBlockProperty&& other) noexcept = default;
    ~ServerBlockProperty() = default;
};
