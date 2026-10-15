#pragma once
#include <memory>
#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/client/renderer/blockactor/BlockActorRenderDispatcher.h"
#include "mc/client/renderer/game/ItemInHandRenderer.h"
#include "mc/client/ClientInstance.h"

class BaseActorRenderContext {
public:
    ScreenContext& mScreenContext;
    BlockActorRenderDispatcher& mBlockEntityRenderDispatcher;
    ItemInHandRenderer& mItemInHandRenderer;
    ClientInstance& mClientInstance;
    std::shared_ptr<void> mImpl{};

    BaseActorRenderContext(
        ScreenContext& screenContext,
        BlockActorRenderDispatcher& dispatcher,
        ItemInHandRenderer& itemRenderer,
        ClientInstance& client
    ) : mScreenContext(screenContext),
        mBlockEntityRenderDispatcher(dispatcher),
        mItemInHandRenderer(itemRenderer),
        mClientInstance(client) {}
};
