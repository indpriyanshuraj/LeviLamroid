#pragma once
#include "mc/deps/minecraft_renderer/renderer/RenderMaterial.h"
#include <memory>

namespace mce {

struct RenderMaterialInfo {
    std::shared_ptr<RenderMaterial> mPtr;
};

} // namespace mce
