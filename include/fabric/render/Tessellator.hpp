#pragma once

#include <cstdint>
#include <cstddef>
#include <dlfcn.h>
#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/core/math/AABB.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"

namespace fabric::render {

enum class TessellatorMode : int {
    Points = 0,
    Lines = 1,
    LineList = 2,
    LineStrip = 3,
    Triangles = 4,
    TriangleStrip = 5,
    TriangleFan = 6,
    Quads = 7
};

class Tessellator {
private:
    using GetInstanceFn = void* (*)();
    using BeginFn = void (*)(void*, int, size_t);
    using VertexFn = void (*)(void*, float, float, float);
    using ColorFn = void (*)(void*, float, float, float, float);
    using EndFn = void (*)(void*);

    void* mInstance{nullptr};
    BeginFn mBeginFn{nullptr};
    VertexFn mVertexFn{nullptr};
    ColorFn mColorFn{nullptr};
    EndFn mEndFn{nullptr};

    Tessellator() {
        // Dynamic symbol / RVA resolution
        uintptr_t mcBase = ::ll::memory::getMinecraftBase();
        const auto& off = ::levilamroid::mcpe::getOffsets();

        // 1. Dynamic export lookup
        void* getInstSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator3getEv");
        if (!getInstSym) getInstSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator11getInstanceEv");
        if (getInstSym) {
            mInstance = reinterpret_cast<GetInstanceFn>(getInstSym)();
        }

        void* beginSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator5beginEii");
        if (!beginSym) beginSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator5beginEiRK8Materialm");
        if (!beginSym) beginSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator5beginEim");
        if (beginSym) {
            mBeginFn = reinterpret_cast<BeginFn>(beginSym);
        } else if (mcBase) {
            uintptr_t targetRva = off.tessellatorBegin ? off.tessellatorBegin : 0x096b24ec;
            mBeginFn = reinterpret_cast<BeginFn>(mcBase + targetRva);
        }

        void* vertexSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator6vertexEfff");
        if (vertexSym) mVertexFn = reinterpret_cast<VertexFn>(vertexSym);

        void* colorSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator5colorEffff");
        if (colorSym) mColorFn = reinterpret_cast<ColorFn>(colorSym);

        void* endSym = ::ll::memory::findMinecraftSymbol("_ZN11Tessellator3endEv");
        if (endSym) mEndFn = reinterpret_cast<EndFn>(endSym);
    }

public:
    static Tessellator& getInstance() {
        static Tessellator sTess;
        return sTess;
    }

    [[nodiscard]] bool isAvailable() const noexcept {
        return mInstance != nullptr && mBeginFn != nullptr && mVertexFn != nullptr && mColorFn != nullptr && mEndFn != nullptr;
    }

    void begin(TessellatorMode mode = TessellatorMode::LineList, size_t maxVertices = 512) {
        if (mInstance && mBeginFn) {
            mBeginFn(mInstance, static_cast<int>(mode), maxVertices);
        }
    }

    void vertex(float x, float y, float z) {
        if (mInstance && mVertexFn) {
            mVertexFn(mInstance, x, y, z);
        }
    }

    void vertex(const Vec3& pos) {
        vertex(pos.x, pos.y, pos.z);
    }

    void color(float r, float g, float b, float a = 1.0f) {
        if (mInstance && mColorFn) {
            mColorFn(mInstance, r, g, b, a);
        }
    }

    void end() {
        if (mInstance && mEndFn) {
            mEndFn(mInstance);
        }
    }

    // ── Helper: 3D Depth-Tested Box Wireframe (Litematica / Block Outline) ──
    void drawBoxWireframe(const AABB& box, float r, float g, float b, float a = 1.0f) {
        if (!isAvailable()) return;
        begin(TessellatorMode::LineList, 24);

        const Vec3& min = box.min;
        const Vec3& max = box.max;

        auto line = [this, r, g, b, a](float x1, float y1, float z1, float x2, float y2, float z2) {
            color(r, g, b, a); vertex(x1, y1, z1);
            color(r, g, b, a); vertex(x2, y2, z2);
        };

        // Bottom quad
        line(min.x, min.y, min.z, max.x, min.y, min.z);
        line(max.x, min.y, min.z, max.x, min.y, max.z);
        line(max.x, min.y, max.z, min.x, min.y, max.z);
        line(min.x, min.y, max.z, min.x, min.y, min.z);

        // Top quad
        line(min.x, max.y, min.z, max.x, max.y, min.z);
        line(max.x, max.y, min.z, max.x, max.y, max.z);
        line(max.x, max.y, max.z, min.x, max.y, max.z);
        line(min.x, max.y, max.z, min.x, max.y, min.z);

        // Vertical pillars
        line(min.x, min.y, min.z, min.x, max.y, min.z);
        line(max.x, min.y, min.z, max.x, max.y, min.z);
        line(max.x, min.y, max.z, max.x, max.y, max.z);
        line(min.x, min.y, max.z, min.x, max.y, max.z);

        end();
    }

    // ── Helper: Single Block Outline at Voxel Coordinates ──
    void drawBlockOutline(int x, int y, int z, float r, float g, float b, float a = 1.0f) {
        constexpr float pad = 0.002f;
        AABB box{
            Vec3{static_cast<float>(x) - pad, static_cast<float>(y) - pad, static_cast<float>(z) - pad},
            Vec3{static_cast<float>(x + 1) + pad, static_cast<float>(y + 1) + pad, static_cast<float>(z + 1) + pad}
        };
        drawBoxWireframe(box, r, g, b, a);
    }
};

} // namespace fabric::render
