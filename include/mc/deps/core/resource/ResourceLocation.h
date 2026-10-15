#pragma once
#include <string>
#include <string_view>
#include "mc/deps/core/file/Path.h"
#include "mc/deps/core/file/PathView.h"

enum class ResourceFileSystem : uint32_t {
    Raw           = 0, // Direct filesystem path access (/sdcard, /data)
    InUserPackage = 1, // Inside loaded behavior/resource pack (.mcpack / folder)
    AppPackage    = 2, // Base APK assets (assets/assets/...)
    Settings      = 3, // Application preferences storage
    External      = 4, // External storage directory
    Server        = 5  // Server-authoritative downloaded pack
};

class ResourceLocation {
public:
    ResourceFileSystem mFileSystem{ResourceFileSystem::AppPackage};
    Core::Path         mPath{};

    ResourceLocation() = default;
    ResourceLocation(std::string const& path) : mPath(path) {}
    ResourceLocation(char const* path) : mPath(path ? path : "") {}
    ResourceLocation(Core::PathView pathView) : mPath(pathView) {}
    ResourceLocation(Core::Path path) : mPath(std::move(path)) {}
    ResourceLocation(ResourceFileSystem fs, Core::PathView pathView) : mFileSystem(fs), mPath(pathView) {}
    ResourceLocation(ResourceFileSystem fs, std::string const& path) : mFileSystem(fs), mPath(path) {}

    [[nodiscard]] std::string const& getPath() const noexcept { return mPath.getFullPath(); }
    [[nodiscard]] Core::Path const& getCorePath() const noexcept { return mPath; }
    [[nodiscard]] ResourceFileSystem getFileSystem() const noexcept { return mFileSystem; }

    [[nodiscard]] bool operator==(ResourceLocation const& rhs) const noexcept {
        return mFileSystem == rhs.mFileSystem && mPath == rhs.mPath;
    }
    [[nodiscard]] bool operator!=(ResourceLocation const& rhs) const noexcept {
        return !(*this == rhs);
    }
};
