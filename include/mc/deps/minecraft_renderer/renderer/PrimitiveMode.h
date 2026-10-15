#pragma once

namespace mce {

enum class PrimitiveMode : int {
    LineList = 1,
    LineStrip = 2,
    Triangles = 3,
    TriangleStrip = 4,
    QuadList = 7
};

} // namespace mce
