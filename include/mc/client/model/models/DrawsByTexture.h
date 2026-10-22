#pragma once

#include "mc/_HeaderOutputPredefine.h"

// auto generated forward declare list
// clang-format off
class MeshRenderData;
namespace mce { struct ClientTexture; }
// clang-format on

class DrawsByTexture {
public:
    // member variables
    // NOLINTBEGIN
    ::ll::TypedStorage<8, 8, ::mce::ClientTexture const*>      mTexture;
    ::ll::TypedStorage<8, 24, ::std::vector<::MeshRenderData>> mMeshes;
    ::ll::TypedStorage<8, 24, ::std::vector<::DrawsByTexture>> mDrawsByTexture;
    // NOLINTEND
};
