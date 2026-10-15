#pragma once
#include "mc/util/Mirror.h"
#include "mc/util/Rotation.h"
#include "mc/world/level/levelgen/structure/BoundingBox.h"

class Block;

class LegacyStructureSettings {
public:
    Mirror      mMirror{Mirror::None};
    Rotation    mRotation{Rotation::None};
    Block const* mIgnoreBlock{nullptr};
    BoundingBox mBoundingBox{};
    bool        mIgnoreEntities{false};
    float       mIntegrity{1.0f};
    uint32_t    mSeed{0};

    constexpr LegacyStructureSettings() noexcept = default;

    constexpr LegacyStructureSettings(
        Mirror mirror,
        Rotation rotation,
        Block const* ignoreBlock,
        BoundingBox const& boundingBox
    ) noexcept
        : mMirror(mirror),
          mRotation(rotation),
          mIgnoreBlock(ignoreBlock),
          mBoundingBox(boundingBox) {}

    constexpr LegacyStructureSettings(
        Mirror mirror,
        Rotation rotation,
        bool ignoreEntities,
        BoundingBox const* boundingBox
    ) noexcept
        : mMirror(mirror),
          mRotation(rotation),
          mIgnoreEntities(ignoreEntities),
          mBoundingBox(boundingBox ? *boundingBox : BoundingBox{}) {}

    [[nodiscard]] constexpr Mirror getMirror() const noexcept { return mMirror; }
    [[nodiscard]] constexpr Rotation getRotation() const noexcept { return mRotation; }
    [[nodiscard]] constexpr BoundingBox const& getBoundingBox() const noexcept { return mBoundingBox; }
    [[nodiscard]] constexpr bool isIgnoreEntities() const noexcept { return mIgnoreEntities; }
    [[nodiscard]] constexpr float getIntegrity() const noexcept { return mIntegrity; }

    constexpr void setMirror(Mirror mirror) noexcept { mMirror = mirror; }
    constexpr void setRotation(Rotation rotation) noexcept { mRotation = rotation; }
    constexpr void setBoundingBox(BoundingBox const& bb) noexcept { mBoundingBox = bb; }
    constexpr void setIgnoreEntities(bool ignore) noexcept { mIgnoreEntities = ignore; }
    constexpr void setIntegrity(float integrity) noexcept { mIntegrity = integrity; }
};
