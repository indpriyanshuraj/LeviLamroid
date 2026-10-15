#pragma once
#include <memory>
#include <optional>
#include "mc/deps/core/resource/ResourceLocation.h"
#include "mc/deps/core_graphics/TextureSetLayerType.h"
#include "mc/deps/minecraft_renderer/renderer/BedrockTextureData.h"
#include "mc/deps/minecraft_renderer/renderer/TexturePtr.h"
#include "mc/deps/minecraft_renderer/resources/ClientTexture.h"

namespace mce {

class TextureGroup {
public:
    virtual ~TextureGroup() = default;
    virtual TexturePtr getTexture(ResourceLocation const& location, bool loadImmediately = false);
    virtual TexturePtr getTexture(
        ResourceLocation const& location,
        bool loadImmediately,
        std::optional<uint32_t> const& type,
        cg::TextureSetLayerType layer = cg::TextureSetLayerType::Color
    );
    virtual void unloadTexture(ResourceLocation const& location);
};

} // namespace mce

