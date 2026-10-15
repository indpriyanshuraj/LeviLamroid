#pragma once
#include "mc/deps/core/math/Vec3.h"

class IVanillaRenderBlockActorComponent {
public:
    virtual ~IVanillaRenderBlockActorComponent() = default;
    [[nodiscard]] virtual bool isWithinRenderDistance(Vec3 const& camera) const;
    [[nodiscard]] virtual int getRendererId() const;
};
