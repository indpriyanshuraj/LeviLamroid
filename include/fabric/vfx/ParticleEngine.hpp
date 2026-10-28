#pragma once

#include "mc/deps/core/math/Vec3.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/memory/Hook.h"
#include <string_view>
#include <string>

namespace fabric::vfx {

class ParticleEngine {
private:
    using AddParticleFn = void (*)(void*, int, const Vec3&, const Vec3&, int, const void*, bool, bool);
    AddParticleFn mAddParticleFn{nullptr};

    ParticleEngine() {
        void* sym = ::ll::memory::findMinecraftSymbol("_ZN5Level11addParticleE12ParticleTypeRK4Vec3S3_iPK17CompoundTag_N_N");
        if (sym) {
            mAddParticleFn = reinterpret_cast<AddParticleFn>(sym);
        }
    }

public:
    static ParticleEngine& getInstance() {
        static ParticleEngine sEngine;
        return sEngine;
    }

    /// Spawn a native Bedrock particle type with velocity
    bool spawnParticle(int particleType, const Vec3& pos, const Vec3& velocity = Vec3{0, 0, 0}, int data = 0) {
        auto* level = ::ll::service::getLevel();
        if (level && mAddParticleFn) {
            mAddParticleFn(level, particleType, pos, velocity, data, nullptr, false, false);
            return true;
        }

        // Direct command dispatch fallback
        using DispatchFn = bool (*)(const char*);
        static auto sDispatch = reinterpret_cast<DispatchFn>(dlsym(RTLD_DEFAULT, "ll_command_dispatch_client"));
        if (sDispatch) {
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "/particle minecraft:basic_smoke_particle %f %f %f", pos.x, pos.y, pos.z);
            sDispatch(cmd);
            return true;
        }
        return false;
    }

    /// Spawn a custom data-driven particle definition injected via Resource Pack (e.g. Photon / AAA Particles)
    bool spawnCustomParticle(std::string_view particleIdentifier, const Vec3& pos, const Vec3& velocity = Vec3{0, 0, 0}) {
        (void)velocity;
        using DispatchFn = bool (*)(const char*);
        static auto sDispatch = reinterpret_cast<DispatchFn>(dlsym(RTLD_DEFAULT, "ll_command_dispatch_client"));
        if (sDispatch) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "/particle %.*s %f %f %f",
                     static_cast<int>(particleIdentifier.length()), particleIdentifier.data(),
                     pos.x, pos.y, pos.z);
            return sDispatch(cmd);
        }
        return false;
    }
};

} // namespace fabric::vfx

extern "C" {
bool fabric_spawn_particle(const char* identifier, float x, float y, float z, float vx, float vy, float vz);
}
