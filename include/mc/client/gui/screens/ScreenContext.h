#pragma once
#include <memory>
#include "mc/client/renderer/Tessellator.h"

class ScreenContext {
public:
    virtual ~ScreenContext() = default;

    Tessellator tessellator{};

    struct CameraWrapper {
        struct StackWrapper {
            struct MatWrapper {
                void translate(float x, float y, float z) { (void)x; (void)y; (void)z; }
            };
            struct Node {
                MatWrapper* mat{nullptr};
            };
            Node push(bool b = false) { (void)b; static MatWrapper sMat; return Node{&sMat}; }
        };
        struct StackRef {
            StackWrapper& get() { static StackWrapper s; return s; }
        };
        StackRef worldMatrixStack{};
    };
    CameraWrapper camera{};
};
