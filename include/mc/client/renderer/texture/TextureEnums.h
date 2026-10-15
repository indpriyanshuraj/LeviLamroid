#pragma once
#include <cstdint>

enum class TextureLoadState : uint32_t {
    None        = 0, // Uninitialized texture state
    LoadedBit   = 1, // Fully decoded and uploaded to GPU VRAM
    QueuedBit   = 2, // Scheduled in background worker list
    LoadingBit  = 4, // Active decoding in CPU thread
    ErrorBit    = 8  // Failed decoding or missing resource
};

enum class TextureFormat : uint32_t {
    Unknown            = 0, // Unspecified texture format
    R8G8B8A8_UNORM     = 1, // Standard 32-bit RGBA normalized color
    B8G8R8A8_UNORM     = 2, // Standard 32-bit BGRA color
    R8_UNORM           = 3, // Single channel 8-bit grayscale / alpha map
    R16G16B16A16_FLOAT = 4  // High dynamic range 64-bit floating point texture
};

enum class ResourcePackStackType : uint32_t {
    Invalid   = 0,
    Levels    = 1,
    Addon     = 2,
    World     = 3,
    Global    = 4, // Global Resource Packs (highest client priority)
    BaseGame  = 5, // Vanilla base assets
    Treatment = 6,
    Premium   = 7
};

enum class PackAccessStrategyType : uint32_t {
    Directory    = 1, // Extracted folders on disk
    Zip          = 2, // Compressed .zip or .mcpack files
    Subpack      = 3, // Subpack configuration inside pack
    EncryptedZip = 4  // Encrypted marketplace pack
};
