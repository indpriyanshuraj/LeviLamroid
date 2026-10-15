#pragma once

class ScreenContext;
#include "mc/deps/minecraft_renderer/renderer/MaterialPtr.h"
#include "mc/deps/minecraft_renderer/renderer/BedrockTextureData.h"
#include "mc/deps/minecraft_renderer/framebuilder/dragon/RenderMetadata.h"
#include <cstdint>
#include <optional>
#include <vector>
#include <array>
#include <memory>
#include <string_view>
#include <glm/glm.hpp>
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/minecraft_renderer/renderer/PrimitiveMode.h"

namespace mce {

template <typename T>
struct MeshFieldWrapper {
    std::vector<T> mVector;
    std::vector<T>& get() { return mVector; }
    std::vector<T> const& get() const { return mVector; }
};

struct MeshData {
    MeshFieldWrapper<Vec3> mPositions;
    MeshFieldWrapper<Vec3> mNormals;
    MeshFieldWrapper<Vec3> mTangents;
    MeshFieldWrapper<uint32_t> mColors;
    MeshFieldWrapper<uint32_t> mBoneId0s;
    MeshFieldWrapper<uint32_t> mPBRTextureIndices;
    MeshFieldWrapper<uint32_t> mMERS;
    MeshFieldWrapper<uint32_t> mGeoType;
    std::array<MeshFieldWrapper<glm::vec2>, 4> mTextureUVs;

    [[nodiscard]] size_t size() const noexcept {
        return mPositions.mVector.size();
    }
};

struct MeshDataWrapper {
    MeshData mData;
    MeshData& get() { return mData; }
    MeshData const& get() const { return mData; }
};

class Mesh {
public:
    struct VertexCountWrapper {
        std::optional<uint32_t> mCount;
        std::optional<uint32_t> const& get() const { return mCount; }
    };

    VertexCountWrapper mVertexCount;
    MeshDataWrapper    mMeshData;
    uint32_t           mIndexCount{0};
    uint32_t           mPrimitiveType{0};

    Mesh() = default;
    Mesh(std::shared_ptr<void> bufferService, MeshData data, bool dynamic = false, std::string_view name = "") {
        (void)bufferService;
        (void)dynamic;
        (void)name;
        mVertexCount.mCount = static_cast<uint32_t>(data.size());
        mMeshData.mData = std::move(data);
    }
    virtual ~Mesh() = default;

    void reset() {
        mVertexCount.mCount = std::nullopt;
        mIndexCount = 0;
    }

    [[nodiscard]] bool isValid() const noexcept {
        return mVertexCount.mCount.has_value() && *mVertexCount.mCount > 0;
    }

    template <typename TextureT = BedrockTextureData>
    void renderMesh(
        ScreenContext& screenContext,
        MaterialPtr const& material,
        TextureT const& texture,
        uint32_t offset,
        uint32_t count,
        OffscreenCaptureDescription const& offscreen,
        void* unk = nullptr
    ) {
        (void)screenContext;
        (void)material;
        (void)texture;
        (void)offset;
        (void)count;
        (void)offscreen;
        (void)unk;
    }
};

} // namespace mce
