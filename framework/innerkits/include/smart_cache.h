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

#ifndef SMART_CACHE_H
#define SMART_CACHE_H

#include <map>
#include <mutex>
#include <memory>
#include <ctime>
#include <algorithm>
#include <functional>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

template<typename K, typename V>
class SmartCache {
public:
    using Factory = std::function<std::shared_ptr<V>(const K&)>;
    
    struct Config {
        size_t maxSize = 100;
        size_t ttl = 300; // seconds
        size_t memoryThreshold = 80; // percentage
        bool enableEviction = true;
    };
    
    struct Stats {
        size_t hits = 0;
        size_t misses = 0;
        size_t evictions = 0;
        size_t currentSize = 0;
        
        float HitRate() const {
            size_t total = hits + misses;
            return total > 0 ? (float)hits / total : 0.0f;
        }
    };
    
    explicit SmartCache(const Config& config, Factory factory = nullptr)
        : config_(config), factory_(factory) {}
    
    ~SmartCache() = default;
    
    std::shared_ptr<V> Get(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            // Cache hit
            if (!IsExpired(it->second)) {
                it->second.accessCount++;
                stats_.hits++;
                PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
                    "Cache hit: key=%{public}s, hits=%{public}zu", 
                    ToString(key).c_str(), stats_.hits);
                return it->second.value;
            } else {
                // Expired
                cache_.erase(it);
            }
        }
        
        // Cache miss
        stats_.misses++;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
            "Cache miss: key=%{public}s, misses=%{public}zu",
            ToString(key).c_str(), stats_.misses);
        
        return nullptr;
    }
    
    std::shared_ptr<V> GetOrLoad(const K& key)
    {
        auto cached = Get(key);
        if (cached) {
            return cached;
        }
        
        // Load from factory
        if (factory_) {
            auto value = factory_(key);
            if (value) {
                Put(key, value);
            }
            return value;
        }
        
        return nullptr;
    }
    
    void Put(const K& key, std::shared_ptr<V> value)
    {
        if (!value) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check memory pressure
        if (config_.enableEviction && 
            (cache_.size() >= config_.maxSize || IsMemoryPressure())) {
            Evict();
        }
        
        CacheEntry entry;
        entry.value = value;
        entry.timestamp = std::time(nullptr);
        entry.accessCount = 0;
        
        cache_[key] = entry;
        stats_.currentSize = cache_.size();
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "Cache put: key=%{public}s, size=%{public}zu",
            ToString(key).c_str(), cache_.size());
    }
    
    void Remove(const K& key)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            cache_.erase(it);
            stats_.currentSize = cache_.size();
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
                "Cache remove: key=%{public}s", ToString(key).c_str());
        }
    }
    
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        stats_.currentSize = 0;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Cache cleared");
    }
    
    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    void SetFactory(Factory factory)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        factory_ = factory;
    }
    
    void SetConfig(const Config& config)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
        
        // Trigger eviction if size exceeds new limit
        if (config_.enableEviction && cache_.size() > config_.maxSize) {
            while (cache_.size() > config_.maxSize) {
                EvictOne();
            }
        }
    }
    
private:
    struct CacheEntry {
        std::shared_ptr<V> value;
        std::time_t timestamp;
        size_t accessCount;
    };
    
    bool IsExpired(const CacheEntry& entry) const
    {
        std::time_t now = std::time(nullptr);
        return (now - entry.timestamp) > config_.ttl;
    }
    
    bool IsMemoryPressure() const
    {
        // In real implementation, check system memory usage
        // For now, return false
        return false;
    }
    
    void Evict()
    {
        if (cache_.empty()) {
            return;
        }
        
        // Evict multiple entries based on memory pressure
        size_t evictCount = std::max(size_t(1), cache_.size() / 10);
        
        for (size_t i = 0; i < evictCount && !cache_.empty(); i++) {
            EvictOne();
        }
        
        stats_.currentSize = cache_.size();
    }
    
    void EvictOne()
    {
        if (cache_.empty()) {
            return;
        }
        
        // LRU eviction: find least recently used
        auto victim = std::min_element(cache_.begin(), cache_.end(),
            [](const auto& a, const auto& b) {
                return a.second.accessCount < b.second.accessCount;
            });
        
        if (victim != cache_.end()) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
                "Evict: key=%{public}s, accessCount=%{public}zu",
                ToString(victim->first).c_str(), victim->second.accessCount);
            cache_.erase(victim);
            stats_.evictions++;
        }
    }
    
    std::string ToString(const K& key) const
    {
        // Generic key to string conversion
        return std::to_string(std::hash<K>{}(key));
    }
    
    std::map<K, CacheEntry> cache_;
    mutable std::mutex mutex_;
    Config config_;
    Factory factory_;
    mutable Stats stats_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // SMART_CACHE_H