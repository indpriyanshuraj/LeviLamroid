#pragma once
#include "levilamroid/api/event/Event.h"
#include <string>

namespace ll::event {

class ScreenTechStackQueryEvent : public Event {
private:
    std::string mScreenName;
    int mTechStack;

public:
    ScreenTechStackQueryEvent(std::string name, int defaultStack)
        : mScreenName(std::move(name)), mTechStack(defaultStack) {}

    const std::string& getScreenName() const { return mScreenName; }
    int getTechStack() const { return mTechStack; }
    void setTechStack(int stack) { mTechStack = stack; }
};

class StartMenuOpenEvent : public Event {
public:
    StartMenuOpenEvent() = default;
};

class PlayButtonClickedEvent : public Event {
public:
    PlayButtonClickedEvent() = default;
};

class SettingsButtonClickedEvent : public Event {
public:
    SettingsButtonClickedEvent() = default;
};

class RealmsButtonClickedEvent : public Event {
public:
    RealmsButtonClickedEvent() = default;
};

class MarketplaceButtonClickedEvent : public Event {
public:
    MarketplaceButtonClickedEvent() = default;
};

class DressingRoomButtonClickedEvent : public Event {
public:
    DressingRoomButtonClickedEvent() = default;
};

class MenuExitEvent : public Event {
public:
    MenuExitEvent() = default;
};

} // namespace ll::event
