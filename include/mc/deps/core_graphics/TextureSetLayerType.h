#pragma once
#include <cstdint>

namespace cg {

enum class TextureSetLayerType : uint32_t {
    Color      = 0, // Albedo color diffuse layer
    Normal     = 1, // Tangent space normal map
    Heightmap  = 2, // Parallax height displacement
    MER        = 3  // Metallic, Emissive, Roughness PBR map
};

} // namespace cg
