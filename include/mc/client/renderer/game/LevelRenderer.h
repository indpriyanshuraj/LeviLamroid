#pragma once
#include <memory>
#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/client/renderer/TextureGroup.h"
#include "mc/client/renderer/game/LevelRendererPlayer.h"
#include "mc/deps/minecraft_renderer/renderer/TexturePtr.h"

class LevelRenderer {
public:
    struct TextureGroupPtr {
        std::shared_ptr<mce::TextureGroup> mPtr{};
        [[nodiscard]] mce::TextureGroup* get() const noexcept { return mPtr.get(); }
        [[nodiscard]] mce::TextureGroup* operator->() const noexcept { return mPtr.get(); }
        explicit operator bool() const noexcept { return mPtr != nullptr; }
    };

    struct LevelRendererPlayerPtr {
        std::shared_ptr<LevelRendererPlayer> mPtr{};
        [[nodiscard]] LevelRendererPlayer* get() const noexcept { return mPtr.get(); }
        [[nodiscard]] LevelRendererPlayer* operator->() const noexcept { return mPtr.get(); }
        explicit operator bool() const noexcept { return mPtr != nullptr; }
    };

    struct AtlasTextureWrapper {
        mce::TexturePtr mTexture;
        mce::TexturePtr const& get() const { return mTexture; }
        mce::TexturePtr& get() { return mTexture; }
    };

    TextureGroupPtr        mTextureGroup{};
    LevelRendererPlayerPtr mLevelRendererPlayer{};
    AtlasTextureWrapper    mAtlasTexture{};

    virtual ~LevelRenderer() = default;
    virtual void renderLevel(ScreenContext& screenContext, void* renderView);
};
