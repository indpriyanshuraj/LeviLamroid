#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include "ll/api/Logger.h"

namespace ll::mod {

class NativeMod {
public:
    std::string mName{"UnknownMod"};
    ll::Logger  mLogger{"Mod"};

    mutable std::filesystem::path mCachedConfigDir;

    NativeMod() = default;
    explicit NativeMod(const char* name) : mName(name ? name : ""), mLogger(mName) {}
    explicit NativeMod(std::string_view name) : mName(name), mLogger(mName) {}
    explicit NativeMod(const std::string& name) : mName(name), mLogger(mName) {}
    virtual ~NativeMod() = default;

    [[nodiscard]] ll::Logger& getLogger() noexcept { return mLogger; }
    [[nodiscard]] ll::Logger const& getLogger() const noexcept { return mLogger; }
    [[nodiscard]] std::string const& getName() const noexcept { return mName; }
    [[nodiscard]] std::filesystem::path getConfigDir() const {
        if (!mCachedConfigDir.empty()) return mCachedConfigDir;
#ifdef _WIN32
        mCachedConfigDir = std::filesystem::path("config") / mName;
        return mCachedConfigDir;
#else
        std::error_code ec;
        const std::filesystem::path baseLauncherDir = "/sdcard/Android/media/org.levimc.launcher/minecraft";
        if (std::filesystem::exists(baseLauncherDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(baseLauncherDir, ec)) {
                if (entry.is_directory(ec)) {
                    auto candidate = entry.path() / "mods" / mName;
                    if (std::filesystem::exists(candidate, ec)) {
                        mCachedConfigDir = candidate;
                        return mCachedConfigDir;
                    }
                }
            }
        }
        const std::filesystem::path candidateDirs[] = {
            baseLauncherDir / "com.mojang.minecraftpe/mods" / mName,
            std::filesystem::path("/sdcard/Android/media/org.levimc.launcher/mods") / mName,
            std::filesystem::path("/sdcard/games/com.mojang/mods") / mName
        };
        for (const auto& dir : candidateDirs) {
            if (std::filesystem::exists(dir, ec)) {
                mCachedConfigDir = dir;
                return mCachedConfigDir;
            }
        }
        std::filesystem::create_directories(candidateDirs[0], ec);
        mCachedConfigDir = candidateDirs[0];
        return mCachedConfigDir;
#endif
    }
    [[nodiscard]] std::filesystem::path getDataDir() const {
#ifdef _WIN32
        return std::filesystem::path("plugins") / mName;
#else
        return getConfigDir();
#endif
    }

    static NativeMod* current() {
        static NativeMod sCurrent{"CurrentMod"};
        return &sCurrent;
    }
};

} // namespace ll::mod
