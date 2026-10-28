#pragma once

#include "mc/_HeaderOutputPredefine.h"

// auto generated inclusion list
#include "mc/deps/core/utility/NonOwnerPointer.h"

// auto generated forward declare list
// clang-format off
class CreativeItemRegistry;
struct CreativeGroupInfoPayload;
struct CreativeItemEntryPayload;
// clang-format on

struct CreativeContentPacketPayload {
public:
    // member variables
    // NOLINTBEGIN
    ::ll::TypedStorage<8, 24, ::std::vector<::CreativeGroupInfoPayload>> mGroups;
    ::ll::TypedStorage<8, 24, ::std::vector<::CreativeItemEntryPayload>> mEntries;
    // NOLINTEND

public:
    CreativeContentPacketPayload() = default;
    ~CreativeContentPacketPayload() = default;
    CreativeContentPacketPayload(CreativeContentPacketPayload const&) = default;
    CreativeContentPacketPayload(CreativeContentPacketPayload&&) = default;
    CreativeContentPacketPayload& operator=(CreativeContentPacketPayload const&) = default;
    CreativeContentPacketPayload& operator=(CreativeContentPacketPayload&&) = default;

public:
    // member functions
    // NOLINTBEGIN
    MCAPI explicit CreativeContentPacketPayload(
        ::Bedrock::NonOwnerPointer<::CreativeItemRegistry const> creativeItemRegistryForWrite
    );
    // NOLINTEND

public:
    // constructor thunks
    // NOLINTBEGIN
    MCAPI void* $ctor(::Bedrock::NonOwnerPointer<::CreativeItemRegistry const> creativeItemRegistryForWrite);
    // NOLINTEND
};
