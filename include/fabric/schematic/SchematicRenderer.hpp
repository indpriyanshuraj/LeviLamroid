#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "fabric/render/Tessellator.hpp"

namespace fabric::schematic {

struct GhostBlock {
    int x;
    int y;
    int z;
    std::string blockIdentifier;
};

class SchematicRendererEngine {
public:
    static SchematicRendererEngine& getInstance() {
        static SchematicRendererEngine instance;
        return instance;
    }

    void addGhostBlock(int x, int y, int z, const std::string& blockIdentifier) {
        std::lock_guard lock(mMutex);
        mGhostBlocks.push_back({x, y, z, blockIdentifier});
    }

    void clearGhostBlocks() {
        std::lock_guard lock(mMutex);
        mGhostBlocks.clear();
    }

    const std::vector<GhostBlock>& getGhostBlocks() const {
        return mGhostBlocks;
    }

    void renderGhostBlocks() {
        std::lock_guard lock(mMutex);
        if (mGhostBlocks.empty()) return;

        auto& tess = render::Tessellator::getInstance();
        for (const auto& gb : mGhostBlocks) {
            // Draw cyan semi-transparent wireframe for ghost block preview
            tess.drawBlockOutline(gb.x, gb.y, gb.z, 0.2f, 0.8f, 1.0f, 0.7f);
        }
    }

    void drawChunkBorder(int chunkX, int chunkZ, float r, float g, float b, float a) {
        float minX = static_cast<float>(chunkX * 16);
        float minZ = static_cast<float>(chunkZ * 16);
        float maxX = minX + 16.0f;
        float maxZ = minZ + 16.0f;
        float minY = -64.0f;
        float maxY = 320.0f;

        AABB box{Vec3{minX, minY, minZ}, Vec3{maxX, maxY, maxZ}};
        render::Tessellator::getInstance().drawBoxWireframe(box, r, g, b, a);
    }

private:
    SchematicRendererEngine() = default;
    std::mutex mMutex;
    std::vector<GhostBlock> mGhostBlocks;
};

} // namespace fabric::schematic
