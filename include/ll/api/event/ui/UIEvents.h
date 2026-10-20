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

class ButtonClickedEvent : public Event {
private:
    std::string mButtonName;
    void* mController;
    void* mPropertyBag;

public:
    ButtonClickedEvent(std::string name, void* controller, void* propBag)
        : mButtonName(std::move(name)), mController(controller), mPropertyBag(propBag) {}

    const std::string& getButtonName() const { return mButtonName; }
    void* getController() const { return mController; }
    void* getPropertyBag() const { return mPropertyBag; }
};

class PauseMenuOpenEvent : public Event {
private:
    void* mController;
public:
    explicit PauseMenuOpenEvent(void* controller = nullptr) : mController(controller) {}
    void* getController() const { return mController; }
};

class PauseMenuCloseEvent : public Event {
private:
    void* mController;
public:
    explicit PauseMenuCloseEvent(void* controller = nullptr) : mController(controller) {}
    void* getController() const { return mController; }
};

} // namespace ll::event
