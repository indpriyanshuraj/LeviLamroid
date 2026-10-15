#pragma once
#include "Event.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <typeindex>

namespace ll::event {

class Listener {
public:
    virtual ~Listener() = default;
};

using ListenerPtr = std::shared_ptr<Listener>;

template <typename E>
class TypedListener : public Listener {
public:
    using Callback = std::function<void(E&)>;
    Callback callback;

    explicit TypedListener(Callback cb) : callback(std::move(cb)) {}
};

class EventBus {
private:
    std::mutex mMutex;
    std::unordered_map<std::type_index, std::vector<std::weak_ptr<Listener>>> mListeners;

    EventBus() = default;

public:
    static EventBus& getInstance() {
        static EventBus instance;
        return instance;
    }

    template <typename E>
    ListenerPtr subscribe(std::function<void(E&)> callback) {
        std::lock_guard<std::mutex> lock(mMutex);
        auto listener = std::make_shared<TypedListener<E>>(std::move(callback));
        mListeners[std::type_index(typeid(E))].push_back(listener);
        return listener;
    }

    template <typename E>
    bool publish(E& event) {
        std::vector<std::shared_ptr<Listener>> activeListeners;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            auto it = mListeners.find(std::type_index(typeid(E)));
            if (it != mListeners.end()) {
                auto& vec = it->second;
                for (auto listIt = vec.begin(); listIt != vec.end();) {
                    if (auto sp = listIt->lock()) {
                        activeListeners.push_back(sp);
                        ++listIt;
                    } else {
                        listIt = vec.erase(listIt);
                    }
                }
            }
        }

        for (auto& l : activeListeners) {
            auto typed = std::static_pointer_cast<TypedListener<E>>(l);
            typed->callback(event);
            if (event.isCancelled()) {
                return false;
            }
        }
        return true;
    }
};

} // namespace ll::event
