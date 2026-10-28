#pragma once

#include "mc/deps/core/math/Vec3.h"
#include "levilamroid/api/mcpe/Offsets.h"
#include "ll/api/memory/Hook.h"
#include <string>
#include <string_view>

namespace fabric::audio {

class AudioEngine {
private:
    using PlayAtFn = void (*)(void*, const std::string&, const Vec3&, float, float);
    PlayAtFn mPlayAtFn{nullptr};
    void* mSoundEngineInstance{nullptr};

    AudioEngine() {
        uintptr_t mcBase = ::ll::memory::getMinecraftBase();
        const auto& off = ::levilamroid::mcpe::getOffsets();

        void* sym = ::ll::memory::findMinecraftSymbol("_ZN11SoundEngine6playAtERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERK4Vec3ff");
        if (sym) {
            mPlayAtFn = reinterpret_cast<PlayAtFn>(sym);
        } else if (mcBase && off.soundEnginePlayAt) {
            mPlayAtFn = reinterpret_cast<PlayAtFn>(mcBase + off.soundEnginePlayAt);
        }

        void* getInstSym = ::ll::memory::findMinecraftSymbol("_ZN11SoundEngine3getEv");
        if (getInstSym) {
            using GetFn = void* (*)();
            mSoundEngineInstance = reinterpret_cast<GetFn>(getInstSym)();
        }
    }

public:
    static AudioEngine& getInstance() {
        static AudioEngine sAudio;
        return sAudio;
    }

    /// Play a 3D positional sound effect or voice packet at exact (x, y, z) coordinates (Simple Voice Chat)
    bool playPositionalSound(std::string_view soundName, const Vec3& pos, float volume = 1.0f, float pitch = 1.0f) {
        if (mPlayAtFn && mSoundEngineInstance) {
            std::string name(soundName);
            mPlayAtFn(mSoundEngineInstance, name, pos, volume, pitch);
            return true;
        }

        // Direct command dispatch fallback
        using DispatchFn = bool (*)(const char*);
        static auto sDispatch = reinterpret_cast<DispatchFn>(dlsym(RTLD_DEFAULT, "ll_command_dispatch_client"));
        if (sDispatch) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "/playsound %.*s @a %f %f %f %f %f",
                     static_cast<int>(soundName.length()), soundName.data(),
                     pos.x, pos.y, pos.z, volume, pitch);
            return sDispatch(cmd);
        }
        return false;
    }
};

} // namespace fabric::audio

extern "C" {
bool fabric_play_sound_at(const char* soundName, float x, float y, float z, float volume, float pitch);
}
