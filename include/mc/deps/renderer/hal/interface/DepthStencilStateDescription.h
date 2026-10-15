#pragma once
#include <cstdint>

namespace mce {

struct DepthStencilStateDescription {
    bool depthTestEnabled{true};
    bool depthWriteEnabled{true};
    uint32_t depthFunc{0};
    bool stencilEnabled{false};
};

} // namespace mce
