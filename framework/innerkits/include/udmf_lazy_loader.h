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

#ifndef UDMF_LAZY_LOADER_H
#define UDMF_LAZY_LOADER_H

#include <vector>
#include <memory>
#include <mutex>
#include "unified_data.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

class UdmfLazyLoader {
public:
    static UdmfLazyLoader& GetInstance()
    {
        static UdmfLazyLoader instance;
        return instance;
    }

    std::shared_ptr<UDMF::UnifiedData> AcquireUnifiedData()
    {
        std::lock_guard<std::mutex> lock(poolMutex_);
        
        if (dataPool_.empty()) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Create new UnifiedData");
            return std::make_shared<UDMF::UnifiedData>();
        }
        
        auto data = dataPool_.back();
        dataPool_.pop_back();
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
            "Reuse UnifiedData from pool, pool size: %{public}zu", dataPool_.size());
        return data;
    }

    void ReleaseUnifiedData(const std::shared_ptr<UDMF::UnifiedData>& data)
    {
        if (!data) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(poolMutex_);
        
        constexpr size_t MAX_POOL_SIZE = 10;
        if (dataPool_.size() < MAX_POOL_SIZE) {
            // 创建副本添加到池中，避免修改传入的 shared_ptr
            auto pooled = std::make_shared<UDMF::UnifiedData>(*data);
            pooled->Reset();
            dataPool_.push_back(pooled);
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, 
                "Return UnifiedData to pool, pool size: %{public}zu", dataPool_.size());
        } else {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Pool full, discard UnifiedData");
        }
    }

    void ClearPool()
    {
        std::lock_guard<std::mutex> lock(poolMutex_);
        dataPool_.clear();
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "Clear UnifiedData pool");
    }

    size_t GetPoolSize() const
    {
        std::lock_guard<std::mutex> lock(poolMutex_);
        return dataPool_.size();
    }

private:
    UdmfLazyLoader() = default;
    ~UdmfLazyLoader() = default;
    UdmfLazyLoader(const UdmfLazyLoader&) = delete;
    UdmfLazyLoader& operator=(const UdmfLazyLoader&) = delete;

    std::vector<std::shared_ptr<UDMF::UnifiedData>> dataPool_;
    mutable std::mutex poolMutex_;
};

class ScopedUnifiedData {
public:
    ScopedUnifiedData() : data_(UdmfLazyLoader::GetInstance().AcquireUnifiedData()) {}
    
    ~ScopedUnifiedData()
    {
        if (data_) {
            UdmfLazyLoader::GetInstance().ReleaseUnifiedData(data_);
        }
    }

    UDMF::UnifiedData* operator->() { return data_.get(); }
    const UDMF::UnifiedData* operator->() const { return data_.get(); }
    UDMF::UnifiedData& operator*() { return *data_; }
    const UDMF::UnifiedData& operator*() const { return *data_; }
    operator bool() const { return data_ != nullptr; }

    std::shared_ptr<UDMF::UnifiedData> Get() const { return data_; }

private:
    std::shared_ptr<UDMF::UnifiedData> data_;
};

} // namespace MiscServices
} // namespace OHOS

#endif // UDMF_LAZY_LOADER_H