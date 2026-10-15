#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/vec2.hpp>

struct PropertyVec2 {
    glm::vec2 val{0.0f, 0.0f};
    glm::vec2 get() const { return val; }
};

struct PropertyString {
    std::string val;
    std::string get() const { return val; }
};

class UIControl {
public:
    PropertyVec2 mSize;
    PropertyVec2 mParentRelativePosition;
    PropertyString mName;
    float mAlpha{1.0f};
    bool mPropagateAlpha{true};
    struct PropertyChildren {
        std::vector<std::shared_ptr<UIControl>> children;
        const std::vector<std::shared_ptr<UIControl>>& get() const { return children; }
    } mChildren;
};

class IClientInstance {};
