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

#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>
#include <ctime>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

enum class MemoryPressureLevel {
    NONE = 0,
    MODERATE = 1,   // 中等压力：清理缓存
    CRITICAL = 2    // 严重压力：释放所有可释放资源
};

class MemoryMonitor {
public:
    using CleanupCallback = std::function<void(size_t)>;
    
    struct Config {
        size_t checkInterval = 5; // 检查间隔（秒）
        size_t moderateThreshold = 70; // 中等压力阈值（%）
        size_t criticalThreshold = 85; // 严重压力阈值（%）
        bool enableAutoCleanup = true;
    };
    
    struct Stats {
        size_t totalChecks = 0;
        size_t moderateCount = 0;
        size_t criticalCount = 0;
        size_t cleanupCount = 0;
        size_t memoryReleased = 0; // KB
        MemoryPressureLevel lastLevel = MemoryPressureLevel::NONE;
        std::time_t lastCheckTime = 0;
    };
    
    static MemoryMonitor& GetInstance()
    {
        static MemoryMonitor instance;
        return instance;
    }
    
    void Start()
    {
        if (running_) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "Memory monitor already running");
            return;
        }
        
        running_ = true;
        monitorThread_ = std::thread([this]() {
            MonitorLoop();
        });
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Memory monitor started");
    }
    
    void Stop()
    {
        running_ = false;
        
        if (monitorThread_.joinable()) {
            monitorThread_.join();
        }
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Memory monitor stopped");
    }
    
    void RegisterCleanupCallback(CleanupCallback callback)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cleanupCallbacks_.push_back(callback);
    }
    
    MemoryPressureLevel CheckMemoryPressure()
    {
        size_t memoryUsage = GetSystemMemoryUsage();
        
        MemoryPressureLevel level;
        if (memoryUsage >= config_.criticalThreshold) {
            level = MemoryPressureLevel::CRITICAL;
            stats_.criticalCount++;
        } else if (memoryUsage >= config_.moderateThreshold) {
            level = MemoryPressureLevel::MODERATE;
            stats_.moderateCount++;
        } else {
            level = MemoryPressureLevel::NONE;
        }
        
        stats_.totalChecks++;
        stats_.lastLevel = level;
        stats_.lastCheckTime = std::time(nullptr);
        
        if (level != MemoryPressureLevel::NONE) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT,
                "Memory pressure detected: level=%{public}d, usage=%{public}zu%%",
                static_cast<int>(level), memoryUsage);
        }
        
        return level;
    }
    
    size_t Cleanup(MemoryPressureLevel level)
    {
        if (!config_.enableAutoCleanup) {
            return 0;
        }
        
        size_t targetReduction = CalculateTargetReduction(level);
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
            "Starting cleanup: level=%{public}d, target=%{public}zu KB",
            static_cast<int>(level), targetReduction);
        
        size_t released = 0;
        
        for (auto& callback : cleanupCallbacks_) {
            try {
                callback(targetReduction);
                released += targetReduction; // 简化计算
            } catch (const std::exception& e) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                    "Cleanup callback failed: %{public}s", e.what());
            }
        }
        
        stats_.cleanupCount++;
        stats_.memoryReleased += released;
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
            "Cleanup completed: released=%{public}zu KB", released);
        
        return released;
    }
    
    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    void SetConfig(const Config& config)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }
    
    size_t GetProcessMemoryUsage() const
    {
        // 读取 /proc/self/status
        FILE* fp = fopen("/proc/self/status", "r");
        if (!fp) {
            return 0;
        }
        
        char line[256];
        size_t vmRSS = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "VmRSS: %zu kB", &vmRSS) == 1) {
                break;
            }
        }
        
        fclose(fp);
        return vmRSS;
    }
    
private:
    MemoryMonitor() = default;
    ~MemoryMonitor()
    {
        Stop();
    }
    
    void MonitorLoop()
    {
        while (running_) {
            try {
                MemoryPressureLevel level = CheckMemoryPressure();
                
                if (level != MemoryPressureLevel::NONE) {
                    Cleanup(level);
                }
                
                if (stats_.totalChecks % 12 == 0) {
                    ReportStats();
                }
                
            } catch (const std::exception& e) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                    "Exception in monitor loop: %{public}s", e.what());
            } catch (...) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT,
                    "Unknown exception in monitor loop");
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(config_.checkInterval));
        }
    }
    
    size_t GetSystemMemoryUsage() const
    {
        FILE* fp = fopen("/proc/meminfo", "r");
        if (!fp) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "Failed to open /proc/meminfo");
            return 0;
        }
        
        char line[256];
        size_t total = 0;
        size_t available = 0;
        size_t value = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            if (sscanf(line, "MemTotal: %zu kB", &value) == 1) {
                total = value;
            } else if (sscanf(line, "MemAvailable: %zu kB", &value) == 1) {
                available = value;
            }
        }
        
        fclose(fp);
        
        if (total == 0) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, 
                "Failed to parse MemTotal from /proc/meminfo");
            return 0;
        }
        
        size_t used = total - available;
        size_t percentage = (used * 100) / total;
        
        return percentage;
    }
    
    size_t CalculateTargetReduction(MemoryPressureLevel level) const
    {
        switch (level) {
            case MemoryPressureLevel::MODERATE:
                return 10 * 1024; // 10 MB
            case MemoryPressureLevel::CRITICAL:
                return 50 * 1024; // 50 MB
            default:
                return 0;
        }
    }
    
    void ReportStats() const
    {
        size_t processMemory = GetProcessMemoryUsage();
        
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT,
            "Memory stats: checks=%{public}zu, moderate=%{public}zu, "
            "critical=%{public}zu, cleanups=%{public}zu, "
            "released=%{public}zu KB, process=%{public}zu KB",
            stats_.totalChecks, stats_.moderateCount, stats_.criticalCount,
            stats_.cleanupCount, stats_.memoryReleased, processMemory);
    }
    
    std::thread monitorThread_;
    std::atomic<bool> running_{false};
    std::vector<CleanupCallback> cleanupCallbacks_;
    mutable std::mutex mutex_;
    Config config_;
    mutable Stats stats_;
};

// RAII 风格的内存监控
class ScopedMemoryMonitor {
public:
    ScopedMemoryMonitor()
    {
        MemoryMonitor::GetInstance().Start();
    }
    
    ~ScopedMemoryMonitor()
    {
        MemoryMonitor::GetInstance().Stop();
    }
};

} // namespace MiscServices
} // namespace OHOS

#endif // MEMORY_MONITOR_H