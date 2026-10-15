#pragma once
#include <memory>
#include <string>
#include <glm/vec2.hpp>
#include "mc/client/gui/controls/UIControl.h"

class VisualTree {
public:
    std::shared_ptr<UIControl> mRootControl;
};

class ScreenView {
public:
    virtual ~ScreenView() = default;

    PropertyVec2 mSize;
    std::string mScreenName{"start_screen"};
    std::shared_ptr<void> mController{nullptr};
    std::unique_ptr<VisualTree> mVisualTree;

    std::string getScreenName() const {
        return mScreenName;
    }
};
