#pragma once

#include "mc/_HeaderOutputPredefine.h"

// auto generated inclusion list
#include "mc/deps/shared_types/item/CreativeItemCategory.h"
#include "mc/world/item/NetworkItemInstanceDescriptor.h"

struct CreativeGroupInfoPayload {
public:
    // member variables
    // NOLINTBEGIN
    ::ll::TypedStorage<1, 1, ::SharedTypes::CreativeItemCategory> mCreativeItemCategory;
    ::ll::TypedStorage<8, 32, ::std::string>                      mName;
    ::ll::TypedStorage<8, 64, ::NetworkItemInstanceDescriptor>    mIcon;
    // NOLINTEND

public:
    CreativeGroupInfoPayload() = default;
    ~CreativeGroupInfoPayload() = default;
    CreativeGroupInfoPayload(CreativeGroupInfoPayload const&) = default;
    CreativeGroupInfoPayload(CreativeGroupInfoPayload&&) = default;
    CreativeGroupInfoPayload& operator=(CreativeGroupInfoPayload const&) = default;
    CreativeGroupInfoPayload& operator=(CreativeGroupInfoPayload&&) = default;
};
