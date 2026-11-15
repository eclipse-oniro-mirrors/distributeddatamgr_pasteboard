/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_PASTEBOARD_STORE_OBSERVER_H
#define DISTRIBUTED_PASTEBOARD_STORE_OBSERVER_H

#include "kvstore_observer.h"

#include "common/block_object.h"
#include "serializable/serializable.h"

namespace OHOS::MiscServices {
class StoreObserver : public DistributedKv::KvStoreObserver {
public:
    struct RequestEvent final : public DistributedData::Serializable {
        uint32_t recordId = 0;
        uint16_t seqId = 0;
        uint8_t version = 0;
        std::string deviceId;
        std::string targetDeviceId;
        std::string dataType;
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        std::string ToString() const;
    };

    struct ResponseEvent final : public DistributedData::Serializable {
        int32_t result = 0;
        uint32_t recordId = 0;
        uint16_t seqId = 0;
        uint8_t version = 0;
        std::string dataType;
        ResponseEvent(int32_t defaultResult = 0) : result(defaultResult) {}
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
        std::string ToString() const;
    };

    using ChangeNotification = DistributedKv::ChangeNotification;
    using Value = DistributedKv::Value;
    using RequestHandler = std::function<void(RequestEvent)>;

    StoreObserver(int32_t invalidValue = 0, uint32_t interval = 0) : result_(interval, invalidValue)
    {
    }

    virtual ~StoreObserver() = default;
    void OnChange(const ChangeNotification &changeNotification) override;
    void InitRequestHandler(const RequestHandler &handler);
    void InitInterval(uint32_t interval);
    ResponseEvent GetResponseEvent();

private:
    RequestHandler handler_;
    BlockObject<ResponseEvent> result_;
};
} // namespace OHOS::MiscServices
#endif // DISTRIBUTED_PASTEBOARD_STORE_OBSERVER_H