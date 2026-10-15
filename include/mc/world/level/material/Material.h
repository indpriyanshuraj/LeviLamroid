#pragma once
#include <cstdint>

enum class MaterialType : int32_t {
    Air = 0,
    Dirt = 1,
    Wood = 2,
    Stone = 3,
    Metal = 4,
    Water = 5,
    Lava = 6,
    Leaves = 7,
    Plant = 8,
    Solid = 9,
    Glass = 10,
    Explosive = 11,
    Ice = 12,
    Portal = 13,
    Decoration = 14
};

class Material {
public:
    MaterialType mType{MaterialType::Solid};
    bool         mFlammable{false};
    bool         mNeverBuildable{false};
    bool         mAlwaysDestroyable{false};
    bool         mReplaceable{false};
    bool         mLiquid{false};
    float        mTranslucency{0.0f};
    bool         mBlocksMotion{true};
    bool         mSolid{true};
    bool         mSuperHot{false};

    constexpr Material() noexcept = default;
    constexpr Material(MaterialType type, bool isLiquid = false, bool isSolid = true) noexcept
        : mType(type), mLiquid(isLiquid), mSolid(isSolid) {}

    [[nodiscard]] constexpr bool isLiquid() const noexcept { return mLiquid; }
    [[nodiscard]] constexpr bool isSolid() const noexcept { return mSolid; }
    [[nodiscard]] constexpr bool isReplaceable() const noexcept { return mReplaceable; }
    [[nodiscard]] constexpr MaterialType getType() const noexcept { return mType; }
};
