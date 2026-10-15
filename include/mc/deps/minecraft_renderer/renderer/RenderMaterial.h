#pragma once
#include "mc/deps/renderer/hal/interface/DepthStencilStateDescription.h"
#include "mc/deps/minecraft_renderer/renderer/PrimitiveMode.h"
#include <string>
#include <memory>

namespace mce {

struct BlendStateDescription {
    bool mBlendEnable{false};
    uint32_t mSrcBlend{0};
    uint32_t mDestBlend{0};
    uint32_t mBlendOp{0};
};

class RenderMaterial {
public:
    struct DescriptionWrapper {
        DepthStencilStateDescription mDesc;
        DepthStencilStateDescription& get() { return mDesc; }
        DepthStencilStateDescription const& get() const { return mDesc; }
    };

    struct BlendDescriptionWrapper {
        BlendStateDescription mDesc;
        BlendStateDescription& get() { return mDesc; }
        BlendStateDescription const& get() const { return mDesc; }
    };

    DescriptionWrapper depthStencilStateDescription;
    BlendDescriptionWrapper blendStateDescription;
    float mDepthBias{0.0f};
    float mSlopeScaledDepthBias{0.0f};
    mce::PrimitiveMode mPrimitiveMode{mce::PrimitiveMode::Triangles};
};

} // namespace mce
