#pragma once
#include <memory>
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/deps/minecraft_renderer/renderer/MaterialPtr.h"
#include "mc/world/level/BlockPos.h"

class BlockSource;

class LevelRendererPlayer {
public:
    struct MaterialWrapper {
        mce::MaterialPtr mMaterial{};
        [[nodiscard]] mce::MaterialPtr const& get() const noexcept { return mMaterial; }
        [[nodiscard]] mce::MaterialPtr& get() noexcept { return mMaterial; }
        [[nodiscard]] mce::MaterialPtr const* operator->() const noexcept { return &mMaterial; }
        [[nodiscard]] mce::MaterialPtr* operator->() noexcept { return &mMaterial; }
    };

    MaterialWrapper mOutlineSelectionMaterial{};
    MaterialWrapper selectionBlockEntityOverlayColorMaterial{};

    virtual ~LevelRendererPlayer() = default;

    virtual void renderHitSelect(
        BaseActorRenderContext& renderContext,
        BlockSource& region,
        BlockPos const& pos,
        bool fancyGraphics
    );

    virtual void renderBlockEntities(
        BaseActorRenderContext& renderContext,
        bool renderAlphaLayer
    );

    // Alias for symbol name generator / hook macros
    virtual void $renderBlockEntities(
        BaseActorRenderContext& renderContext,
        bool renderAlphaLayer
    );

    virtual void preRenderUpdate();
    virtual void tickClouds();
};
