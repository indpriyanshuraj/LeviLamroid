#pragma once

#include <atomic>
#include <cstdint>

namespace fabric::render {

class GammaUtils {
private:
    static inline std::atomic<bool> sFullbrightEnabled{false};
    static inline std::atomic<float> sCustomGamma{1.0f};

public:
    static void setFullbright(bool enabled) noexcept {
        sFullbrightEnabled.store(enabled);
    }

    [[nodiscard]] static bool isFullbright() noexcept {
        return sFullbrightEnabled.load();
    }

    static void setGamma(float gamma) noexcept {
        sCustomGamma.store(gamma);
    }

    [[nodiscard]] static float getGamma() noexcept {
        return sCustomGamma.load();
    }
};

} // namespace fabric::render

extern "C" {
void fabric_set_fullbright(bool enabled);
bool fabric_is_fullbright();
void fabric_set_gamma(float gamma);
float fabric_get_gamma();
}
