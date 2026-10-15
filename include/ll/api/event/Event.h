#pragma once
#include <string>

namespace ll::event {

class Event {
protected:
    bool mCancelled = false;

public:
    virtual ~Event() = default;

    bool isCancelled() const { return mCancelled; }
    void setCancelled(bool cancel = true) { mCancelled = cancel; }
};

} // namespace ll::event
