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

#ifndef FAST_SERIALIZER_H
#define FAST_SERIALIZER_H

#include <vector>
#include <cstring>
#include <type_traits>
#include <string_view>
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

class FastSerializer {
public:
    struct Stats {
        size_t totalSerialized = 0;
        size_t totalDeserialized = 0;
        size_t totalBytes = 0;
        size_t avgTimeUs = 0;
    };
    
    explicit FastSerializer(size_t initialCapacity = 4096)
    {
        buffer_.reserve(initialCapacity);
    }
    
    void Reset()
    {
        buffer_.clear();
        offset_ = 0;
    }
    
    // 基础类型序列化（零拷贝）
    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type
    Write(const T& value)
    {
        size_t size = sizeof(T);
        EnsureCapacity(size);
        
        std::memcpy(buffer_.data() + offset_, &value, size);
        offset_ += size;
        
        stats_.totalBytes += size;
    }
    
    // 字符串视图序列化（避免拷贝）
    void WriteString(std::string_view str)
    {
        uint32_t size = static_cast<uint32_t>(str.size());
        Write(size);
        
        EnsureCapacity(size);
        std::memcpy(buffer_.data() + offset_, str.data(), size);
        offset_ += size;
        
        stats_.totalBytes += size;
    }
    
    // 字节数组序列化
    void WriteBytes(const uint8_t* data, size_t size)
    {
        uint32_t len = static_cast<uint32_t>(size);
        Write(len);
        
        EnsureCapacity(len);
        std::memcpy(buffer_.data() + offset_, data, len);
        offset_ += len;
        
        stats_.totalBytes += len;
    }
    
    // 基础类型反序列化
    template<typename T>
    typename std::enable_if<std::is_trivially_copyable<T>::value>::type
    Read(T& value)
    {
        size_t size = sizeof(T);
        
        if (offset_ + size > buffer_.size()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "FastSerializer read overflow");
            return;
        }
        
        std::memcpy(&value, buffer_.data() + offset_, size);
        offset_ += size;
    }
    
    // 字符串反序列化
    std::string_view ReadString()
    {
        uint32_t size;
        Read(size);
        
        if (offset_ + size > buffer_.size()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "FastSerializer read string overflow");
            return {};
        }
        
        const char* data = reinterpret_cast<const char*>(buffer_.data() + offset_);
        offset_ += size;
        
        return std::string_view(data, size);
    }
    
    // 字节数组反序列化
    std::pair<const uint8_t*, size_t> ReadBytes()
    {
        uint32_t size;
        Read(size);
        
        if (offset_ + size > buffer_.size()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "FastSerializer read bytes overflow");
            return {nullptr, 0};
        }
        
        const uint8_t* data = buffer_.data() + offset_;
        offset_ += size;
        
        return {data, size};
    }
    
    const std::vector<uint8_t>& GetBuffer() const
    {
        return buffer_;
    }
    
    std::vector<uint8_t>&& TakeBuffer()
    {
        return std::move(buffer_);
    }
    
    void SetBuffer(std::vector<uint8_t> buffer)
    {
        buffer_ = std::move(buffer);
        offset_ = 0;
    }
    
    size_t GetSize() const
    {
        return offset_;
    }
    
    bool HasMore() const
    {
        return offset_ < buffer_.size();
    }
    
    Stats GetStats() const
    {
        return stats_;
    }

private:
    void EnsureCapacity(size_t additionalSize)
    {
        size_t requiredSize = offset_ + additionalSize;
        
        if (buffer_.capacity() < requiredSize) {
            buffer_.reserve(requiredSize * 2);
        }
        
        if (buffer_.size() < requiredSize) {
            buffer_.resize(requiredSize);
        }
    }
    
    std::vector<uint8_t> buffer_;
    size_t offset_ = 0;
    Stats stats_;
};

// 快速TLV序列化器
class TlvSerializer {
public:
    enum class Type : uint8_t {
        INT32 = 1,
        UINT32 = 2,
        INT64 = 3,
        UINT64 = 4,
        STRING = 5,
        BYTES = 6,
        FLOAT = 7,
        DOUBLE = 8,
        BOOL = 9
    };
    
    struct Tag {
        Type type;
        uint16_t id;
    };
    
    void WriteTag(Tag tag)
    {
        serializer_.Write(static_cast<uint8_t>(tag.type));
        serializer_.Write(tag.id);
    }
    
    void WriteInt32(Tag tag, int32_t value)
    {
        WriteTag(tag);
        serializer_.Write(value);
    }
    
    void WriteUInt32(Tag tag, uint32_t value)
    {
        WriteTag(tag);
        serializer_.Write(value);
    }
    
    void WriteInt64(Tag tag, int64_t value)
    {
        WriteTag(tag);
        serializer_.Write(value);
    }
    
    void WriteString(Tag tag, std::string_view value)
    {
        WriteTag(tag);
        serializer_.WriteString(value);
    }
    
    void WriteBytes(Tag tag, const uint8_t* data, size_t size)
    {
        WriteTag(tag);
        serializer_.WriteBytes(data, size);
    }
    
    void WriteBool(Tag tag, bool value)
    {
        WriteTag(tag);
        serializer_.Write(static_cast<uint8_t>(value ? 1 : 0));
    }
    
    bool ReadTag(Tag& tag)
    {
        uint8_t type;
        serializer_.Read(type);
        tag.type = static_cast<Type>(type);
        serializer_.Read(tag.id);
        return true;
    }
    
    bool ReadInt32(Tag expectedTag, int32_t& value)
    {
        Tag tag;
        if (!ReadTag(tag) || tag.type != expectedTag.type || tag.id != expectedTag.id) {
            return false;
        }
        serializer_.Read(value);
        return true;
    }
    
    bool ReadString(Tag expectedTag, std::string_view& value)
    {
        Tag tag;
        if (!ReadTag(tag) || tag.type != expectedTag.type || tag.id != expectedTag.id) {
            return false;
        }
        value = serializer_.ReadString();
        return true;
    }
    
    const std::vector<uint8_t>& GetBuffer() const
    {
        return serializer_.GetBuffer();
    }
    
    void SetBuffer(std::vector<uint8_t> buffer)
    {
        serializer_.SetBuffer(std::move(buffer));
    }
    
    void Reset()
    {
        serializer_.Reset();
    }

private:
    FastSerializer serializer_;
};

// 批量序列化辅助类
class BatchSerializer {
public:
    explicit BatchSerializer(size_t batchSize = 64 * 1024)
        : batchSize_(batchSize)
    {
        buffer_.reserve(batchSize);
    }
    
    void Add(const std::vector<uint8_t>& data)
    {
        size_t newSize = buffer_.size() + data.size();
        
        if (newSize > batchSize_) {
            Flush();
        }
        
        buffer_.insert(buffer_.end(), data.begin(), data.end());
        count_++;
    }
    
    void Flush()
    {
        if (buffer_.empty()) {
            return;
        }
        
        // 处理批量数据
        ProcessBatch(buffer_);
        
        buffer_.clear();
        buffer_.reserve(batchSize_);
        count_ = 0;
    }
    
    size_t GetCount() const
    {
        return count_;
    }
    
    size_t GetSize() const
    {
        return buffer_.size();
    }

private:
    virtual void ProcessBatch(const std::vector<uint8_t>& batch)
    {
        // 子类实现
    }
    
    std::vector<uint8_t> buffer_;
    size_t batchSize_;
    size_t count_ = 0;
};

} // namespace MiscServices
} // namespace OHOS

#endif // FAST_SERIALIZER_H