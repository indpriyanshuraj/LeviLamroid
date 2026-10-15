#pragma once

#include <string>
#include "levilamroid/api/event/Event.h"
#include "levilamroid/api/sound/SoundEngine.h"
#include "levilamroid/api/sound/LevelSoundEvent.h"

namespace ll::event {

/// @brief Fired before Minecraft plays any 3D sound event in the world.
/// Essential for acoustic simulation mods (e.g. Sound Physics / Spatial Reverb).
/// Mods can read sound parameters, perform block raycasts, calculate occlusion / reverberation,
/// adjust volume and pitch, or cancel the sound entirely.
class SoundPlayEvent : public Event {
private:
    std::string mSoundName;
    levilamroid::mcpe::Vec3 mPosition;
    float mVolume;
    float mPitch;

public:
    SoundPlayEvent(
        std::string soundName,
        const levilamroid::mcpe::Vec3& pos,
        float volume,
        float pitch
    ) : mSoundName(std::move(soundName)), mPosition(pos), mVolume(volume), mPitch(pitch) {}

    const std::string& getSoundName() const { return mSoundName; }
    void setSoundName(std::string name) { mSoundName = std::move(name); }

    const levilamroid::mcpe::Vec3& getPosition() const { return mPosition; }
    levilamroid::mcpe::Vec3& getPosition() { return mPosition; }
    void setPosition(const levilamroid::mcpe::Vec3& pos) { mPosition = pos; }

    float getVolume() const { return mVolume; }
    void setVolume(float vol) { mVolume = vol; }

    float getPitch() const { return mPitch; }
    void setPitch(float p) { mPitch = p; }
};

/// @brief Fired when a high-level gameplay sound event is triggered (e.g. footsteps, breaking, chests).
class LevelSoundEventOccurred : public Event {
private:
    levilamroid::mcpe::LevelSoundEvent mType;
    levilamroid::mcpe::Vec3 mPosition;
    int mData;

public:
    LevelSoundEventOccurred(
        levilamroid::mcpe::LevelSoundEvent type,
        const levilamroid::mcpe::Vec3& pos,
        int data
    ) : mType(type), mPosition(pos), mData(data) {}

    levilamroid::mcpe::LevelSoundEvent getType() const { return mType; }
    const levilamroid::mcpe::Vec3& getPosition() const { return mPosition; }
    int getData() const { return mData; }
};

} // namespace ll::event

namespace levilamroid::event {
    using namespace ll::event;
}
