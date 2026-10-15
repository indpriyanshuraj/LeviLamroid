#pragma once
#include "mc/deps/core/renderer/RenderMaterialInfo.h"
#include <memory>
#include <string_view>

namespace mce {

class MaterialPtr {
public:
    std::shared_ptr<RenderMaterialInfo> mRenderMaterialInfoPtr;

    MaterialPtr() = default;
    explicit MaterialPtr(std::shared_ptr<RenderMaterialInfo> ptr) : mRenderMaterialInfoPtr(std::move(ptr)) {}

    [[nodiscard]] bool isValid() const { return mRenderMaterialInfoPtr != nullptr; }
    explicit operator bool() const { return isValid(); }

    static MaterialPtr createMaterial(std::string_view name) {
        (void)name;
        auto info = std::make_shared<RenderMaterialInfo>();
        info->mPtr = std::make_shared<RenderMaterial>();
        return MaterialPtr(info);
    }
};

} // namespace mce
