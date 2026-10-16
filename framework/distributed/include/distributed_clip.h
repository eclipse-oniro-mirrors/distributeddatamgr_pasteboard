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

#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_DISTRIBUTED_CLIP_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_DISTRIBUTED_CLIP_H

#include <condition_variable>
#include <event_handler.h>
#include <shared_mutex>
#include "all_connect_adapter.h"
#include "api/visibility.h"
#include "broadcast_manager.h"
#include "clip/clip_plugin.h"
#include "clip_para.h"
#include "device/dm_adapter.h"
#include "ffrt/ffrt_utils.h"
#include "common/block_object.h"
#include "trans_type.h"
#include "utils.h"
#include "store.h"
#include "store_observer.h"

namespace OHOS::MiscServices {
class DistributedClip;

struct PreSyncFinishInfo {
    PreSyncFinishInfo(int32_t tmpSeqId, int32_t tmpUserId, const std::string &tmpDeviceId)
        : seqId(tmpSeqId), userId(tmpUserId), deviceId(tmpDeviceId)
    {}
    int32_t seqId = 0;
    int32_t userId = 0;
    std::string deviceId;
};

class PreSyncHandler : public AppExecFwk::EventHandler {
public:
    PreSyncHandler() = default;
    ~PreSyncHandler();
    enum PreSyncEventType : uint32_t {
        MSG_START_PRESYNC_EVENT = 0,
        MSG_FINISH_PRESYNC_EVENT,
        MSG_RESOURCE_CLEAN_EVENT,
    };
private:
    void Init(DistributedClip *distributedClip);
    void ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event) override;
    void SendPreSyncTask(int32_t userId);
    void SendPreSyncFinishEvent(std::shared_ptr<PreSyncFinishInfo> finishInfo);
    friend class DistributedClip;
private:
    static const int64_t CLEAN_DELAY_MILLISECONDS = 1 * 60 * 60 * 1000; // 1 hours
    DistributedClip *distributedClip_ = nullptr;
};
class SyncCallback;

struct PreSyncCounter {
    uint32_t counter = 0;
    uint32_t lastCopySeconds = 0;
};

struct PreSyncRunning {
    bool isSyncing = false;
    uint16_t seqId = 0;
    uint32_t lastCopySeconds;
    uint64_t startSyncTime = 0;
    std::condition_variable preSyncCV;
    std::string deviceId;
    std::shared_ptr<SyncCallback> syncCallback = nullptr;
    std::mutex preSyncCounterMutex;
    std::map<std::string, std::shared_ptr<PreSyncCounter>> preSyncCounterMap; // Each device inserts a reference count.
};

struct PreSyncInfo {
    bool isPasting = false;
    int32_t userId = 0;
    std::mutex preSyncMutex;
    std::shared_ptr<PreSyncRunning> preSyncRunning = nullptr; // Each user saves a pre-synchronization structure.
};

class API_EXPORT DistributedClip : public ClipPlugin, protected DMAdapter::DMObserver {
public:
    using SendInformation = ClipPara::SendInformation;
    using SendInfoBrief = ClipPara::SendInfoBrief;
    using RequestEvent = StoreObserver::RequestEvent;
    using ResponseEvent = StoreObserver::ResponseEvent;

    static DistributedClip &GetInstance();
    DistributedClip();
    virtual ~DistributedClip();
    int32_t SetPasteData(const GlobalEvent &event, const std::vector<uint8_t> &data, uint32_t version,
        const std::vector<uint8_t> &mimeTypes) override;
    int32_t ApplyAdvancedResource(const std::string &deviceId) override;
    int32_t PublishServiceState(const std::string &networkId, ClipPlugin::ServiceStatus status) override;
    std::pair<int32_t, int32_t> GetPasteData(const GlobalEvent &event, std::vector<uint8_t> &data) override;
    std::vector<GlobalEvent> GetTopEvents(uint32_t topN, int32_t user) override;
    void SendBroadcastNotify(const GlobalEvent &event, const std::vector<uint8_t> &exData = std::vector<uint8_t>(),
        bool isClear = false, uint32_t version = 0, const std::vector<uint8_t> &mimeTypes = std::vector<uint8_t>());
    void Clear(int32_t user) override;
    int32_t Close(int32_t user) override;
    void RegisterDelayCallback(const DelayDataCallback &dataCallback, const DelayEntryCallback &entryCallback) override;
    int32_t GetPasteDataEntry(const GlobalEvent &event, uint32_t recordId, const std::string &utdId,
        std::vector<uint8_t> &rawData) override;
    void ChangeStoreStatus(int32_t userId) override;
    bool NeedSyncTopEvent() override;
    void RegisterPreSyncCallback(const PreSyncCallback &callback) override;
    void RegisterPreSyncMonitorCallback(const PreSyncMonitorCallback &callback) override;
    void SendPreSyncEvent(int32_t userId) override;
    void SetMaxLocalCapacity(int64_t maxLocalCapacity) override;
    int32_t GetMimeTypes(std::vector<uint8_t> &mimeTypes, const GlobalEvent &event) override;
    void PreSyncTopEvent(int32_t userId);
    void SendPreSyncFinishEvent(int32_t userId, int32_t seqId, const std::string &deviceId);
    void PreSyncFinish(std::shared_ptr<PreSyncFinishInfo> finishInfo);
    void ProcessPreSyncFinish(std::shared_ptr<PreSyncFinishInfo> finishInfo);
    void ClearPreSyncResource();
    bool IsWiFiEnable() override;
    void RegisterListeners(int32_t userId) override;
    void UnregisterListeners(int32_t userId) override;

protected:
    void Online(const std::string &device) override;
    void Offline(const std::string &device) override;
    void OnReady(const std::string &device) override;

private:
    static constexpr const char *APP_ID = "pasteboard_service";
    static constexpr const char *STORE_ID = "distributed_store";
    static constexpr const char *DISCONNECT_EVENT_NAME = "lifecycle";
    static constexpr const char *PASTEBOARD_SERVICE_NAME = "Pasteboard";
    static constexpr uint32_t ERR_OK = 0;
    static constexpr uint32_t EXPIRATION_INTERVAL = 2 * 60 * 1000; // 2min
    static constexpr uint32_t DATA_SYNC_INTERVAL = 30 * 1000; // 30 seconds
    static constexpr uint8_t CLEAR_BROADCAST = 0x1; // bit 0 means the type of broadcast is clear or not
    static constexpr uint8_t PLAINTEXT_BROADCAST = 0x2; // bit 1 means the data has plaintext type or not
    static constexpr uint8_t URI_BROADCAST = 0x4; // bit 2 means the data has uri type or not
    static constexpr uint8_t HTML_BROADCAST = 0x8; // bit 3 means the data has html type or not
    static constexpr uint8_t DELAY_BROADCAST = 0x10; // bit 4 means the data is delay or not
    static constexpr uint8_t PIXELMAP_BROADCAST = 0x20; // bit 5 means the data has pixelMap type or not
    static constexpr uint8_t NOTLINK_BROADCAST = 0x40; // bit 6 means not need establish p2p or not
    static constexpr uint32_t MAX_PRESYNC_USE_DEVICE_COUNTER_VALUE = 3;
    static constexpr uint32_t EVENT_DATA_MAX_LENGTH = 1300;

    void ProcessDelayRequest(const RequestEvent &reqEvent);
    void ProcessDataAndUpdateStore(const RequestEvent &reqEvent, GlobalEvent &event,
        std::vector<uint8_t> &rawData, std::shared_ptr<Store> store, int32_t result);
    bool IsExpiredBroadcast(uint16_t seqId, const std::string& senderNetworkId, uint64_t time);
    int32_t NotifyDelayResponse(const RequestEvent &reqEvent, int32_t result, const std::shared_ptr<Store> store);
    void NotifyDataReceive(const std::string &senderNetworkId, uint8_t *payload, uint32_t dataLen,
        uint8_t *exData, uint32_t exDataLen);
    std::string getAnonymousDeviceID(std::string deviceId);
    static bool RemoveDirtyData(const GlobalEvent &event, std::shared_ptr<Store> store, uint32_t frameNum = 0);
    void DeInit(bool destroy = false);
    std::shared_ptr<Store> InitializeStore(int32_t userId);
    bool InitTopEvent();
    bool InitDelayRequestListener(std::shared_ptr<Store> store);
    void SaveTopEvent(const GlobalEvent &event);
    void SaveLocalTopEvent(const GlobalEvent &event);
    void UpdateSetDataClipPara(const uint64_t expiration);
    void UpdateGetEventClipPara(const uint64_t expiration, const std::string &deviceId);
    void UpdateClearDataClipPara();
    uint8_t GetTypesDescription(const GlobalEvent &event);
    void SetTypes(GlobalEvent &event, uint8_t typesDescription);
    void UpdateDataReceiveClipPara(const GlobalEvent &event, const SendInformation &sendInformation);
    static SendInformation Convert(const SendInfoBrief &sendInfoBrief, uint64_t curTime);
    static SendInfoBrief Convert(const SendInformation &sendInformation);
    int32_t DoSync(GlobalEvent &event, const DistributedKv::DataQuery &query,
        uint32_t recordId = 0, const std::string &utdType = "");
    int32_t DoSyncPush(GlobalEvent &event, const std::string &localNetworkId, const std::shared_ptr<Store> store);
    int32_t DoSyncPull(GlobalEvent &event, const DataQuery &query);
    void RemoveRemoteData();
    GlobalEvent GetTopEvent();
    void AgingDataClear(bool isLocal);
    void OnAgingDataClear(bool isLocal, int32_t userId, std::shared_ptr<Store> store);
    void LocalAgingDataClear();
    void SyncAgingDataClear();
    std::pair<int32_t, std::shared_ptr<Store>> GetStore(int32_t hintUserId = Utils::ERROR_USERID);
    bool IsContainUri(const GlobalEvent &evt);
    void CheckPreSyncInfoAndDoPreSync(int32_t userId, const GlobalEvent &curTopEvent, bool &isNeedP2p);
    void PreP2pEstablishCallback(const std::string &networkId);
    void ClearKvStoreSyncData(const GlobalEvent &event);
    void ClearPreSyncCounter(const std::string &networkId, int32_t userId);
    void AddPreSyncMonitor(int32_t userId, const std::string &networkId);
    std::shared_ptr<PreSyncInfo> GetPreSyncInProgressByUser(int32_t userId, bool onlyGain = false);
    void UpdateLastBroadcastTimeForPreSync(int32_t userId, const std::string &networkId);
    int32_t DoPreSyncPull(std::shared_ptr<PreSyncRunning> preSyncRunning, const GlobalEvent &event);
    bool IsPreSyncCountGreaterThreshold(std::shared_ptr<PreSyncRunning> preSyncRunning, const std::string &deviceId);
    void AddPreSyncCounter(std::shared_ptr<PreSyncRunning> preSyncRunning);
    std::shared_ptr<PreSyncInfo> CheckAndWaitPreSyncFinish(const GlobalEvent &event);
    void SetTopEventPullFinish(const GlobalEvent &event);
    bool SaveExtMimeTypes(const GlobalEvent &event, const uint8_t *exData, uint32_t exDataLen);
    bool SaveExtPasteData(const GlobalEvent &event, const uint8_t *exData, uint32_t exDataLen);
    bool SaveExtData(const GlobalEvent &event, const uint8_t *exData, uint32_t exDataLen);
    bool GetExtPasteData(const GlobalEvent &event, std::vector<uint8_t> &data, int32_t &syncTime);
    void ClearExtData();
    ffrt::shared_mutex rwMutex_;
    std::shared_ptr<BlockObject<GlobalEvent>> eventBlock_;
    std::mutex topEventMutex_;
    std::mutex localTopEventMutex_;
    std::mutex delayMutex_;
    GlobalEvent topEvent_;
    GlobalEvent localTopEvent_;
    AllConnectAdapter allConnect_;
    static const std::map<std::string, uint8_t> types_;
    DelayDataCallback delayDataCallback_;
    DelayEntryCallback delayEntryCallback_;
    PreSyncCallback preSyncCallback_;
    PreSyncMonitorCallback preSyncMonitorCallback_;
    enum KvStoreStatus {
        OPEN,
        CLOSED,
    };
    struct StoreInfo {
        std::shared_ptr<Store> store;
        KvStoreStatus status;
    };
    ffrt::shared_mutex storeInfoMapMutex_;
    std::map<int32_t, StoreInfo> storeInfoMap_;
    std::shared_ptr<PreSyncHandler> preSyncHandler_ = nullptr;
    std::mutex preSyncMapMutex_;
    std::map<int32_t, std::shared_ptr<PreSyncInfo>> preSyncMap_;
    struct BroadcastInfo {
        uint16_t seqId;
        std::string networkId;
        uint64_t time;
    };
    std::mutex broadcastInfoLock_;
    std::vector<BroadcastInfo> broadcastInfo_;
    std::atomic<uint64_t> clearTime_ = 0;
    std::atomic<int64_t> maxLocalCapacity_ = 0;
    std::shared_ptr<FFRTTimer> ffrtTimer;

    std::mutex extDataLock_;
    struct ExtData {
        GlobalEvent event_;
        std::vector<uint8_t> data_;
    };
    std::unique_ptr<ExtData> extData_;
    std::unique_ptr<std::pair<GlobalEvent, std::vector<uint8_t>>> extMimeTypes_ = nullptr;

    std::atomic<int32_t> registeredUserId_{Utils::ERROR_USERID};
    std::shared_ptr<BroadcastManagerListener> CreateBroadcastListener();
};
} // namespace OHOS::MiscServices
#endif // DISTRIBUTED_PASTEBOARD_FRAMEWORK_DISTRIBUTED_CLIP_H
