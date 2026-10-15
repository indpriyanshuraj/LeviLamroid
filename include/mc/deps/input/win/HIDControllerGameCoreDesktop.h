#pragma once
#include <cstdint>
#include "mc/deps/input/Keyboard.h"

namespace Bedrock::Input::KeyboardEventProcessor {
enum class InputOrigin : int32_t {
    Direct = 0,
    Injected = 1
};
}

class HIDControllerGameCoreDesktop {
public:
    virtual ~HIDControllerGameCoreDesktop() = default;

    virtual void onKeyDown(
        int keyCode,
        int keyData,
        Bedrock::Input::KeyboardEventProcessor::InputOrigin originType
    ) {
        (void)keyCode;
        (void)keyData;
        (void)originType;
    }

    virtual void onKeyUp(
        int keyCode,
        int keyData,
        Bedrock::Input::KeyboardEventProcessor::InputOrigin originType
    ) {
        (void)keyCode;
        (void)keyData;
        (void)originType;
    }

    virtual void $onKeyDown(
        int keyCode,
        int keyData,
        Bedrock::Input::KeyboardEventProcessor::InputOrigin originType
    ) {
        onKeyDown(keyCode, keyData, originType);
    }

    virtual void $onKeyUp(
        int keyCode,
        int keyData,
        Bedrock::Input::KeyboardEventProcessor::InputOrigin originType
    ) {
        onKeyUp(keyCode, keyData, originType);
    }
};
