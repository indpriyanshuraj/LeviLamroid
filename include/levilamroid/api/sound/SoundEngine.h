#pragma once

#include <string>
#include <cstdint>
#include "LevelSoundEvent.h"

namespace levilamroid::mcpe {

/// @brief 3D Floating point coordinate vector
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

class Actor;

/// @brief Minecraft Bedrock Core Audio Engine Interface (Method B: In-Game Physics / Engine Audio).
/// Hookable for acoustic simulation, raycasting occlusion, reverb, and DSP filters (e.g. Sound Physics).
class SoundEngine {
public:
    virtual ~SoundEngine() = default;

    /// @brief Play a 3D positioned sound in the world with attenuation and spatial panning.
    /// Mangled: _ZN11SoundEngine4playERKNSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEERK4Vec3ff
    virtual void play(
        const std::string& soundName,
        const Vec3& position,
        float volume = 1.0f,
        float pitch = 1.0f
    ) = 0;

    /// @brief Play a 2D non-spatial user interface sound effect (menus, clicks, notifications).
    /// Mangled: _ZN11SoundEngine6playUIERKNSt6__ndk112basic_stringIcNS0_11char_traitsIcEENS0_9allocatorIcEEEEff
    virtual void playUI(
        const std::string& soundName,
        float volume = 1.0f,
        float pitch = 1.0f
    ) = 0;

    /// @brief Play a sound anchored to a living entity or player (follows entity translation).
    virtual void playAt(
        Actor* actor,
        const std::string& soundName,
        float volume = 1.0f,
        float pitch = 1.0f
    ) = 0;

    /// @brief Stop all instances of a playing sound event or track.
    virtual void stop(const std::string& soundName) = 0;
};

/// @brief Definition repository for sounds declared in sound_definitions.json or virtual memory.
class SoundRepository {
public:
    virtual ~SoundRepository() = default;

    /// @brief Query definition for a registered sound ID.
    virtual bool hasSound(const std::string& soundName) const = 0;
};

} // namespace levilamroid::mcpe
