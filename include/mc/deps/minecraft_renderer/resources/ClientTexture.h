#pragma once
#include <cstdint>
#include <string>
#include "mc/deps/core/resource/ResourceLocation.h"
#include "mc/deps/minecraft_renderer/renderer/IsMissingTexture.h"

namespace mce {

class ClientTexture {
public:
    ResourceLocation mResourceLocation{};
    IsMissingTexture mIsMissingTexture{IsMissingTexture::No};
    uint32_t         mWidth{0};
    uint32_t         mHeight{0};
    uint32_t         mTextureId{0};
    uint32_t         mTextureType{0};
    bool             mHasAlpha{true};

    ClientTexture() = default;
    virtual ~ClientTexture() = default;

    [[nodiscard]] ResourceLocation const& getResourceLocation() const noexcept { return mResourceLocation; }
    [[nodiscard]] uint32_t getWidth() const noexcept { return mWidth; }
    [[nodiscard]] uint32_t getHeight() const noexcept { return mHeight; }
    [[nodiscard]] uint32_t getTextureId() const noexcept { return mTextureId; }
    [[nodiscard]] bool isMissing() const noexcept { return mIsMissingTexture == IsMissingTexture::Yes; }
};

} // namespace mce
