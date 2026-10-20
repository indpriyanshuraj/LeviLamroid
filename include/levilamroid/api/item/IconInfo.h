#pragma once
#include <string>
#include <cstdint>

#include "levilamroid/api/math/HashedString.h"

namespace levilamroid::item {

enum class ItemIconInfoType : int {
    Texture = 0,
    Atlas   = 1
};

struct ItemIconInfo {
    HashedString mIcon;        // 32 bytes: uint64_t mHash at +0x00, std::string mStr at +0x08
    int mFrame{0};             // 4 bytes at +0x20
    int mAtlasTotalFrames{1};  // 4 bytes at +0x24

    ItemIconInfo() = default;
    ItemIconInfo(const HashedString& icon, int frame = 0, int totalFrames = 1)
        : mIcon(icon), mFrame(frame), mAtlasTotalFrames(totalFrames) {}
    ItemIconInfo(std::string_view name, int frame = 0, int totalFrames = 1)
        : mIcon(name), mFrame(frame), mAtlasTotalFrames(totalFrames) {}
    ItemIconInfo(const std::string& name, int frame = 0, int totalFrames = 1)
        : mIcon(name), mFrame(frame), mAtlasTotalFrames(totalFrames) {}
};
// NOTE: sizeof(ItemIconInfo) depends on HashedString which depends on std::string ABI.
//   Android NDK libc++ (LLVM): 32 + 4 + 4 = 40 bytes  ✓
//   Host GCC libstdc++:        40 + 4 + 4 = 48 bytes (cross-compile host only — harmless)
// _LIBCPP_VERSION is defined by libc++ (NDK) but NOT by host GCC libstdc++.
#if defined(__ANDROID__) || defined(_LIBCPP_VERSION)
static_assert(sizeof(ItemIconInfo) == 40, "ItemIconInfo must be exactly 40 bytes on ARM64");
#endif

struct ResolvedItemIconInfo {
    std::string      mIconName;
    int              mIconFrame{0};
    ItemIconInfoType mIconType{ItemIconInfoType::Texture};
    uint32_t         mRuntimeBlockID{0};
    float            mU0{0.0f};
    float            mV0{0.0f};
    float            mU1{1.0f};
    float            mV1{1.0f};
    uint16_t         mTexSizeW{16};
    uint16_t         mTexSizeH{16};
};

} // namespace levilamroid::item

namespace ll::item {
    using ItemIconInfo = ::levilamroid::item::ItemIconInfo;
    using ResolvedItemIconInfo = ::levilamroid::item::ResolvedItemIconInfo;
    using ItemIconInfoType = ::levilamroid::item::ItemIconInfoType;
}
