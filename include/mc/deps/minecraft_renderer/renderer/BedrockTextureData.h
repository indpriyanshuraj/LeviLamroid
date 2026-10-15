#pragma once
#include <string>

namespace mce {

class BedrockTextureData {
public:
    std::string mName{};

    BedrockTextureData() = default;

    template <typename T>
    BedrockTextureData(T const&) {}
};

class TexturePtr;

} // namespace mce
