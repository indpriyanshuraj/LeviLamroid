#pragma once

#include <dlfcn.h>
#include <string>
#include <vector>
#include <cstdint>
#include "levilamroid/api/Logger.h"

namespace levilamroid::sound {

// Minimal FMOD types matching libfmod.so AAPCS64
typedef int FMOD_RESULT;
typedef struct FMOD_SYSTEM FMOD_SYSTEM;
typedef struct FMOD_SOUND FMOD_SOUND;
typedef struct FMOD_CHANNEL FMOD_CHANNEL;

#define FMOD_OK 0
#define FMOD_DEFAULT 0x00000000
#define FMOD_OPENMEMORY 0x00000800
#define FMOD_2D 0x00000008
#define FMOD_3D 0x00000010
#define FMOD_LOOP_OFF 0x00000001
#define FMOD_LOOP_NORMAL 0x00000002

struct FMOD_CREATESOUNDEXINFO {
    int cbsize;
    unsigned int length;
    unsigned int fileoffset;
    int numchannels;
    int defaultfrequency;
    int format;
    unsigned int decodebuffersize;
    int initialsubsound;
    int numsubsounds;
    int* inclusionlist;
    int inclusionlistnum;
    void* pcmreadcallback;
    void* pcmsetposcallback;
    void* nonblockcallback;
    const char* dlsname;
    const char* encryptionkey;
    int maxpolyphony;
    void* userdata;
    int suggestedsoundtype;
    void* useropen;
    void* userclose;
    void* userread;
    void* userseek;
    void* userasyncread;
    void* userasynccancel;
    void* fileuserdata;
    int filebuffersize;
    int channelorder;
    void* initialsoundgroup;
    void* minchans;
    void* maxchans;
    int ignoresetfilesystem;
    int audioqueuepolicy;
    unsigned int minfreesize;
};

using FMOD_System_CreateSound_t = FMOD_RESULT (*)(
    FMOD_SYSTEM* system,
    const char* name_or_data,
    unsigned int mode,
    FMOD_CREATESOUNDEXINFO* exinfo,
    FMOD_SOUND** sound
);

using FMOD_System_PlaySound_t = FMOD_RESULT (*)(
    FMOD_SYSTEM* system,
    FMOD_SOUND* sound,
    void* channelgroup,
    bool paused,
    FMOD_CHANNEL** channel
);

using FMOD_Sound_Release_t = FMOD_RESULT (*)(FMOD_SOUND* sound);
using FMOD_Channel_SetVolume_t = FMOD_RESULT (*)(FMOD_CHANNEL* channel, float volume);
using FMOD_Channel_SetPitch_t = FMOD_RESULT (*)(FMOD_CHANNEL* channel, float pitch);
using FMOD_Channel_Stop_t = FMOD_RESULT (*)(FMOD_CHANNEL* channel);

/// @brief Direct In-Memory Audio Engine (Method A).
/// Plays raw embedded audio (WAV/OGG/MP3) directly using FMOD without resource packs or sounds.json.
class FmodAudioEngine {
private:
    FMOD_SYSTEM* mFmodSystem = nullptr;
    FMOD_System_CreateSound_t mCreateSound = nullptr;
    FMOD_System_PlaySound_t mPlaySound = nullptr;
    FMOD_Sound_Release_t mReleaseSound = nullptr;
    FMOD_Channel_SetVolume_t mSetVolume = nullptr;
    FMOD_Channel_SetPitch_t mSetPitch = nullptr;
    FMOD_Channel_Stop_t mStopChannel = nullptr;
    bool mInitialized = false;

    FmodAudioEngine() = default;

public:
    static FmodAudioEngine& getInstance() {
        static FmodAudioEngine instance;
        return instance;
    }

    /// @brief Initialize by linking to Minecraft's loaded libfmod.so
    bool init(FMOD_SYSTEM* system = nullptr) {
        if (mInitialized && mFmodSystem) return true;

        void* fmodHandle = dlopen("libfmod.so", RTLD_NOW | RTLD_NOLOAD);
        if (!fmodHandle) {
            fmodHandle = RTLD_DEFAULT;
        }

        mCreateSound = (FMOD_System_CreateSound_t)dlsym(fmodHandle, "FMOD_System_CreateSound");
        mPlaySound   = (FMOD_System_PlaySound_t)dlsym(fmodHandle, "FMOD_System_PlaySound");
        mReleaseSound= (FMOD_Sound_Release_t)dlsym(fmodHandle, "FMOD_Sound_Release");
        mSetVolume   = (FMOD_Channel_SetVolume_t)dlsym(fmodHandle, "FMOD_Channel_SetVolume");
        mSetPitch    = (FMOD_Channel_SetPitch_t)dlsym(fmodHandle, "FMOD_Channel_SetPitch");
        mStopChannel = (FMOD_Channel_Stop_t)dlsym(fmodHandle, "FMOD_Channel_Stop");

        if (system) {
            mFmodSystem = system;
        }

        mInitialized = (mCreateSound != nullptr && mPlaySound != nullptr);
        return mInitialized;
    }

    void setSystem(FMOD_SYSTEM* system) {
        mFmodSystem = system;
    }

    FMOD_SYSTEM* getSystem() const {
        return mFmodSystem;
    }

    /// @brief Play an in-memory sound buffer (WAV, OGG, MP3) directly without any resourcepack on disk!
    FMOD_CHANNEL* playMemory(
        const void* data,
        size_t size,
        float volume = 1.0f,
        float pitch = 1.0f,
        bool loop = false
    ) {
        if (!init() || !mFmodSystem || !data || size == 0) return nullptr;

        FMOD_CREATESOUNDEXINFO exinfo{};
        exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
        exinfo.length = static_cast<unsigned int>(size);

        unsigned int mode = FMOD_OPENMEMORY | FMOD_2D;
        mode |= loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;

        FMOD_SOUND* sound = nullptr;
        FMOD_RESULT res = mCreateSound(
            mFmodSystem,
            reinterpret_cast<const char*>(data),
            mode,
            &exinfo,
            &sound
        );

        if (res == FMOD_OK && sound) {
            FMOD_CHANNEL* channel = nullptr;
            if (mPlaySound(mFmodSystem, sound, nullptr, false, &channel) == FMOD_OK && channel) {
                if (mSetVolume) mSetVolume(channel, volume);
                if (mSetPitch) mSetPitch(channel, pitch);
                return channel;
            }
        }
        return nullptr;
    }
};

} // namespace levilamroid::sound
