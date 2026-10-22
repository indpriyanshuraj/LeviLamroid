#pragma once

#include "ll/api/event/Event.h"

namespace ll::event {
template <class T>
class Cancellable : public T {
protected:
    using T::T;

public:
    [[nodiscard]] constexpr bool isCancelled() const { return T::isCancelled(); }

    constexpr void setCancelled(bool cancelled = true) { T::setCancelled(cancelled); }

    constexpr void cancel() { setCancelled(true); }
};
} // namespace ll::event
