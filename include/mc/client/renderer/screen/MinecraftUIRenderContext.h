#pragma once
#include <memory>
#include <string>
#include <glm/vec2.hpp>
#include "mc/deps/core/math/Color.h"
#include "mc/client/gui/screens/ScreenContext.h"
#include "mc/client/renderer/TextureGroup.h"

struct RectangleArea {
    float _x0{0.0f}, _x1{0.0f}, _y0{0.0f}, _y1{0.0f};
};

struct HashedString {
    std::string str;
    explicit HashedString(const char* s) : str(s) {}
    explicit HashedString(const std::string& s) : str(s) {}
};

class MinecraftUIRenderContext {
public:
    virtual ~MinecraftUIRenderContext() = default;

    void* mClientInstance{nullptr};
    ScreenContext* mScreenContext{nullptr};
    std::shared_ptr<mce::TextureGroup> mTextureGroup{nullptr};

    ScreenContext* getScreenContext() {
        if (mScreenContext) return mScreenContext;
        auto raw = *reinterpret_cast<ScreenContext**>(reinterpret_cast<uintptr_t>(this) + 16);
        return raw;
    }

    void fillRectangle(const RectangleArea& /*area*/, mce::Color /*color*/, float /*alpha*/) {}
    void drawImage(void* /*texture*/, glm::vec2 /*pos*/, glm::vec2 /*size*/, glm::vec2 /*uvPos*/, glm::vec2 /*uvSize*/, bool /*flush*/) {}
    void flushImages(mce::Color /*color*/, float /*alpha*/, const HashedString& /*material*/) {}
    mce::TexturePtr getTexture(const ResourceLocation& loc, bool fallback) {
        if (mTextureGroup) {
            return mTextureGroup->getTexture(loc, fallback);
        }
        return {};
    }
};

using UIRenderContext = MinecraftUIRenderContext;
