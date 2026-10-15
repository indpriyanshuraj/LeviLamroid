#pragma once
#include <cstdint>

struct OffscreenCaptureDescription {
    bool enabled{false};
    uint32_t captureTargetId{0};
};

namespace dragon {

struct RenderMetadata {
    struct Storage {
        OffscreenCaptureDescription mDesc;
        OffscreenCaptureDescription const& get() const { return mDesc; }
    };

    static Storage mOffscreenCaptureDescription;
};

inline RenderMetadata::Storage RenderMetadata::mOffscreenCaptureDescription{};

} // namespace dragon
