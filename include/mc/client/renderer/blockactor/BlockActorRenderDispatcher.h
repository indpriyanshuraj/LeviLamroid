#pragma once
#include "mc/deps/minecraft_renderer/renderer/MaterialPtr.h"
#include "mc/world/level/block/actor/component/IVanillaRenderBlockActorComponent.h"
#include <optional>

class BaseActorRenderContext;
class BlockSource;
class Block;

class BlockActorRenderDispatcher {
public:
    struct RenderersWrapper {
        void* dummy[512];
        RenderersWrapper() {
            for (auto& p : dummy) p = reinterpret_cast<void*>(1);
        }
        void* const* get() const { return dummy; }
        void** get() { return dummy; }
    };
    RenderersWrapper mRenderers;

    virtual ~BlockActorRenderDispatcher() = default;

    virtual void render(
        BaseActorRenderContext& renderContext,
        BlockSource& region,
        IVanillaRenderBlockActorComponent& renderComponent,
        Block const& block,
        bool renderAlphaLayer,
        mce::MaterialPtr const& forcedMaterial,
        void const* unk1 = nullptr,
        int unk2 = -1,
        std::optional<int> unk3 = std::nullopt
    ) {
        (void)renderContext;
        (void)region;
        (void)renderComponent;
        (void)block;
        (void)renderAlphaLayer;
        (void)forcedMaterial;
        (void)unk1;
        (void)unk2;
        (void)unk3;
    }
};
