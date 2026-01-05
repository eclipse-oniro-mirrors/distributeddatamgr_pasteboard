/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_SERVICE_H
#define PASTE_BOARD_SERVICE_H

#include <system_ability_definition.h>

#include "bundle_mgr_proxy.h"
#include "clip/clip_plugin.h"
#include "common/block_object.h"
#include "device/distributed_module_config.h"
#include "eventcenter/event_center.h"
#include "ffrt/ffrt_utils.h"
#include "i_paste_data_processor.h"
#include "ientity_recognition_observer.h"
#include "input_manager.h"
#include "loader.h"
#include "pasteboard_account_state_subscriber.h"
#include "pasteboard_common_event_subscriber.h"
#include "pasteboard_dump_helper.h"
#include "pasteboard_event_common.h"
#include "pasteboard_service_stub.h"
#include "pasteboard_switch.h"
#include "privacy_kit.h"
#include "security_level.h"
#include "system_ability.h"

namespace OHOS {
namespace MiscServices {
constexpr int32_t ERROR_USERID = -1;
constexpr int32_t RESULT_OK = 0;
constexpr int64_t SIZE_K = 1024;
constexpr int64_t MIN_LOCAL_CAPACITY = 1; // 1M
constexpr int64_t DEFAULT_LOCAL_CAPACITY = 128; // 128M
constexpr int64_t MAX_LOCAL_CAPACITY = 2048; // 2G
enum class ServiceRunningState {
    STATE_NOT_START,
    STATE_RUNNING
};
struct AppInfo {
    std::string bundleName = "com.pasteboard.default";
    int32_t tokenType = -1;
    int32_t userId = ERROR_USERID;
    uint32_t tokenId;
    int32_t appIndex = 0;
};

struct HistoryInfo {
    std::string time;
    std::string bundleName;
    std::string state;
    std::string remote;
    int32_t userId = ERROR_USERID;
};

struct PasteDateTime {
    int32_t syncTime = 0;
    int32_t errorCode = 0;
    std::shared_ptr<PasteData> data;
};

struct PasteDateResult {
    int32_t syncTime = 0;
    int32_t errorCode = 0;
};

struct PasteP2pEstablishInfo {
    std::string networkId;
    std::shared_ptr<BlockObject<int32_t>> pasteBlock;
};

struct FocusedAppInfo {
    int32_t windowId = 0;
    sptr<IRemoteObject> abilityToken = nullptr;
};

class PasteboardService;
class InputEventCallback : public MMI::IInputEventConsumer {
public:
    enum InputType : int32_t {
        INPUTTYPE_PASTE = 0,
        INPUTTYPE_PRESYNC,
    };
    InputEventCallback(int32_t inputType = INPUTTYPE_PASTE, PasteboardService *pasteboardService = nullptr)
        : inputType_(inputType), pasteboardService_(pasteboardService) {};
    void OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const override;
    void OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const override;
    void OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const override;
    void OnKeyInputEventForPaste(std::shared_ptr<MMI::KeyEvent> keyEvent) const;
    bool IsCtrlVProcess(uint32_t callingPid, bool isFocused);
    void Clear();

private:
    static constexpr uint32_t WAIT_TIME_OUT = 100;
    mutable int32_t windowPid_ = -1;
    mutable uint64_t actionTime_ = 0;
    mutable std::shared_mutex inputEventMutex_;
    mutable std::shared_mutex blockMapMutex_;
    mutable std::map<uint32_t, std::shared_ptr<BlockObject<int32_t>>> blockMap_;
    int32_t inputType_ = INPUTTYPE_PASTE;
    PasteboardService *pasteboardService_ = nullptr;
};

class PasteboardService final : public SystemAbility, public PasteboardServiceStub {
    DECLARE_SYSTEM_ABILITY(PasteboardService)

public:
    API_EXPORT PasteboardService();
    API_EXPORT ~PasteboardService();
    int32_t CallbackEnter(uint32_t code) override;
    int32_t CallbackExit(uint32_t code, int32_t result) override;
    virtual int32_t Clear() override;
    virtual int32_t ClearByUser(int32_t userId) override;
    virtual int32_t GetRecordValueByType(uint32_t dataId, uint32_t recordId, int64_t &rawDataSize,
        std::vector<uint8_t> &buffer, int &fd) override;
    virtual int32_t GetPasteData(int &fd, int64_t &size, std::vector<uint8_t> &rawData,
        const std::string &pasteId, int32_t &syncTime, int32_t &realErrCode) override;
    virtual int32_t HasPasteData(bool &funcResult) override;
    virtual int32_t SetPasteData(int fd, int64_t memSize, const std::vector<uint8_t> &buffer,
        const sptr<IPasteboardDelayGetter> &delayGetter, const sptr<IPasteboardEntryGetter> &entryGetter) override;
    virtual int32_t SetPasteDataDelayData(int fd, int64_t memSize, const std::vector<uint8_t> &buffer,
        const sptr<IPasteboardDelayGetter> &delayGetter) override;
    virtual int32_t SetPasteDataEntryData(int fd, int64_t memSize, const std::vector<uint8_t> &buffer,
        const sptr<IPasteboardEntryGetter> &entryGetter) override;
    virtual int32_t SetPasteDataOnly(int fd, int64_t memSize, const std::vector<uint8_t> &buffer) override;
    virtual int32_t SyncDelayedData() override;
    virtual int32_t IsRemoteData(bool &funcResult) override;
    virtual int32_t GetMimeTypes(std::vector<std::string> &funcResult) override;
    virtual int32_t HasDataType(const std::string &mimeType, bool &funcResult) override;
    virtual int32_t HasUtdType(const std::string &utdType, bool &funcResult) override;
    virtual int32_t DetectPatterns(
        const std::vector<Pattern> &patternsToCheck, std::vector<Pattern>& funcResult) override;
    virtual int32_t GetDataSource(std::string &bundleNme) override;
    virtual int32_t SubscribeEntityObserver(
        EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver> &observer) override;
    virtual int32_t UnsubscribeEntityObserver(
        EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver> &observer) override;
    virtual int32_t SubscribeObserver(
        PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer) override;
    virtual int32_t ResubscribeObserver(
        PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer) override;
    virtual int32_t UnsubscribeObserver(
        PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer) override;
    virtual int32_t UnsubscribeAllObserver(PasteboardObserverType type) override;
    int32_t SubscribeDisposableObserver(const sptr<IPasteboardDisposableObserver> &observer,
        int32_t targetWindowId, DisposableType type, uint32_t maxLength) override;
    virtual int32_t SetGlobalShareOption(const std::unordered_map<uint32_t, int32_t> &globalShareOptions) override;
    virtual int32_t RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds) override;
    virtual int32_t GetGlobalShareOption(
        const std::vector<uint32_t> &tokenIds, std::unordered_map<uint32_t, int32_t>& funcResult) override;
    virtual int32_t SetAppShareOptions(int32_t shareOptions) override;
    virtual int32_t RemoveAppShareOptions() override;
    virtual void OnStart() override;
    virtual void OnStop() override;
    virtual int32_t PasteStart(const std::string &pasteId) override;
    virtual int32_t PasteComplete(const std::string &deviceId, const std::string &pasteId) override;
    int32_t GetRemoteDeviceName(std::string &deviceName, bool &isRemote);
    virtual int32_t ShowProgress(const std::string &progressKey, const sptr<IRemoteObject> &observer) override;
    virtual int32_t RegisterClientDeathObserver(const sptr<IRemoteObject> &observer) override;
    virtual int32_t DetachPasteboard() override;
    static int32_t currentUserId_;
    static ScreenEvent currentScreenStatus;
    size_t GetDataSize(PasteData &data) const;
    int Dump(int fd, const std::vector<std::u16string> &args) override;
    void NotifyDelayGetterDied(int32_t userId);
    void NotifyEntryGetterDied(int32_t userId);
    virtual int32_t GetChangeCount(uint32_t &changeCount) override;
    void CloseDistributedStore(int32_t user, bool isNeedClear);
    void ChangeStoreStatus(int32_t userId);
    void PreSyncRemotePasteboardData();
    PastedSwitch switch_;
    static int32_t GetCurrentAccountId();
    void ClearUriOnUninstall(int32_t tokenId);
    void ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData);

    static std::shared_mutex pasteDataMutex_;

private:
    std::atomic<bool> isCritical_ = false;
    std::mutex saMutex_;
    using Event = ClipPlugin::GlobalEvent;
    using GetProcessorFunc = IPasteDataProcessor& (*)();
    static constexpr const int32_t LISTENING_SERVICE[] = { DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID,
        WINDOW_MANAGER_SERVICE_ID, MEMORY_MANAGER_SA_ID, DISTRIBUTED_DEVICE_PROFILE_SA_ID };
    static constexpr const char *PLUGIN_NAME = "distributed_clip";
    static constexpr const char *SET_CRITICAL_ID = "pasteboard_service_set_critical_id";
    static constexpr const pid_t EDM_UID = 3057;
    static constexpr const pid_t ROOT_UID = 0;
    static constexpr uint32_t EXPIRATION_INTERVAL = 2 * 60 * 1000;
    static constexpr int MIN_TRANMISSION_TIME = 30 * 1000; // ms
    static constexpr int PRESYNC_MONITOR_TIME = 2 * 60 * 1000; // ms
    static constexpr int PRE_ESTABLISH_P2P_LINK_TIME = 2 * 60 * 1000; // ms
    static constexpr uint32_t SET_DISTRIBUTED_DATA_INTERVAL = 40 * 1000; // 40 seconds
    static constexpr int32_t ONE_HOUR_MINUTES = 60;
    static constexpr int32_t MAX_AGED_TIME = 24 * 60; // minute
    static constexpr int32_t MIN_AGED_TIME = 1; // minute
    static constexpr int32_t MINUTES_TO_MILLISECONDS = 60 * 1000;
    static constexpr uint32_t GET_REMOTE_DATA_WAIT_TIME = 30000;
    static constexpr int64_t PRESYNC_MONITOR_INTERVAL_MILLISECONDS = 500; // ms
    static constexpr int32_t INVALID_SUBSCRIBE_ID = -1;
    static const std::string REGISTER_PRESYNC_MONITOR;
    static const std::string UNREGISTER_PRESYNC_MONITOR;
    static const std::string P2P_ESTABLISH_STR;
    static const std::string P2P_PRESYNC_ID;
    std::atomic<int32_t> agedTime_ = ONE_HOUR_MINUTES * MINUTES_TO_MILLISECONDS; // 1 hour
    bool SetPasteboardHistory(HistoryInfo &info);
    bool IsFocusedApp(uint32_t tokenId);
    void InitBundles(Loader &loader);
    void SetInputMethodPid(int32_t userId, pid_t callPid);
    void ClearInputMethodPidByPid(int32_t userId, pid_t callPid);
    void ClearInputMethodPid(void);
    int32_t ClearInner(int32_t userId, const AppInfo &appInfo);
    bool IsSystemAppByFullTokenID(uint64_t tokenId);
    FocusedAppInfo GetFocusedAppInfo(void) const;
    int32_t GetDataTokenId(PasteData &pasteData);
    class DelayGetterDeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        explicit DelayGetterDeathRecipient(int32_t userId, PasteboardService &service);
        virtual ~DelayGetterDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

    private:
        int32_t userId_ = ERROR_USERID;
        PasteboardService &service_;
    };

    class EntryGetterDeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        explicit EntryGetterDeathRecipient(int32_t userId, PasteboardService &service);
        virtual ~EntryGetterDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

    private:
        int32_t userId_ = ERROR_USERID;
        PasteboardService &service_;
    };

    class RemoteDataTaskManager {
    public:
        struct TaskContext {
            std::atomic<bool> pasting_ = false;
            ConcurrentMap<uint32_t, std::shared_ptr<BlockObject<bool>>> getDataBlocks_;
            std::shared_ptr<PasteDateTime> data_;
        };
        using DataTask = std::pair<std::shared_ptr<PasteboardService::RemoteDataTaskManager::TaskContext>, bool>;
        DataTask GetRemoteDataTask(const Event &event);
        void Notify(const Event &event, std::shared_ptr<PasteDateTime> data);
        void ClearRemoteDataTask(const Event &event);
        std::shared_ptr<PasteDateTime> WaitRemoteData(const Event &event);

    private:
        std::atomic<uint32_t> mapKey_ = 0;
        std::mutex mutex_;
        std::map<std::string, std::shared_ptr<TaskContext>> dataTasks_;
    };

    struct classcomp {
        bool operator()(const sptr<IPasteboardChangedObserver> &l, const sptr<IPasteboardChangedObserver> &r) const
        {
            return l->AsObject() < r->AsObject();
        }
    };
    struct EntityObserverInfo {
        EntityType entityType;
        uint32_t expectedDataLength;
        uint32_t tokenId;
        sptr<IEntityRecognitionObserver> observer;
        EntityObserverInfo(EntityType entityType_, uint32_t expectedDataLength_, uint32_t tokenId_,
            sptr<IEntityRecognitionObserver> observer_) : entityType(entityType_),
            expectedDataLength(expectedDataLength_), tokenId(tokenId_), observer(observer_) { }
    };
    using ObserverMap = std::map<std::pair<int32_t, pid_t>,
        std::shared_ptr<std::set<sptr<IPasteboardChangedObserver>, classcomp>>>;
    uint32_t GetObserversSize(int32_t userId, pid_t pid, ObserverMap &observerMap);
    uint32_t GetAllObserversSize(int32_t userId, pid_t pid);
    void AddSysAbilityListener();
    int32_t Init();
    void InitScreenStatus();
    static ScreenEvent GetCurrentScreenStatus();
    std::string DumpHistory() const;
    std::string DumpData();
    void ThawInputMethod(pid_t imePid);
    bool IsNeedThaw(void);
    int32_t ExtractEntity(const std::string &entity, std::string &location);
    int32_t GetAllEntryPlainText(uint32_t dataId, uint32_t recordId,
        std::vector<std::shared_ptr<PasteDataEntry>> &entries, std::string &primaryText);
    std::string GetAllPrimaryText(const PasteData &pasteData);
    void NotifyEntityObservers(std::string &entity, EntityType entityType, uint32_t dataLength);
    void UnsubscribeAllEntityObserver();
    void NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status);
    void InitServiceHandler();
    bool IsCopyable(uint32_t tokenId) const;
    std::mutex imeMutex_;
    ConcurrentMap<int32_t, pid_t> imeMap_;

    struct DistributedMemory {
        std::mutex mutex;
        bool isRunning = false;
        std::shared_ptr<PasteData> data;
        Event event;
    };
    DistributedMemory setDistributedMemory_;

    int32_t SaveData(PasteData &pasteData, int64_t dataSize, const sptr<IPasteboardDelayGetter> delayGetter = nullptr,
        const sptr<IPasteboardEntryGetter> entryGetter = nullptr);
    void SetPasteDataInfo(PasteData &pasteData, const AppInfo &appInfo);
    void HandleDelayDataAndRecord(PasteData &pasteData, const sptr<IPasteboardDelayGetter> delayGetter,
        const sptr<IPasteboardEntryGetter> entryGetter, const AppInfo &appInfo);
    void RemovePasteData(const AppInfo &appInfo);
    void SetPasteDataDot(PasteData &pasteData, const int32_t &userId);
    std::pair<int32_t, ClipPlugin::GlobalEvent> GetValidDistributeEvent(int32_t user);
    int32_t GetSdkVersion(uint32_t tokenId);
    bool IsPermissionGranted(const std::string &perm, uint32_t tokenId);
    int32_t CheckAndGrantRemoteUri(PasteData &data, const AppInfo &appInfo,
        const std::string &pasteId, std::shared_ptr<BlockObject<int32_t>> pasteBlock);
    int32_t GetData(uint32_t tokenId, PasteData &data, int32_t &syncTime, bool &isPeerOnline, std::string &peerNetId,
        std::string &peerUdid);
    void HandleInitFailure();
    void InitializeDumpCommands();
    void HandleNotificationsAndStatusChecks(const AppInfo &appInfo, const PasteData &data,
        const std::string &peerNetId, bool &isPeerOnline);
    void PublishServiceState(const PasteData &data, int32_t syncTime,
        const std::string &peerNetId, std::shared_ptr<BlockObject<int32_t>> pasteBlock);
    void HandleGetDataError(int32_t result, std::shared_ptr<BlockObject<int32_t>> pasteBlock,
        const std::string &deviceId, const std::string &pasteId);
    CommonInfo GetCommonState(int64_t dataSize);
    void SetRadarEvent(const AppInfo &appInfo, PasteData &data, bool isPeerOnline,
        RadarReportInfo &radarReportInfo, const std::string &peerNetId);
    void SetUeEvent(const AppInfo &appInfo, PasteData &data, bool isPeerOnline,
        UeReportInfo &ueReportInfo, const std::string &peerNetId);
    int32_t GetPasteDataInner(int &fd, int64_t &size, std::vector<uint8_t> &rawData,
        const std::string &pasteId, int32_t &syncTime, UeReportInfo &ueReportInfo);
    void GetPasteDataDot(PasteData &pasteData, const std::string &bundleName, const int32_t &userId);
    int32_t GetLocalData(const AppInfo &appInfo, PasteData &data);
    int32_t GetRemoteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime);
    int32_t GetRemotePasteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime);
    int32_t GetDelayPasteRecord(int32_t userId, PasteData &data);
    void GetDelayPasteData(int32_t userId, PasteData &data);
    int32_t ProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo, PasteDataEntry &entry);
    int32_t PostProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetInfo, PasteDataEntry &entry);
    std::map<uint32_t, std::vector<Uri>> CheckUriPermission(
        PasteData &data, const std::pair<std::string, int32_t> &targetBundleAppIndex);
    void RemoveInvalidRemoteUri(std::vector<Uri> &grantUris);
    int32_t GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
        const std::string &targetBundleName, int32_t appIndex);
    int32_t GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
        const std::string &targetBundleName, bool isRemoteData, int32_t appIndex);
    void GenerateDistributedUri(PasteData &data);
    bool IsBundleOwnUriPermission(const std::string &bundleName, Uri &uri);
    std::string GetAppLabel(uint32_t tokenId);
    sptr<OHOS::AppExecFwk::IBundleMgr> GetAppBundleManager();
    void OpenP2PLink(const std::string &networkId);
    std::shared_ptr<BlockObject<int32_t>> CheckAndReuseP2PLink(const std::string &networkId,
        const std::string &pasteId);
    void EstablishP2PLink(const std::string &networkId, const std::string &pasteId);
    bool IsContainUri(const std::vector<std::string> &dataType);
    std::shared_ptr<BlockObject<int32_t>> EstablishP2PLinkTask(
        const std::string &pasteId, const ClipPlugin::GlobalEvent &event);
    void OnEstablishP2PLinkTask(const std::string &networkId, std::shared_ptr<BlockObject<int32_t>> pasteBlock);
    void ClearP2PEstablishTaskInfo();
    void CloseP2PLink(const std::string &networkId);
    bool HasDistributedDataType(const std::string &mimeType);

    std::pair<std::shared_ptr<PasteData>, PasteDateResult> GetDistributedData(const Event &event, int32_t user);
    int32_t GetDistributedDelayData(const Event &evt, uint8_t version, std::vector<uint8_t> &rawData);
    int32_t GetDistributedDelayEntry(const Event &evt, uint32_t recordId, const std::string &utdId,
        std::vector<uint8_t> &rawData);
    int32_t ProcessDistributedDelayUri(int32_t userId, PasteData &data, PasteDataEntry &entry,
        std::vector<uint8_t> &rawData);
    int32_t ProcessDistributedDelayHtml(PasteData &data, PasteDataEntry &entry, std::vector<uint8_t> &rawData);
    int32_t ProcessDistributedDelayEntry(PasteDataEntry &entry, std::vector<uint8_t> &rawData);
    int32_t GetRemoteEntryValue(const AppInfo &appInfo, PasteData &data, PasteDataRecord &record,
        PasteDataEntry &entry);
    int32_t ProcessRemoteDelayUri(const std::string &deviceId, const AppInfo &appInfo,
        PasteData &data, PasteDataRecord &record, PasteDataEntry &entry);
    int32_t ProcessRemoteDelayHtml(const std::string &remoteDeviceId, const AppInfo &appInfo,
        const std::vector<uint8_t> &rawData, PasteData &data, PasteDataRecord &record, PasteDataEntry &entry);
    int32_t ProcessRemoteDelayHtmlInner(const std::string &remoteDeviceId, const AppInfo &appInfo,
        PasteData &tmpData, PasteData &data, PasteDataEntry &entry);
    int32_t GetLocalEntryValue(int32_t userId, PasteData &data, PasteDataRecord &record, PasteDataEntry &entry);
    int32_t GetFullDelayPasteData(int32_t userId, PasteData &data);
    bool IsDisallowDistributed();
    bool SetDistributedData(int32_t user, PasteData &data);
    bool SetCurrentDistributedData(PasteData &data, Event event);
    bool SetCurrentData(Event event, PasteData &data);
    void CleanDistributedData(int32_t user);
    void OnConfigChange(bool isOn);
    void OnConfigChangeInner(bool isOn);
    std::shared_ptr<ClipPlugin> GetClipPlugin();
    void IncreaseChangeCount(int32_t userId);

    static std::string GetTime();
    bool IsDataAged();
    bool VerifyPermission(uint32_t tokenId);
    int32_t IsDataValid(PasteData &pasteData, uint32_t tokenId);
    static AppInfo GetAppInfo(uint32_t tokenId);
    static std::string GetAppBundleName(const AppInfo &appInfo);
    static void SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData &pasteData);
    void RecognizePasteData(PasteData &pasteData);
    void OnRecognizePasteData(const std::string &primaryText);
    void OnRecognizePasteDataInner(const std::string &primaryText, void *nulGuard);
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void UpdateAgedTime();
    void CancelCriticalTimer();
    void SetCriticalTimer();
    void OnAddDeviceManager();
    void OnAddMemoryManager();
    void OnAddDeviceProfile();
    void OnRemoveDeviceProfile();
    void ReportUeCopyEvent(PasteData &pasteData, int64_t dataSize, int32_t result);
    bool HasDataType(const std::string &mimeType);
    bool HasUtdType(const std::string &utdType);
    bool HasPasteData();
    bool IsRemoteData();
    int32_t GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry &value);
    int32_t GetRecordValueByType(int64_t &rawDataSize, std::vector<uint8_t> &buffer, int &fd,
        const PasteDataEntry &entryValue);
    int32_t DealData(int &fd, int64_t &size, std::vector<uint8_t> &rawData, PasteData &data);
    bool WriteRawData(const void *data, int64_t size, int &serFd);
    int32_t WritePasteData(
        int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer, PasteData &pasteData, bool &hasData);
    void CloseSharedMemFd(int fd);
    void ClearAgedData(int32_t userId);
    void SetDataExpirationTimer(int32_t userId);

    void InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin);
    bool OpenP2PLinkForPreEstablish(const std::string &networkId, ClipPlugin *clipPlugin);
    void PreEstablishP2PLink(const std::string &networkId, ClipPlugin *clipPlugin);
    void PreEstablishP2PLinkCallback(const std::string &networkId, ClipPlugin *clipPlugin);
    void PreSyncSwitchMonitorCallback();
    void RegisterPreSyncMonitor();
    void UnRegisterPreSyncMonitor();
    void DeletePreSyncP2pFromP2pMap(const std::string &networkId);
    void DeletePreSyncP2pMap(const std::string &networkId);
    void AddPreSyncP2pTimeoutTask(const std::string &networkId);

    static inline ServiceRunningState state_ = ServiceRunningState::STATE_NOT_START;
    std::shared_ptr<AppExecFwk::EventHandler> serviceHandler_;
    std::mutex observerMutex_;
    ObserverMap observerLocalChangedMap_;
    ObserverMap observerRemoteChangedMap_;
    ObserverMap observerEventMap_;
    ClipPlugin::GlobalEvent currentEvent_;
    ClipPlugin::GlobalEvent remoteEvent_;
    ConcurrentMap<int32_t, std::shared_ptr<PasteData>> clips_;
    ConcurrentMap<int32_t, uint32_t> clipChangeCount_;
    ConcurrentMap<pid_t, std::vector<EntityObserverInfo>> entityObserverMap_;
    ConcurrentMap<int32_t, std::pair<sptr<IPasteboardEntryGetter>, sptr<EntryGetterDeathRecipient>>> entryGetters_;
    ConcurrentMap<int32_t, std::pair<sptr<IPasteboardDelayGetter>, sptr<DelayGetterDeathRecipient>>> delayGetters_;
    ConcurrentMap<int32_t, uint64_t> copyTime_;
    std::set<std::pair<std::string, int32_t>> readBundles_;
    std::shared_ptr<PasteBoardCommonEventSubscriber> commonEventSubscriber_ = nullptr;
    std::shared_ptr<PasteBoardAccountStateSubscriber> accountStateSubscriber_ = nullptr;

    std::recursive_mutex mutex;
    std::shared_ptr<ClipPlugin> clipPlugin_ = nullptr;
    std::atomic<uint16_t> sequenceId_ = 0;
    std::atomic<uint32_t> dataId_ = 0;
    std::atomic<uint32_t> delayTokenId_ = 0;
    std::atomic<uint32_t> delayDataId_ = 0;
    static std::mutex historyMutex_;
    std::mutex bundleMutex_;
    std::mutex readBundleMutex_;
    static std::vector<std::string> dataHistory_;
    static std::shared_ptr<Command> copyHistory;
    static std::shared_ptr<Command> copyData;
    std::atomic<bool> setting_ = false;

    struct PasteboardP2pInfo {
        pid_t callPid;
        bool isSuccess;
    };
    std::shared_ptr<FFRTTimer> ffrtTimer_;
    std::mutex p2pMapMutex_;
    PasteP2pEstablishInfo p2pEstablishInfo_;
    ConcurrentMap<std::string, ConcurrentMap<std::string, PasteboardP2pInfo>> p2pMap_;
    std::map<std::string, std::shared_ptr<BlockObject<int32_t>>> preSyncP2pMap_;
    int32_t subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
    enum GlobalShareOptionSource {
        MDM = 0,
        APP = 1,
    };

    struct GlobalShareOption {
        GlobalShareOptionSource source;
        ShareOption shareOption;
    };

    ConcurrentMap<uint32_t, GlobalShareOption> globalShareOptions_;

    bool AddObserver(int32_t userId, const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap);
    void RemoveSingleObserver(
        int32_t userId, const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap);
    void RemoveAllObserver(int32_t userId, ObserverMap &observerMap);
    bool IsCallerUidValid();
    std::vector<std::string> GetLocalMimeTypes();
    bool HasLocalDataType(const std::string &mimeType);
    void AddPermissionRecord(uint32_t tokenId, bool isReadGrant, bool isSecureGrant);
    bool SubscribeKeyboardEvent();
    bool IsConstraintEnabled(int32_t user);
    void UpdateShareOption(PasteData &pasteData);
    bool CheckMdmShareOption(PasteData &pasteData);
    void PasteboardEventSubscriber();
    void CommonEventSubscriber();
    void AccountStateSubscriber();
    bool IsBasicType(const std::string &mimeType);
    std::function<void(const OHOS::MiscServices::Event &)> RemotePasteboardChange();
    std::shared_ptr<InputEventCallback> inputEventCallback_;
    DistributedModuleConfig moduleConfig_;
    int32_t uid_ = -1;
    std::atomic<int64_t> maxLocalCapacity_ = DEFAULT_LOCAL_CAPACITY * SIZE_K * SIZE_K;
    RemoteDataTaskManager taskMgr_;
    pid_t setPasteDataUId_ = 0;
    static constexpr pid_t TEST_SERVER_UID = 3500;
    std::mutex eventMutex_;
    std::mutex entityRecognizeMutex_;
    SecurityLevel securityLevel_;
    class PasteboardDeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        PasteboardDeathRecipient(PasteboardService &service, pid_t pid);
        virtual ~PasteboardDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

    private:
        PasteboardService &service_;
        pid_t pid_;
    };
    int32_t AppExit(pid_t pid);
    void RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap &observerMap);
    ConcurrentMap<pid_t, std::pair<sptr<IRemoteObject>, sptr<PasteboardDeathRecipient>>> clients_;
    static constexpr pid_t INVALID_UID = -1;
    static constexpr pid_t INVALID_PID = -1;
    static constexpr uint32_t INVALID_TOKEN = 0;
    static constexpr uint32_t MAX_OBSERVER_COUNT = 10;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_SERVICE_H