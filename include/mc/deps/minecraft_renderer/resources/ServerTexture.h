#pragma once
#include <cstdint>
#include <string>
#include "mc/deps/core/resource/ResourceLocation.h"

namespace mce {

class ServerTexture {
public:
    ResourceLocation mResourceLocation{};
    uint32_t         mWidth{0};
    uint32_t         mHeight{0};
    uint32_t         mTextureId{0};

    ServerTexture() = default;
    virtual ~ServerTexture() = default;

    [[nodiscard]] ResourceLocation const& getResourceLocation() const noexcept { return mResourceLocation; }
    [[nodiscard]] uint32_t getWidth() const noexcept { return mWidth; }
    [[nodiscard]] uint32_t getHeight() const noexcept { return mHeight; }
    [[nodiscard]] uint32_t getTextureId() const noexcept { return mTextureId; }
};

} // namespace mce
