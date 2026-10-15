#pragma once
#include "mc/util/Mirror.h"
#include "mc/util/Rotation.h"

class Block;

class VanillaBlockStateTransformUtils {
public:
    static Block const* transformBlock(
        Block const& block,
        Rotation rotation,
        Mirror mirror
    );

    static Block const* transformBlock(
        Block const& block,
        Rotation rotation
    ) {
        return transformBlock(block, rotation, Mirror::None);
    }

    static Block const* transformBlock(
        Block const& block,
        Mirror mirror
    ) {
        return transformBlock(block, Rotation::None, mirror);
    }

    static unsigned char transformFacing(unsigned char facing, Rotation rotation) noexcept;
    static unsigned char transformFacing(unsigned char facing, Mirror mirror) noexcept;
};

inline Block const* VanillaBlockStateTransformUtils::transformBlock(
    Block const& block,
    Rotation rotation,
    Mirror mirror
) {
    (void)rotation;
    (void)mirror;
    return &block;
}

inline unsigned char VanillaBlockStateTransformUtils::transformFacing(unsigned char facing, Rotation rotation) noexcept {
    (void)rotation;
    return facing;
}

inline unsigned char VanillaBlockStateTransformUtils::transformFacing(unsigned char facing, Mirror mirror) noexcept {
    (void)mirror;
    return facing;
}
