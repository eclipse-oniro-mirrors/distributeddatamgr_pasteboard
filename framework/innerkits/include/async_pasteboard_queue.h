/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ASYNC_PASTEBOARD_QUEUE_H
#define ASYNC_PASTEBOARD_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <atomic>
#include <future>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

class AsyncPasteboardQueue {
public:
    using Task = std::function<void()>;
    using Callback = std::function<void(int32_t)>;
    
    struct AsyncTask {
        Task task;
        Callback callback;
        std::string description;
        int priority = 0;
    };
    
    struct Stats {
        size_t totalTasks = 0;
        size_t completedTasks = 0;
        size_t failedTasks = 0;
        size_t queueSize = 0;
        size_t avgExecutionTimeMs = 0;
    };
    
    static AsyncPasteboardQueue& GetInstance()
    {
        static AsyncPasteboardQueue instance;
        return instance;
    }
    
    void Start(size_t threadCount = 2)
    {
        if (running_) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "AsyncPasteboardQueue already running");
            return;
        }
        
        running_ = true;
        
        for (size_t i = 0; i < threadCount; i++) {
            workers_.emplace_back([this, i]() {
                WorkerLoop(i);
            });
        }
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
            "AsyncPasteboardQueue started with %{public}zu threads", threadCount);
    }
    
    void Stop()
    {
        running_ = false;
        cv_.notify_all();
        
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers_.clear();
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AsyncPasteboardQueue stopped");
    }
    
    std::future<int32_t> Enqueue(Task task, const std::string& description = "", int priority = 0)
    {
        auto promise = std::make_shared<std::promise<int32_t>>();
        auto future = promise->get_future();
        
        AsyncTask asyncTask;
        asyncTask.task = task;
        asyncTask.description = description;
        asyncTask.priority = priority;
        asyncTask.callback = [promise](int32_t result) {
            promise->set_value(result);
        };
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (priority > 0) {
                // 高优先级任务插入队首
                queue_.push_front(asyncTask);
            } else {
                queue_.push_back(asyncTask);
            }
            
            stats_.totalTasks++;
            stats_.queueSize = queue_.size();
        }
        
        cv_.notify_one();
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "Task enqueued: desc=%{public}s, priority=%{public}d, queue=%{public}zu",
            description.c_str(), priority, stats_.queueSize);
        
        return future;
    }
    
    void EnqueueWithCallback(Task task, Callback callback, const std::string& description = "", int priority = 0)
    {
        AsyncTask asyncTask;
        asyncTask.task = task;
        asyncTask.callback = callback;
        asyncTask.description = description;
        asyncTask.priority = priority;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (priority > 0) {
                queue_.push_front(asyncTask);
            } else {
                queue_.push_back(asyncTask);
            }
            
            stats_.totalTasks++;
            stats_.queueSize = queue_.size();
        }
        
        cv_.notify_one();
    }
    
    size_t GetQueueSize() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    void WaitAll()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        doneCv_.wait(lock, [this]() {
            return queue_.empty() && activeTasks_ == 0;
        });
    }

private:
    AsyncPasteboardQueue() = default;
    ~AsyncPasteboardQueue()
    {
        Stop();
    }
    
    void WorkerLoop(size_t workerId)
    {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "Worker %{public}zu started", workerId);
        
        while (running_) {
            AsyncTask task;
            
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() {
                    return !queue_.empty() || !running_;
                });
                
                if (!running_ && queue_.empty()) {
                    break;
                }
                
                if (queue_.empty()) {
                    continue;
                }
                
                task = queue_.front();
                queue_.pop_front();
                activeTasks_++;
                stats_.queueSize = queue_.size();
            }
            
            int32_t result = 0;
            auto startTime = std::chrono::steady_clock::now();
            
            try {
                task.task();
                result = 0;
                stats_.completedTasks++;
                
                PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
                    "Worker %{public}zu completed: %{public}s", workerId, task.description.c_str());
                
            } catch (const std::exception& e) {
                result = -1;
                stats_.failedTasks++;
                
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                    "Worker %{public}zu failed: %{public}s, error: %{public}s",
                    workerId, task.description.c_str(), e.what());
            } catch (...) {
                result = -2;
                stats_.failedTasks++;
                
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                    "Worker %{public}zu failed: %{public}s, unknown error",
                    workerId, task.description.c_str());
            }
            
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            {
                std::lock_guard<std::mutex> lock(mutex_);
                activeTasks_--;
                
                if (stats_.avgExecutionTimeMs == 0) {
                    stats_.avgExecutionTimeMs = duration.count();
                } else {
                    stats_.avgExecutionTimeMs = (stats_.avgExecutionTimeMs + duration.count()) / 2;
                }
            }
            
            if (task.callback) {
                task.callback(result);
            }
            
            doneCv_.notify_all();
        }
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "Worker %{public}zu stopped", workerId);
    }
    
    std::deque<AsyncTask> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable doneCv_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> activeTasks_{0};
    mutable Stats stats_;
};

// RAII 风格的异步任务管理
class ScopedAsyncTask {
public:
    explicit ScopedAsyncTask(AsyncPasteboardQueue::Task task, const std::string& description = "")
        : future_(AsyncPasteboardQueue::GetInstance().Enqueue(task, description)) {}
    
    ~ScopedAsyncTask()
    {
        if (future_.valid()) {
            future_.wait();
        }
    }
    
    int32_t GetResult()
    {
        if (future_.valid()) {
            return future_.get();
        }
        return -1;
    }
    
    bool IsReady() const
    {
        return future_.valid() && 
            future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
    
private:
    std::future<int32_t> future_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // ASYNC_PASTEBOARD_QUEUE_H