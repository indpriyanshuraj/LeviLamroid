#pragma once
#include <memory>
#include <unordered_map>
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/core/renderer/RenderMaterialInfo.h"
#include "mc/deps/minecraft_renderer/renderer/MaterialPtr.h"

namespace mce {

class RenderMaterialGroup {
public:
    class MaterialMapWrapper {
    public:
        std::unordered_map<HashedString, std::shared_ptr<RenderMaterialInfo>> mMap{};

        [[nodiscard]] auto begin() const noexcept { return mMap.begin(); }
        [[nodiscard]] auto end() const noexcept { return mMap.end(); }
        [[nodiscard]] auto begin() noexcept { return mMap.begin(); }
        [[nodiscard]] auto end() noexcept { return mMap.end(); }
        [[nodiscard]] auto const& get() const noexcept { return mMap; }
        [[nodiscard]] auto& get() noexcept { return mMap; }
    };

    MaterialMapWrapper mMaterials{};

    static RenderMaterialGroup& common() {
        static RenderMaterialGroup sGroup;
        return sGroup;
    }
    static RenderMaterialGroup& switchable() {
        static RenderMaterialGroup sGroup;
        return sGroup;
    }
};

} // namespace mce
