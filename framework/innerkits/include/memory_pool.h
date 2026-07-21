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

#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <vector>
#include <mutex>
#include <memory>
#include <cstring>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

template<typename T, size_t BlockSize = 4096>
class MemoryPool {
public:
    struct Stats {
        size_t totalAllocations = 0;
        size_t totalDeallocations = 0;
        size_t poolSize = 0;
        size_t freeCount = 0;
        size_t blockCount = 0;
    };
    
    static MemoryPool& GetInstance()
    {
        static MemoryPool instance;
        return instance;
    }
    
    T* Allocate()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        stats_.totalAllocations++;
        
        if (freeList_.empty()) {
            AllocateBlock();
        }
        
        if (freeList_.empty()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "MemoryPool allocation failed");
            return nullptr;
        }
        
        T* obj = freeList_.back();
        freeList_.pop_back();
        stats_.freeCount = freeList_.size();
        
        return obj;
    }
    
    void Deallocate(T* obj)
    {
        if (!obj) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        obj->~T();
        freeList_.push_back(obj);
        
        stats_.totalDeallocations++;
        stats_.freeCount = freeList_.size();
    }
    
    template<typename... Args>
    T* New(Args&&... args)
    {
        T* obj = Allocate();
        if (obj) {
            new (obj) T(std::forward<Args>(args)...);
        }
        return obj;
    }
    
    void Delete(T* obj)
    {
        if (obj) {
            obj->~T();
            Deallocate(obj);
        }
    }
    
    void Clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        freeList_.clear();
        
        for (void* block : blocks_) {
            delete[] static_cast<char*>(block);
        }
        blocks_.clear();
        
        stats_.poolSize = 0;
        stats_.freeCount = 0;
        stats_.blockCount = 0;
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "MemoryPool cleared");
    }
    
    Stats GetStats() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    size_t GetCapacity() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return blocks_.size() * (BlockSize / sizeof(T));
    }

private:
    MemoryPool() = default;
    ~MemoryPool()
    {
        Clear();
    }
    
    void AllocateBlock()
    {
        char* block = new char[BlockSize];
        blocks_.push_back(block);
        
        size_t count = BlockSize / sizeof(T);
        
        for (size_t i = 0; i < count; i++) {
            T* obj = reinterpret_cast<T*>(block + i * sizeof(T));
            freeList_.push_back(obj);
        }
        
        stats_.poolSize += count;
        stats_.blockCount = blocks_.size();
        stats_.freeCount = freeList_.size();
        
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT,
            "MemoryPool allocated block: total=%{public}zu, free=%{public}zu",
            stats_.poolSize, stats_.freeCount);
    }
    
    std::vector<T*> freeList_;
    std::vector<void*> blocks_;
    mutable std::mutex mutex_;
    mutable Stats stats_;
};

// 字符串缓冲区（避免频繁分配）
class StringBuffer {
public:
    explicit StringBuffer(size_t initialCapacity = 1024)
    {
        buffer_.reserve(initialCapacity);
    }
    
    void Append(const char* data, size_t len)
    {
        if (buffer_.size() + len > buffer_.capacity()) {
            Reserve(buffer_.size() + len);
        }
        
        buffer_.append(data, len);
    }
    
    void Append(std::string_view str)
    {
        Append(str.data(), str.size());
    }
    
    void Reserve(size_t capacity)
    {
        buffer_.reserve(capacity);
    }
    
    void Clear()
    {
        buffer_.clear();
    }
    
    const char* Data() const
    {
        return buffer_.data();
    }
    
    size_t Size() const
    {
        return buffer_.size();
    }
    
    size_t Capacity() const
    {
        return buffer_.capacity();
    }
    
    std::string ToString() const
    {
        return buffer_;
    }
    
    std::string_view ToStringView() const
    {
        return buffer_;
    }
    
private:
    std::string buffer_;
};

// 固定大小对象池
template<typename T, size_t PoolSize = 100>
class FixedObjectPool {
public:
    FixedObjectPool()
    {
        pool_.reserve(PoolSize);
        used_.resize(PoolSize, false);
    }
    
    T* Acquire()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (size_t i = 0; i < PoolSize; i++) {
            if (!used_[i]) {
                used_[i] = true;
                
                if (i >= pool_.size()) {
                    pool_.push_back(std::make_unique<T>());
                }
                
                return pool_[i].get();
            }
        }
        
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_CLIENT, "FixedObjectPool exhausted");
        return nullptr;
    }
    
    void Release(T* obj)
    {
        if (!obj) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (size_t i = 0; i < pool_.size(); i++) {
            if (pool_[i].get() == obj) {
                used_[i] = false;
                return;
            }
        }
    }
    
    size_t GetUsedCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        size_t count = 0;
        for (bool used : used_) {
            if (used) count++;
        }
        return count;
    }
    
    size_t GetCapacity() const
    {
        return PoolSize;
    }

private:
    std::vector<std::unique_ptr<T>> pool_;
    std::vector<bool> used_;
    mutable std::mutex mutex_;
};

// 智能指针包装器（支持对象池）
template<typename T>
class PooledPtr {
public:
    PooledPtr() : ptr_(nullptr), pool_(nullptr) {}
    
    PooledPtr(T* ptr, MemoryPool<T>* pool) : ptr_(ptr), pool_(pool) {}
    
    ~PooledPtr()
    {
        if (ptr_ && pool_) {
            pool_->Delete(ptr_);
        }
    }
    
    PooledPtr(const PooledPtr&) = delete;
    PooledPtr& operator=(const PooledPtr&) = delete;
    
    PooledPtr(PooledPtr&& other) noexcept : ptr_(other.ptr_), pool_(other.pool_)
    {
        other.ptr_ = nullptr;
        other.pool_ = nullptr;
    }
    
    PooledPtr& operator=(PooledPtr&& other) noexcept
    {
        if (this != &other) {
            if (ptr_ && pool_) {
                pool_->Delete(ptr_);
            }
            
            ptr_ = other.ptr_;
            pool_ = other.pool_;
            other.ptr_ = nullptr;
            other.pool_ = nullptr;
        }
        return *this;
    }
    
    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    operator bool() const { return ptr_ != nullptr; }
    
private:
    T* ptr_;
    MemoryPool<T>* pool_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // MEMORY_POOL_H