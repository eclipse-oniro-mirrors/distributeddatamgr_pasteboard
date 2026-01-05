/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICES_FRAMEWORK_CLIPS_PLUGIN_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICES_FRAMEWORK_CLIPS_PLUGIN_H
#include <map>

#include "serializable/serializable.h"

namespace OHOS::MiscServices {
class API_EXPORT ClipPlugin {
public:
    enum EventStatus : uint32_t { EVT_UNKNOWN, EVT_INVALID, EVT_NORMAL, EVT_BUTT };
    enum ServiceStatus : uint32_t { UNKNOWN = 0, IDLE, CONNECT_SUCC };

    struct GlobalEvent final : public DistributedData::Serializable {
        uint8_t version = 0;
        uint8_t frameNum = 0;
        uint16_t user = 0;
        uint16_t seqId = 0;
        uint16_t status = EVT_UNKNOWN;
        int32_t syncTime = 0;
        uint32_t dataId = 0;
        uint64_t expiration = 0;
        bool isDelay = false;
        std::string deviceId;
        std::string account;
        std::vector<std::string> dataType;

        bool operator==(const GlobalEvent globalEvent)
        {
            return globalEvent.seqId == this->seqId && globalEvent.deviceId == this->deviceId;
        }
        bool Marshal(json &node) const override;
        bool Unmarshal(const json &node) override;
    };

    class Factory {
    public:
        virtual ClipPlugin *Create() = 0;
        virtual bool Destroy(ClipPlugin *) = 0;
    };

    using DelayDataCallback = std::function<int32_t(const GlobalEvent &, uint8_t, std::vector<uint8_t> &)>;
    using DelayEntryCallback = std::function<int32_t(const GlobalEvent &, uint32_t, const std::string &,
        std::vector<uint8_t> &)>;
    using PreSyncCallback = std::function<void(const std::string &, ClipPlugin *)>;
    using PreSyncMonitorCallback = std::function<void(void)>;

    static bool RegCreator(const std::string &name, Factory *factory);
    static ClipPlugin *CreatePlugin(const std::string &name);
    static bool DestroyPlugin(const std::string &name, ClipPlugin *plugin);

    virtual ~ClipPlugin() = default;
    virtual int32_t SetPasteData(const GlobalEvent &event, const std::vector<uint8_t> &data) = 0;
    virtual std::pair<int32_t, int32_t> GetPasteData(const GlobalEvent &event, std::vector<uint8_t> &data) = 0;
    virtual std::vector<GlobalEvent> GetTopEvents(uint32_t topN);
    virtual std::vector<GlobalEvent> GetTopEvents(uint32_t topN, int32_t user);
    virtual void Clear();
    virtual int32_t ApplyAdvancedResource(const std::string &deviceId);
    virtual int32_t PublishServiceState(const std::string &networkId, ServiceStatus status);
    virtual void Clear(int32_t user);
    virtual int32_t Close(int32_t user);
    virtual void RegisterDelayCallback(const DelayDataCallback &dataCallback, const DelayEntryCallback &entryCallback);
    virtual int32_t GetPasteDataEntry(const GlobalEvent &event, uint32_t recordId, const std::string &utdId,
        std::vector<uint8_t> &rawData);
    virtual void ChangeStoreStatus(int32_t userId);
    virtual bool NeedSyncTopEvent();
    virtual void RegisterPreSyncCallback(const PreSyncCallback &callback);
    virtual void RegisterPreSyncMonitorCallback(const PreSyncMonitorCallback &callback);
    virtual void SendPreSyncEvent(int32_t userId);
    virtual void SetMaxLocalCapacity(int64_t maxLocalCapacity);

private:
    static std::map<std::string, Factory *> factories_;
};
} // namespace OHOS::MiscServices
#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_SERVICES_FRAMEWORK_CLIPS_PLUGIN_H
