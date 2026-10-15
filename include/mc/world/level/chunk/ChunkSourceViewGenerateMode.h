#pragma once
#include <cstdint>

enum class ChunkSourceViewGenerateMode : int32_t {
    None = 0,
    DontGenerateOnlyGet = 0,
    CreateOnly = 1,
    Generate = 2
};
