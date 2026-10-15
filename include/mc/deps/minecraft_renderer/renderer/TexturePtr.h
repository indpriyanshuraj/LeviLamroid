#pragma once
#include <memory>
#include "mc/deps/minecraft_renderer/renderer/BedrockTextureData.h"
#include "mc/deps/minecraft_renderer/resources/ClientTexture.h"

namespace mce {

class TexturePtr {
public:
    std::shared_ptr<ClientTexture>      mClientTexture{};
    std::shared_ptr<BedrockTextureData> mData{};

    TexturePtr() = default;
    explicit TexturePtr(std::shared_ptr<ClientTexture> clientTex)
        : mClientTexture(std::move(clientTex)) {}
    explicit TexturePtr(std::shared_ptr<BedrockTextureData> data)
        : mData(std::move(data)) {}

    [[nodiscard]] bool isValid() const noexcept {
        return mClientTexture != nullptr || mData != nullptr;
    }
    explicit operator bool() const noexcept { return isValid(); }
};

} // namespace mce
