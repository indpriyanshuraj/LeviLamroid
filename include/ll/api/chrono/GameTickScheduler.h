#pragma once
#include <functional>
#include <vector>
#include <mutex>
#include <cstdint>

namespace ll::chrono {

using TaskId = uint64_t;
using TaskCallback = std::function<void()>;

struct ScheduledTask {
    TaskId id;
    uint64_t executeTick;
    uint64_t interval; // 0 for one-shot
    TaskCallback callback;
    bool cancelled;
};

class GameTickScheduler {
private:
    uint64_t mCurrentTick = 0;
    uint64_t mNextId = 1;
    std::vector<ScheduledTask> mTasks;
    mutable std::mutex mMutex;

    GameTickScheduler() = default;

public:
    static GameTickScheduler& getInstance() {
        static GameTickScheduler instance;
        return instance;
    }

    uint64_t getCurrentTick() const {
        std::lock_guard<std::mutex> lock(mMutex);
        return mCurrentTick;
    }

    TaskId schedule(uint64_t delayTicks, TaskCallback cb) {
        std::lock_guard<std::mutex> lock(mMutex);
        TaskId id = mNextId++;
        mTasks.push_back({id, mCurrentTick + delayTicks, 0, std::move(cb), false});
        return id;
    }

    TaskId scheduleRepeating(uint64_t intervalTicks, TaskCallback cb) {
        std::lock_guard<std::mutex> lock(mMutex);
        TaskId id = mNextId++;
        mTasks.push_back({id, mCurrentTick + intervalTicks, intervalTicks, std::move(cb), false});
        return id;
    }

    void cancel(TaskId id) {
        std::lock_guard<std::mutex> lock(mMutex);
        for (auto& task : mTasks) {
            if (task.id == id) {
                task.cancelled = true;
                break;
            }
        }
    }

    void tick() {
        std::vector<TaskCallback> toRun;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mCurrentTick++;
            for (auto it = mTasks.begin(); it != mTasks.end(); ) {
                if (it->cancelled) {
                    it = mTasks.erase(it);
                } else if (mCurrentTick >= it->executeTick) {
                    toRun.push_back(it->callback);
                    if (it->interval > 0) {
                        it->executeTick = mCurrentTick + it->interval;
                        ++it;
                    } else {
                        it = mTasks.erase(it);
                    }
                } else {
                    ++it;
                }
            }
        }
        for (auto& cb : toRun) {
            if (cb) cb();
        }
    }
};

inline TaskId scheduleTask(uint64_t delayTicks, TaskCallback cb) {
    return GameTickScheduler::getInstance().schedule(delayTicks, std::move(cb));
}

inline TaskId scheduleRepeatingTask(uint64_t intervalTicks, TaskCallback cb) {
    return GameTickScheduler::getInstance().scheduleRepeating(intervalTicks, std::move(cb));
}

inline void cancelTask(TaskId id) {
    GameTickScheduler::getInstance().cancel(id);
}

} // namespace ll::chrono
