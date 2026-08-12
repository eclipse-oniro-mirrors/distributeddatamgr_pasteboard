/*
 * Copyright (C) 2026-2024 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_PASTEBOARD_DEDUPLICATE_MEMORY_H
#define DISTRIBUTEDDATAMGR_PASTEBOARD_DEDUPLICATE_MEMORY_H

#include "pasteboard_linked_list.h"

namespace OHOS {
namespace MiscServices {

template <typename T>
class DeduplicateMemory {
public:
    explicit DeduplicateMemory(int64_t expirationMilliSeconds);
    ~DeduplicateMemory();
    bool IsDuplicate(const T &data);

private:
    int64_t GetTimestamp();
    void ClearExpiration();
    bool FindExist(const T &data);

    struct MemoryIdentity {
        int64_t timestamp;
        const T data;
    };

    LinkedList<MemoryIdentity> memory_;
    std::mutex mutex_;
    int64_t expirationMS_;
};

template <typename T>
DeduplicateMemory<T>::DeduplicateMemory(int64_t expirationMilliSeconds) : expirationMS_(expirationMilliSeconds)
{
}

template <typename T>
DeduplicateMemory<T>::~DeduplicateMemory()
{
    memory_.Clear();
}

template <typename T>
bool DeduplicateMemory<T>::IsDuplicate(const T &data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ClearExpiration();
    if (FindExist(data)) {
        return true;
    }
    int64_t timestamp = GetTimestamp();
    memory_.InsertFront(MemoryIdentity({.timestamp = timestamp, .data = data}));
    return false;
}

template <typename T>
bool DeduplicateMemory<T>::FindExist(const T &data)
{
    return memory_.FindExist([data](const MemoryIdentity &identity) {
        return identity.data == data;
    });
}

template <typename T>
int64_t DeduplicateMemory<T>::GetTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

template <typename T>
void DeduplicateMemory<T>::ClearExpiration()
{
    int64_t timestamp = GetTimestamp();
    if (timestamp < expirationMS_) {
        return;
    }
    int64_t expirationTimestamp = timestamp - expirationMS_;
    memory_.RemoveIf([expirationTimestamp](const MemoryIdentity &identity) {
        return expirationTimestamp > identity.timestamp;
    });
}
} // namespace MiscServices
} // namespace OHOS

#endif // DISTRIBUTEDDATAMGR_PASTEBOARD_DEDUPLICATE_MEMORY_H
