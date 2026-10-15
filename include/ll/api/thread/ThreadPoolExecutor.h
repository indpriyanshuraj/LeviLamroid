#pragma once
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace ll::thread {

class ThreadPoolExecutor {
private:
    std::string mName;
    std::vector<std::thread> mWorkers;
    std::queue<std::function<void()>> mTasks;
    std::mutex mQueueMutex;
    std::condition_variable mCondition;
    std::atomic<bool> mStop{false};

public:
    ThreadPoolExecutor(std::string name, size_t threads = 1) : mName(std::move(name)) {
        for (size_t i = 0; i < threads; ++i) {
            mWorkers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mQueueMutex);
                        mCondition.wait(lock, [this] {
                            return mStop.load() || !mTasks.empty();
                        });
                        if (mStop.load() && mTasks.empty()) {
                            return;
                        }
                        task = std::move(mTasks.front());
                        mTasks.pop();
                    }
                    if (task) {
                        task();
                    }
                }
            });
        }
    }

    ~ThreadPoolExecutor() {
        destroy();
    }

    void execute(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            if (mStop.load()) return;
            mTasks.push(std::move(task));
        }
        mCondition.notify_one();
    }

    void destroy() {
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            if (mStop.load()) return;
            mStop.store(true);
        }
        mCondition.notify_all();
        for (std::thread& worker : mWorkers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        mWorkers.clear();
    }
};

} // namespace ll::thread
