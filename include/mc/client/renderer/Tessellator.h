#pragma once
#include <cstddef>
#include <cstdint>
#include "mc/deps/core/math/Vec3.h"

#include <string>
#include <string_view>
#include <glm/glm.hpp>
#include "mc/client/renderer/SupplementaryFieldAutoGenerationMode.h"
#include "mc/deps/minecraft_renderer/renderer/Mesh.h"

class Tessellator {
public:
    struct DebugContextCallback {};
    enum class UploadMode {
        Buffered,
        Immediate,
        Never
    };

    std::weak_ptr<void> mBufferResourceService;
    std::shared_ptr<mce::MeshData> mMeshData{std::make_shared<mce::MeshData>()};

    Tessellator(std::weak_ptr<void> bufferService = {})
        : mBufferResourceService(bufferService),
          mMeshData(std::make_shared<mce::MeshData>()) {}

    static Tessellator& getInstance();

    void begin(int mode, size_t maxVertices = 512) {
        (void)mode; (void)maxVertices;
        if (!mMeshData) mMeshData = std::make_shared<mce::MeshData>();
        mMeshData->mPositions.mVector.clear();
        mMeshData->mColors.mVector.clear();
    }
    void begin(DebugContextCallback cb, mce::PrimitiveMode mode, size_t maxVertices, bool b = false) {
        (void)cb; (void)mode; (void)maxVertices; (void)b;
        if (!mMeshData) mMeshData = std::make_shared<mce::MeshData>();
        mMeshData->mPositions.mVector.clear();
        mMeshData->mColors.mVector.clear();
    }
    void vertex(float x, float y, float z) {
        if (!mMeshData) mMeshData = std::make_shared<mce::MeshData>();
        mMeshData->mPositions.mVector.emplace_back(x, y, z);
    }
    void color(float r, float g, float b, float a) { (void)r; (void)g; (void)b; (void)a; }
    void color(uint32_t abgr) {
        if (!mMeshData) mMeshData = std::make_shared<mce::MeshData>();
        mMeshData->mColors.mVector.push_back(abgr);
    }
    void tex(float u, float v) { (void)u; (void)v; }
    void tex2(glm::vec2 uv) { (void)uv; }
    void end() {}
    mce::Mesh end(UploadMode mode, std::string_view name, SupplementaryFieldAutoGenerationMode fieldMode) {
        (void)mode; (void)name; (void)fieldMode;
        mce::Mesh m;
        if (mMeshData) {
            m.mVertexCount.mCount = static_cast<uint32_t>(mMeshData->mPositions.mVector.size());
            m.mMeshData.mData = *mMeshData;
        } else {
            m.mVertexCount.mCount = 24;
        }
        return m;
    }
};

class MaterialPtr {
public:
    static MaterialPtr createMaterial(const char* materialName);
};
