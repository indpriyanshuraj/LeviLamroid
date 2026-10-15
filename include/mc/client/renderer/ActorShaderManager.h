#pragma once
#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/world/actor/Actor.h"
#include "mc/deps/core/math/Color.h"
#include "mc/common/Brightness.h"
#include <optional>

class ActorShaderManager {
public:
    static void setupShaderParameters(
        ScreenContext& screenContext,
        BaseActorRenderContext& renderContext,
        Actor const& actor,
        mce::Color const& overlayColor,
        float changeColor,
        Brightness const& brightness,
        std::optional<int> unk = std::nullopt
    ) {
        (void)screenContext;
        (void)renderContext;
        (void)actor;
        (void)overlayColor;
        (void)changeColor;
        (void)brightness;
        (void)unk;
    }
};
