/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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
#include "pasteboard_service.h"

#include <dlfcn.h>
#include <sys/mman.h>

#include "ashmem.h"
#include "ability_manager_client.h"
#include "accesstoken_kit.h"
#include "account_manager.h"
#include "calculate_time_consuming.h"
#include "common_event_manager.h"
#include "device/dev_profile.h"
#include "distributed_file_daemon_manager.h"
#ifdef WITH_DLP
#include "dlp_permission_kit.h"
#include "dlp_permission.h"
#endif // WITH_DLP
#include "eventcenter/pasteboard_event.h"
#include "hiview_adapter.h"
#include "input_method_controller.h"
#include "ipasteboard_changed_observer.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "mem_mgr_client.h"
#include "message_parcel_warp.h"
#include "os_account_manager.h"
#include "parameters.h"
#include "pasteboard_common.h"
#include "pasteboard_delay_manager.h"
#include "pasteboard_dialog.h"
#include "pasteboard_disposable_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_pattern.h"
#include "pasteboard_time.h"
#include "pasteboard_trace.h"
#include "pasteboard_web_controller.h"
#include "permission/permission_utils.h"
#include "remote_file_share.h"
#include "res_sched_client.h"
#include "reporter.h"
#include "distributed_module_config.h"
#ifdef PB_SCREENLOCK_MGR_ENABLE
#include "screenlock_manager.h"
#include "file_mount_manager.h"
#endif // PB_SCREENLOCK_MGR_ENABLE
#include "tokenid_kit.h"
#include "uri_permission_manager_client.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif // SCENE_BOARD_ENABLE

namespace OHOS {
namespace MiscServices {
using namespace Rosen;
using namespace std::chrono;
using namespace Storage::DistributedFile;
using namespace RadarReporter;
using namespace UeReporter;
namespace {
constexpr int32_t COMMON_USERID = 0;
constexpr int32_t INIT_INTERVAL = 10000L;
constexpr uint32_t MAX_IPC_THREAD_NUM = 32;
constexpr const char *PASTEBOARD_SERVICE_SA_NAME = "pasteboard_service";
constexpr const char *PASTEBOARD_SERVICE_NAME = "PasteboardService";
constexpr const char *NLU_SO_PATH = "libai_nlu_innerapi.z.so";
constexpr const char *GET_PASTE_DATA_PROCESSOR = "GetPasteDataProcessor";
constexpr const char *FAIL_TO_GET_TIME_STAMP = "FAIL_TO_GET_TIME_STAMP";
constexpr const char *SECURE_PASTE_PERMISSION = "ohos.permission.SECURE_PASTE";
constexpr const char *READ_PASTEBOARD_PERMISSION = "ohos.permission.READ_PASTEBOARD";
constexpr const char *TRANSMIT_CONTROL_PROP_KEY = "persist.distributed_scene.datafiles_trans_ctrl";
constexpr const char *MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION =
    "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION";
constexpr const char *GET_DATA_APP = "GET_DATA_APP";
constexpr const char *NETWORK_DEV_NUM = "NETWORK_DEV_NUM";
constexpr const char *COVER_DELAY_DATA = "COVER_DELAY_DATA";
constexpr const char *UE_COPY = "DISTRIBUTED_PASTEBOARD_COPY";
constexpr const char *UE_PASTE = "DISTRIBUTED_PASTEBOARD_PASTE";

constexpr int32_t INVALID_VERSION = -1;
constexpr int32_t ADD_PERMISSION_CHECK_SDK_VERSION = 12;
constexpr int32_t CTRLV_EVENT_SIZE = 2;
constexpr int32_t CONTROL_TYPE_ALLOW_SEND_RECEIVE = 1;
constexpr uint32_t EVENT_TIME_OUT = 2000;
constexpr uint32_t MAX_RECOGNITION_LENGTH = 1000;
constexpr int32_t DEVICE_COLLABORATION_UID = 5521;
constexpr uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);
constexpr uint32_t MAX_BUNDLE_NAME_LENGTH = 127;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
constexpr int32_t E_OK_OPERATION = 0;
constexpr int32_t SET_VALUE_SUCCESS = 1;
constexpr uid_t ANCO_SERVICE_BROKER_UID = 5557;
constexpr float RECALCULATE_DATA_SIZE = 0.9;

const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(new PasteboardService());
const std::string CONSTRAINT = "constraint.distributed.transmission.outgoing";
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
std::mutex PasteboardService::historyMutex_;
std::shared_mutex PasteboardService::pasteDataMutex_;
std::vector<std::string> PasteboardService::dataHistory_;
std::shared_ptr<Command> PasteboardService::copyHistory;
std::shared_ptr<Command> PasteboardService::copyData;
int32_t PasteboardService::currentUserId_ = ERROR_USERID;
ScreenEvent PasteboardService::currentScreenStatus = ScreenEvent::Default;
const std::string PasteboardService::REGISTER_PRESYNC_MONITOR = "RegisterPresyncMonitor";
const std::string PasteboardService::UNREGISTER_PRESYNC_MONITOR = "UnregisterPresyncMonitor";
const std::string PasteboardService::P2P_ESTABLISH_STR = "P2pEstablish";
const std::string PasteboardService::P2P_PRESYNC_ID = "P2pPreSyncId_";

PasteboardService::PasteboardService(): SystemAbility(PASTEBOARD_SERVICE_ID, true)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardService Start.");
    PasteboardService::state_ = ServiceRunningState::STATE_NOT_START;
    p2pEstablishInfo_.pasteBlock = nullptr;
}

PasteboardService::~PasteboardService()
{
    clients_.Clear();
    UnsubscribeAllEntityObserver();
}

int32_t PasteboardService::Init()
{
    if (!Publish(this)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "OnStart register to system ability manager failed.");
        auto userId = GetCurrentAccountId();
        Reporter::GetInstance().PasteboardFault().Report({ userId, "ERR_INVALID_OPTION" });
        return static_cast<int32_t>(PasteboardError::INVALID_OPTION_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Init Success.");
    PasteboardService::state_ = ServiceRunningState::STATE_RUNNING;
    InitScreenStatus();
    return ERR_OK;
}

void PasteboardService::InitScreenStatus()
{
#ifdef PB_SCREENLOCK_MGR_ENABLE
    auto screenLockManager = OHOS::ScreenLock::ScreenLockManager::GetInstance();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(screenLockManager != nullptr, PASTEBOARD_MODULE_SERVICE,
        "ScreenLockManager instance is null.");
    auto isScreenLocked = screenLockManager->IsScreenLocked();
    PasteboardService::currentScreenStatus = isScreenLocked ? ScreenEvent::ScreenLocked : ScreenEvent::ScreenUnlocked;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen status is %{public}d", PasteboardService::currentScreenStatus);
#else
    PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "PB_SCREENLOCK_MGR_ENABLE not defined");
    return;
#endif
}

void PasteboardService::OnStart()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardService OnStart.");
    std::lock_guard<std::mutex> lock(saMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(PasteboardService::state_ != ServiceRunningState::STATE_RUNNING,
        PASTEBOARD_MODULE_SERVICE, "PasteboardService is already running.");
    IPCSkeleton::SetMaxWorkThreadNum(MAX_IPC_THREAD_NUM);
    InitServiceHandler();
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    Loader loader;
    uid_ = loader.LoadUid();
    moduleConfig_.Init();
#ifdef PB_DATACLASSIFICATION_ENABLE
    auto status = DATASL_OnStart();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "datasl on start ret:%{public}d", status);
#endif
    moduleConfig_.Watch(std::bind(&PasteboardService::OnConfigChange, this, std::placeholders::_1));
    ffrtTimer_ = FFRTPool::GetTimer("pasteboard_service");
    UpdateAgedTime();
    AddSysAbilityListener();
    if (Init() != ERR_OK && serviceHandler_ != nullptr) {
        HandleInitFailure();
        return;
    }
    auto callback = [this]() {
        switch_.Init(GetCurrentAccountId());
    };
    if (serviceHandler_ != nullptr) {
        serviceHandler_->PostTask(callback);
    }
    InitializeDumpCommands();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Start PasteboardService success.");
    EventCenter::GetInstance().Subscribe(OHOS::MiscServices::Event::EVT_REMOTE_CHANGE, RemotePasteboardChange());
    HiViewAdapter::StartTimerThread();
    return;
}

void PasteboardService::HandleInitFailure()
{
    auto callback = [this]() {
        Init();
    };
    serviceHandler_->PostTask(callback, INIT_INTERVAL);
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Init failed. Try again 10s later.");
}

void PasteboardService::InitializeDumpCommands()
{
    copyHistory = std::make_shared<Command>(std::vector<std::string>{ "--copy-history" },
        "Dump access history last ten times.",
        [this](const std::vector<std::string> &input, std::string &output) -> bool {
            output = DumpHistory();
            return true;
        });
    copyData = std::make_shared<Command>(std::vector<std::string>{ "--data" }, "Show copy data details.",
        [this](const std::vector<std::string> &input, std::string &output) -> bool {
            output = DumpData();
            return true;
        });
    PasteboardDumpHelper::GetInstance().RegisterCommand(copyHistory);
    PasteboardDumpHelper::GetInstance().RegisterCommand(copyData);
    CommonEventSubscriber();
    AccountStateSubscriber();
    PasteboardEventSubscriber();
}

void PasteboardService::OnStop()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStop Started.");
    std::lock_guard<std::mutex> lock(saMutex_);
    if (PasteboardService::state_ != ServiceRunningState::STATE_RUNNING) {
        return;
    }
    serviceHandler_ = nullptr;
    PasteboardService::state_ = ServiceRunningState::STATE_NOT_START;
    DMAdapter::GetInstance().DeInitialize();
    if (commonEventSubscriber_ != nullptr) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(commonEventSubscriber_);
    }
    moduleConfig_.DeInit();
    switch_.DeInit();
#ifdef PB_DATACLASSIFICATION_ENABLE
    DATASL_OnStop();
#endif
    EventCenter::GetInstance().Unsubscribe(PasteboardEvent::DISCONNECT);
    EventCenter::GetInstance().Unsubscribe(OHOS::MiscServices::Event::EVT_REMOTE_CHANGE);
    CancelCriticalTimer();
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, PASTEBOARD_SERVICE_ID);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStop End.");
}

void PasteboardService::AddSysAbilityListener()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "begin.");
    for (uint32_t i = 0; i < sizeof(LISTENING_SERVICE) / sizeof(LISTENING_SERVICE[0]); i++) {
        auto ret = AddSystemAbilityListener(LISTENING_SERVICE[i]);
        if (ret) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Add listener success, serviceId = %{public}d.",
                LISTENING_SERVICE[i]);
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Add listener failed, serviceId = %{public}d.",
                LISTENING_SERVICE[i]);
        }
    }
}

void PasteboardService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    (void)deviceId;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "systemAbilityId=%{public}d", systemAbilityId);

    switch (systemAbilityId) {
        case DISTRIBUTED_HARDWARE_DEVICEMANAGER_SA_ID:
            OnAddDeviceManager();
            break;
        case MEMORY_MANAGER_SA_ID:
            OnAddMemoryManager();
            break;
        case DISTRIBUTED_DEVICE_PROFILE_SA_ID:
            OnAddDeviceProfile();
            break;
        default:
            break;
    }
}

void PasteboardService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    (void)deviceId;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "systemAbilityId=%{public}d", systemAbilityId);

    switch (systemAbilityId) {
        case DISTRIBUTED_DEVICE_PROFILE_SA_ID:
            OnRemoveDeviceProfile();
            break;
        default:
            break;
    }
}

PasteboardService::DelayGetterDeathRecipient::DelayGetterDeathRecipient(int32_t userId, PasteboardService &service)
    : userId_(userId), service_(service)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Construct Delay Getter Death Recipient");
}

void PasteboardService::DelayGetterDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start");
    (void)remote;
    service_.NotifyDelayGetterDied(userId_);
}

void PasteboardService::NotifyDelayGetterDied(int32_t userId)
{
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "error userId: %{public}d", userId);
        return;
    }
    delayGetters_.Erase(userId);
}

PasteboardService::EntryGetterDeathRecipient::EntryGetterDeathRecipient(int32_t userId, PasteboardService &service)
    : userId_(userId), service_(service)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Construct Entry Getter Death Recipient");
}

void PasteboardService::EntryGetterDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start");
    (void)remote;
    service_.NotifyEntryGetterDied(userId_);
}

void PasteboardService::NotifyEntryGetterDied(int32_t userId)
{
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "error userId: %{public}d", userId);
        return;
    }
    entryGetters_.Erase(userId);
}

void PasteboardService::UpdateAgedTime()
{
    static bool developerMode = OHOS::system::GetBoolParameter("const.security.developermode.state", false);
    int32_t agedTime = developerMode ? system::GetIntParameter("const.pasteboard.rd_test_aged_time",
        ONE_HOUR_MINUTES, MIN_AGED_TIME, MAX_AGED_TIME) : ONE_HOUR_MINUTES;
    agedTime_.store(agedTime * MINUTES_TO_MILLISECONDS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "agedTime_: %{public}d", agedTime_.load());
}

void PasteboardService::CancelCriticalTimer()
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ffrtTimer_ != nullptr, PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
    ffrtTimer_->CancelTimer(SET_CRITICAL_ID);
    Memory::MemMgrClient::GetInstance().SetCritical(getpid(), false, PASTEBOARD_SERVICE_ID);
    isCritical_.store(false);
}

void PasteboardService::SetCriticalTimer()
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ffrtTimer_ != nullptr, PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");

    FFRTTask task = [this] {
        std::thread thread([=]() {
            Memory::MemMgrClient::GetInstance().SetCritical(getpid(), false, PASTEBOARD_SERVICE_ID);
            isCritical_.store(false);
        });
        thread.detach();
    };

    ffrtTimer_->SetTimer(SET_CRITICAL_ID, task, static_cast<uint32_t>(agedTime_.load()));

    if (!isCritical_.load()) {
        Memory::MemMgrClient::GetInstance().SetCritical(getpid(), true, PASTEBOARD_SERVICE_ID);
        isCritical_.store(true);
    }
}

void PasteboardService::OnAddDeviceManager()
{
    DMAdapter::GetInstance().Initialize();
}

void PasteboardService::OnAddMemoryManager()
{
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 1, PASTEBOARD_SERVICE_ID);
    SetCriticalTimer();
}

void PasteboardService::OnAddDeviceProfile()
{
    DevProfile::GetInstance().SendSubscribeInfos();
}

void PasteboardService::OnRemoveDeviceProfile()
{
    DevProfile::GetInstance().ClearDeviceProfileService();
}

void PasteboardService::ReportUeCopyEvent(PasteData &pasteData, int64_t dataSize, int32_t result)
{
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    auto res = (result == static_cast<int32_t>(PasteboardError::E_OK)) ? E_OK_OPERATION : result;
    UeReportInfo reportInfo;
    reportInfo.ret = res;
    reportInfo.dataType = pasteData.GenerateDataType();
    reportInfo.bundleName = appInfo.bundleName;
    reportInfo.description = pasteData.GetReportDescription();
    reportInfo.commonInfo = GetCommonState(dataSize);
    UE_REPORT(UE_COPY, reportInfo,
        "RECORD_NUM", reportInfo.description.recordNum,
        "DATA_SIZE", reportInfo.commonInfo.dataSize,
        "CURRENT_ACCOUNT_ID", reportInfo.commonInfo.currentAccountId,
        "ENTRY_NUM", reportInfo.description.entryNum,
        "MIMETYPES", reportInfo.description.mimeTypes);
}

void PasteboardService::InitServiceHandler()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "InitServiceHandler started.");
    if (serviceHandler_ != nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Already init.");
        return;
    }
    std::shared_ptr<AppExecFwk::EventRunner> runner =
        AppExecFwk::EventRunner::Create(PASTEBOARD_SERVICE_NAME, AppExecFwk::ThreadMode::FFRT);
    serviceHandler_ = std::make_shared<AppExecFwk::EventHandler>(runner);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "InitServiceHandler Succeeded.");
}

int32_t PasteboardService::Clear()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter, clips_.Size=%{public}zu", clips_.Size());
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    auto userId = appInfo.userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    return ClearInner(userId, appInfo);
}

int32_t PasteboardService::ClearByUser(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter, clips_.Size=%{public}zu", clips_.Size());
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(appInfo.tokenType == ATokenTypeEnum::TOKEN_NATIVE,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE,
        "userId is %{public}d", userId);
    return ClearInner(userId, appInfo);
}

int32_t PasteboardService::ClearInner(int32_t userId, const AppInfo &appInfo)
{
    RADAR_REPORT(DFX_CLEAR_PASTEBOARD, DFX_MANUAL_CLEAR, DFX_SUCCESS);
    auto it = clips_.Find(userId);
    if (it.first) {
        clips_.Erase(userId);
        delayDataId_ = 0;
        delayTokenId_ = 0;
        std::string bundleName = GetAppBundleName(appInfo);
        NotifyObservers(bundleName, userId, PasteboardEventStatus::PASTEBOARD_CLEAR);
    }
    CleanDistributedData(userId);
    CancelCriticalTimer();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, clips_.Size=%{public}zu, appInfo.userId = %{public}d",
        clips_.Size(), userId);
    return ERR_OK;
}

int32_t PasteboardService::GetChangeCount(uint32_t &changeCount)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    changeCount = 0;
    clipChangeCount_.ComputeIfPresent(appInfo.userId, [&changeCount](auto, auto &value) {
        changeCount = value;
        PASTEBOARD_HILOGI(
            PASTEBOARD_MODULE_SERVICE, "Find changeCount succeed, changeCount is %{public}u", changeCount);
        return true;
    });
    return ERR_OK;
}

void PasteboardService::IncreaseChangeCount(int32_t userId)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "IncreaseChangeCount start!");
    clipChangeCount_.Compute(userId, [](auto userId, uint32_t &changeCount) {
        changeCount = (changeCount == UINT32_MAX) ? 0 : changeCount + 1;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "userId=%{public}d, changeCount=%{public}u", userId, changeCount);
        return true;
    });
}

void PasteboardService::NotifyEntityObservers(std::string &entity, EntityType entityType, uint32_t dataLength)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "entityType=%{public}u, dataLength=%{public}u",
        static_cast<uint32_t>(entityType), dataLength);
    entityObserverMap_.ForEach([this, &entity, entityType, dataLength](const auto &key, auto &value) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pid=%{public}u, listSize=%{public}zu", key, value.size());
        for (auto entityObserver : value) {
            if (entityType == entityObserver.entityType && dataLength <= entityObserver.expectedDataLength &&
                VerifyPermission(entityObserver.tokenId)) {
                entityObserver.observer->OnRecognitionEvent(entityType, entity);
            }
        }
        return false;
    });
}

int32_t PasteboardService::GetAllEntryPlainText(uint32_t dataId, uint32_t recordId,
    std::vector<std::shared_ptr<PasteDataEntry>> &entries, std::string &primaryText)
{
    for (auto &entry : entries) {
        if (primaryText.size() > MAX_RECOGNITION_LENGTH) {
            return static_cast<int32_t>(PasteboardError::EXCEEDING_LIMIT_EXCEPTION);
        }
        int32_t result = static_cast<int32_t>(PasteboardError::E_OK);
        if (entry->GetMimeType() == MIMETYPE_TEXT_PLAIN && !entry->HasContentByMimeType(MIMETYPE_TEXT_PLAIN)) {
            result = GetRecordValueByType(dataId, recordId, *entry);
        }
        if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
            continue;
        }
        std::shared_ptr<std::string> plainTextPtr = entry->ConvertToPlainText();
        if (plainTextPtr != nullptr) {
            primaryText += *plainTextPtr;
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetAllEntryPlainText finished");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

std::string PasteboardService::GetAllPrimaryText(const PasteData &pasteData)
{
    std::string primaryText = "";
    std::vector<std::shared_ptr<PasteDataRecord>> records = pasteData.AllRecords();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "size of records=%{public}zu", records.size());
    for (const auto &record : records) {
        if (primaryText.size() > MAX_RECOGNITION_LENGTH) {
            primaryText = "";
            break;
        }
        std::shared_ptr<std::string> plainTextPtr = record->GetPlainTextV0();
        if (plainTextPtr != nullptr) {
            primaryText += *plainTextPtr;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "primaryText in record");
            continue;
        }
        auto dataId = pasteData.GetDataId();
        auto recordId = record->GetRecordId();
        std::vector<std::shared_ptr<PasteDataEntry>> entries = record->GetEntries();
        int32_t result = GetAllEntryPlainText(dataId, recordId, entries, primaryText);
        if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "primaryText exceeded size, result=%{public}d", result);
            primaryText = "";
            break;
        }
    }
    return primaryText;
}

int32_t PasteboardService::ExtractEntity(const std::string &entity, std::string &location)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!entity.empty(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "entity empty");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(nlohmann::json::accept(entity),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "entity invalid");
    nlohmann::json entityJson = nlohmann::json::parse(entity);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!entityJson.is_discarded(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "parse entity to json failed");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entityJson.contains("code") && entityJson["code"].is_number(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "entity find code failed");
    int code = entityJson["code"].get<int>();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        code == 0, static_cast<int32_t>(code), PASTEBOARD_MODULE_SERVICE, "failed to get entity");
    if (entityJson.contains("entity") && entityJson["entity"].contains("location") &&
        entityJson["entity"]["location"].is_array()) {
        nlohmann::json locationJson = entityJson["entity"]["location"].get<nlohmann::json>();
        location = locationJson.dump();
        PASTEBOARD_HILOGI(
            PASTEBOARD_MODULE_SERVICE, "location dump finished, location size=%{public}zu", location.size());
        return static_cast<int32_t>(PasteboardError::E_OK);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteData did not contain entity");
    return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
}

void PasteboardService::OnRecognizePasteData(const std::string &primaryText)
{
    pthread_setname_np(pthread_self(), "PasteDataRecognize");
    auto handle = dlopen(NLU_SO_PATH, RTLD_NOW);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(handle != nullptr, PASTEBOARD_MODULE_SERVICE, "Can not get AIEngine handle");
    GetProcessorFunc GetProcessor = reinterpret_cast<GetProcessorFunc>(dlsym(handle, GET_PASTE_DATA_PROCESSOR));
    if (GetProcessor == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Can not get ProcessorFunc");
        dlclose(handle);
        return;
    }
    IPasteDataProcessor &processor = GetProcessor();
    std::string entity = "";
    int32_t result = processor.Process(primaryText, entity);
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "AI Process failed, result=%{public}d", result);
        dlclose(handle);
        return;
    }
    std::string location = "";
    int32_t ret = ExtractEntity(entity, location);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ExtractEntity failed, ret=%{public}d", ret);
        dlclose(handle);
        return;
    }
    NotifyEntityObservers(location, EntityType::ADDRESS, static_cast<uint32_t>(primaryText.size()));
    dlclose(handle);
}

void PasteboardService::RecognizePasteData(PasteData &pasteData)
{
    if (pasteData.GetShareOption() == ShareOption::InApp) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "shareOption is InApp, recognition not allowed");
        return;
    }
    std::string primaryText = GetAllPrimaryText(pasteData);
    if (primaryText.empty()) {
        return;
    }
    FFRTTask task = [this, primaryText]() {
        std::thread thread([=]() {
            PASTEBOARD_CHECK_AND_RETURN_LOGE(PasteboardService::state_ == ServiceRunningState::STATE_RUNNING,
                PASTEBOARD_MODULE_SERVICE, "PasteboardService is not running.");
            OnRecognizePasteData(primaryText);
        });
        thread.detach();
    };
    FFRTUtils::SubmitTask(task);
}

int32_t PasteboardService::SubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver> &observer)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(static_cast<uint32_t>(entityType) < static_cast<uint32_t>(EntityType::MAX),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read entityType data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        observer != nullptr, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read observer data");
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
        "start, type=%{public}u, len=%{public}u", static_cast<uint32_t>(entityType), expectedDataLength);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(expectedDataLength <= MAX_RECOGNITION_LENGTH,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "expected data length exceeds limitation");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(VerifyPermission(tokenId),
        static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR), PASTEBOARD_MODULE_SERVICE,
        "check permission failed");
    auto callingPid = IPCSkeleton::GetCallingPid();
    bool result = entityObserverMap_.ComputeIfPresent(
        callingPid, [entityType, expectedDataLength, tokenId, &observer](auto, auto &observerList) {
            auto it = std::find_if(observerList.begin(), observerList.end(),
                [entityType, expectedDataLength](const EntityObserverInfo &observer) {
                    return observer.entityType == entityType && observer.expectedDataLength == expectedDataLength;
                });
            if (it != observerList.end()) {
                it->tokenId = tokenId;
                it->observer = observer;
                return true;
            }
            observerList.emplace_back(entityType, expectedDataLength, tokenId, observer);
            return true;
        });
    if (!result) {
        std::vector<EntityObserverInfo> observerList;
        observerList.emplace_back(entityType, expectedDataLength, tokenId, observer);
        entityObserverMap_.Emplace(callingPid, observerList);
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "subscribe entityObserver finished");
    return ERR_OK;
}

int32_t PasteboardService::UnsubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver> &observer)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(static_cast<uint32_t>(entityType) < static_cast<uint32_t>(EntityType::MAX),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read entityType data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        observer != nullptr, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to read observer data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(expectedDataLength <= MAX_RECOGNITION_LENGTH,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "expected data length exceeds limitation");
    auto callingPid = IPCSkeleton::GetCallingPid();
    auto result =
        entityObserverMap_.ComputeIfPresent(callingPid, [entityType, expectedDataLength](auto, auto &observerList) {
            auto it = std::find_if(observerList.begin(), observerList.end(),
                [entityType, expectedDataLength](const EntityObserverInfo &observer) {
                    return observer.entityType == entityType && observer.expectedDataLength == expectedDataLength;
                });
            if (it == observerList.end()) {
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
                    "Failed to unsubscribe, observer not found, type is %{public}u, length is %{public}u.",
                    static_cast<uint32_t>(entityType), expectedDataLength);
                return true;
            }
            observerList.erase(it);
            if (observerList.empty()) {
                return false;
            }
            return true;
        });
    return ERR_OK;
}

void PasteboardService::UnsubscribeAllEntityObserver()
{
    entityObserverMap_.Clear();
}

int32_t PasteboardService::GetRecordValueByType(uint32_t dataId, uint32_t recordId, int64_t &rawDataSize,
    std::vector<uint8_t> &buffer, int &fd)
{
    MessageParcelWarp messageReply;
    if (rawDataSize <= 0 || rawDataSize > messageReply.GetRawDataSize()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid raw data size:%{public}" PRId64, rawDataSize);
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    bool ret = false;
    PasteDataEntry entryValue;
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        auto actualSize = AshmemGetSize(fd);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(actualSize >= 0 && rawDataSize <= actualSize,
            static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE), PASTEBOARD_MODULE_SERVICE,
            "rawDataSize invalid, actualSize=%{public}d, rawDataSize:%{public}" PRId64, actualSize, rawDataSize);
        void *ptr = ::mmap(nullptr, rawDataSize, PROT_READ, MAP_SHARED, fd, 0);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ptr != MAP_FAILED,
            static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "mmap failed, fd:%{public}d size:%{public}" PRId64, fd, rawDataSize);
        const uint8_t *rawData = reinterpret_cast<const uint8_t *>(ptr);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(rawData != nullptr,
            static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "Failed to get raw data, size=%{public}" PRId64, rawDataSize);
        std::vector<uint8_t> pasteDataTlv(rawData, rawData + rawDataSize);
        ret = entryValue.Decode(pasteDataTlv);
        ::munmap(ptr, rawDataSize);
    } else {
        ret = entryValue.Decode(buffer);
    }
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fail to decode paste data entry");
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    }
    auto result = GetRecordValueByType(dataId, recordId, entryValue);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result == static_cast<int32_t>(PasteboardError::E_OK), result,
        PASTEBOARD_MODULE_SERVICE, "get record value failed, type=%{public}s, ret=%{public}d",
        entryValue.GetUtdId().c_str(), result);
    rawDataSize = 0;
    std::vector<uint8_t>().swap(buffer);
    fd = -1;
    return GetRecordValueByType(rawDataSize, buffer, fd, entryValue);
}

int32_t PasteboardService::GetRecordValueByType(int64_t &rawDataSize,
    std::vector<uint8_t> &buffer, int32_t &fd, const PasteDataEntry &entryValue)
{
    std::vector<uint8_t> entryValueTLV(0);
    bool ret = entryValue.Encode(entryValueTLV);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "fail encode entry value");
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    }
    rawDataSize = static_cast<int64_t>(entryValueTLV.size());
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        if (!WriteRawData(entryValueTLV.data(), rawDataSize, fd)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to WriteRawData");
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
        std::vector<uint8_t>().swap(entryValueTLV);
    } else {
        fd = AshmemCreate("PasteboardTmpAshmem", 1);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(fd >= 0,
            static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "ashmem create failed");
        buffer = std::move(entryValueTLV);
    }

    return ERR_OK;
}

int32_t PasteboardService::GetRecordValueByType(uint32_t dataId, uint32_t recordId, PasteDataEntry &value)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto callPid = IPCSkeleton::GetCallingPid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((dataId == delayDataId_ && tokenId == delayTokenId_) ||
        VerifyPermission(tokenId), static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR),
        PASTEBOARD_MODULE_SERVICE, "check permission failed, calling pid is %{public}d", callPid);

    auto appInfo = GetAppInfo(tokenId);
    auto [hasData, data] = clips_.Find(appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", dataId, data->GetDataId());

    auto record = data->GetRecordById(recordId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID),
        PASTEBOARD_MODULE_SERVICE, "recordId=%{public}u invalid, max=%{public}zu", recordId, data->GetRecordCount());

    std::string utdId = value.GetUtdId();
    auto entry = record->GetEntry(utdId);
    bool isRemoteData = data->IsRemote();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, static_cast<int32_t>(PasteboardError::INVALID_MIMETYPE),
        PASTEBOARD_MODULE_SERVICE, "entry is null, recordId=%{public}u, type=%{public}s", recordId, utdId.c_str());

    if (isRemoteData && !entry->HasContent(utdId)) {
        int32_t ret = GetRemoteEntryValue(appInfo, *data, *record, value);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "get remote entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    int32_t ret = GetLocalEntryValue(appInfo.userId, *data, *record, value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);

    std::string mimeType = value.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_HTML) {
        return ProcessDelayHtmlEntry(*data, appInfo, value);
    }
    if (mimeType == MIMETYPE_TEXT_URI) {
        bool isInvalid = (isRemoteData || tokenId != data->GetTokenId()) &&
            PasteboardWebController::GetInstance().RemoveInvalidUri(value);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!isInvalid, static_cast<int32_t>(PasteboardError::INVALID_URI_ERROR),
            PASTEBOARD_MODULE_SERVICE, "uri invalid");
        std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
            *data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
        return GrantUriPermission(grantUris, appInfo.bundleName, isRemoteData, appInfo.appIndex);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo,
    PasteDataEntry &entry)
{
    const auto &targetBundle = targetAppInfo.bundleName;
    const auto &appIndex = targetAppInfo.appIndex;
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (!PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, targetAppInfo.userId)) {
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }

    PasteData tmp;
    bool isRemoteData = data.IsRemote();
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    tmp.SetRemote(isRemoteData);
    SetLocalPasteFlag(tmp.IsRemote(), targetAppInfo.tokenId, tmp);
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, targetAppInfo.userId);
    PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
    PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);

    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(tmp, std::make_pair(targetBundle, appIndex));
    int32_t ret = GrantUriPermission(grantUris, targetBundle, isRemoteData, appIndex);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "grant to %{public}s:%{public}d failed, ret=%{public}d", targetBundle.c_str(),
        appIndex, ret);

    return PostProcessDelayHtmlEntry(tmp, targetAppInfo, entry);
}

int32_t PasteboardService::PostProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo,
    PasteDataEntry &entry)
{
    PasteboardWebController::GetInstance().RetainUri(data);
    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    PasteboardWebController::GetInstance().RebuildWebviewPasteData(data, targetAppInfo.bundleName,
        targetAppInfo.appIndex);

    std::shared_ptr<std::string> html = data.GetPrimaryHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::REBUILD_HTML_FAILED),
        PASTEBOARD_MODULE_SERVICE, "rebuild html failed");

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(*html);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::HTML_CONTENT] = *html;
        entry.SetValue(newObject);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

bool PasteboardService::VerifyPermission(uint32_t tokenId)
{
    auto version = GetSdkVersion(tokenId);
    auto callPid = IPCSkeleton::GetCallingPid();
    if (version == INVALID_VERSION) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "get hap version failed, callPid is %{public}d, tokenId is %{public}d", callPid, tokenId);
        return false;
    }
    auto isReadGrant = PermissionUtils::IsPermissionGranted(READ_PASTEBOARD_PERMISSION, tokenId);
    auto isSecureGrant = PermissionUtils::IsPermissionGranted(SECURE_PASTE_PERMISSION, tokenId);
    AddPermissionRecord(tokenId, isReadGrant, isSecureGrant);
    if (isSecureGrant || isReadGrant) {
        return true;
    }
    auto tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    bool isAllowTokenAccess = (tokenType == ATokenTypeEnum::TOKEN_NATIVE || tokenType == ATokenTypeEnum::TOKEN_SHELL);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
        "isReadGrant is %{public}d, isSecureGrant is %{public}d, isAllowTokenAccess is %{public}d", isReadGrant,
        isSecureGrant, isAllowTokenAccess);
    bool isCtrlVAction = false;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (inputEventCallback_ != nullptr) {
            isCtrlVAction = inputEventCallback_->IsCtrlVProcess(callPid, IsFocusedApp(tokenId));
            inputEventCallback_->Clear();
        }
    }
    auto isGrant = isReadGrant || isSecureGrant || isAllowTokenAccess || isCtrlVAction;
    if (!isGrant && version >= ADD_PERMISSION_CHECK_SDK_VERSION) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "no permission, callPid is %{public}d, version is %{public}d",
            callPid, version);
        return false;
    }
    return true;
}

int32_t PasteboardService::IsDataValid(PasteData &pasteData, uint32_t tokenId)
{
    if (pasteData.IsDraggedData() || !pasteData.IsValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is invalid");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    if (IsDataAged()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is aged");
        return static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR);
    }
    auto screenStatus = GetCurrentScreenStatus();
    if (pasteData.GetScreenStatus() > screenStatus) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "current screen is %{public}d, set data screen is %{public}d.",
            screenStatus, pasteData.GetScreenStatus());
        return static_cast<int32_t>(PasteboardError::CROSS_BORDER_ERROR);
    }
    switch (pasteData.GetShareOption()) {
        case ShareOption::InApp: {
            if (pasteData.GetTokenId() != tokenId) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "InApp check failed.");
                return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
            }
            break;
        }
        case ShareOption::LocalDevice: {
            break;
        }
        case ShareOption::CrossDevice: {
            break;
        }
        default: {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "tokenId = 0x%{public}x, shareOption = %{public}d is error.",
                tokenId, pasteData.GetShareOption());
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
        }
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetSdkVersion(uint32_t tokenId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "caller is not application");
        return 0;
    }
    HapTokenInfo hapTokenInfo;
    auto ret = AccessTokenKit::GetHapTokenInfo(tokenId, hapTokenInfo);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "GetHapTokenInfo fail, tokenid is %{public}u, ret is %{public}d.",
            tokenId, ret);
        return INVALID_VERSION;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "ver:%{public}d.", hapTokenInfo.apiVersion);
    return hapTokenInfo.apiVersion;
}

bool PasteboardService::IsDataAged()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "IsDataAged start");
    auto userId = GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return true;
    }
    auto it = copyTime_.Find(userId);
    if (!it.first) {
        return true;
    }
    return false;
}

AppInfo PasteboardService::GetAppInfo(uint32_t tokenId)
{
    AppInfo info;
    info.tokenId = tokenId;
    info.tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    info.userId = GetCurrentAccountId();
    switch (info.tokenType) {
        case ATokenTypeEnum::TOKEN_HAP: {
            HapTokenInfo hapInfo;
            if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get hap token info fail.");
                info.userId = -1;
                return info;
            }
            info.bundleName = hapInfo.bundleName;
            info.appIndex = hapInfo.instIndex;
            info.userId = hapInfo.userID;
            break;
        }
        case ATokenTypeEnum::TOKEN_NATIVE:
        case ATokenTypeEnum::TOKEN_SHELL: {
            NativeTokenInfo tokenInfo;
            if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get native token info fail.");
                return info;
            }
            info.bundleName = tokenInfo.processName;
            break;
        }
        default: {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "tokenType = %{public}d not match.", info.tokenType);
        }
    }
    return info;
}

std::string PasteboardService::GetAppBundleName(const AppInfo &appInfo)
{
    std::string bundleName;
    if (appInfo.userId != ERROR_USERID) {
        bundleName = appInfo.bundleName;
    } else {
        bundleName = "error";
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "GetAppInfo error");
    }
    return bundleName;
}

void PasteboardService::SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData &pasteData)
{
    pasteData.SetLocalPasteFlag(!isCrossPaste && tokenId == pasteData.GetTokenId());
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "isLocalPaste = %{public}d.", pasteData.IsLocalPaste());
}

int32_t PasteboardService::ShowProgress(const std::string &progressKey, const sptr<IRemoteObject> &observer)
{
    if (!HasPasteData()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "not pastedata, no need to show progress.");
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (!IsFocusedApp(tokenId)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "not focused app, no need to show progress.");
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    }
    PasteBoardDialog::ProgressMessageInfo message;
    std::string deviceName = "";
    bool isRemote = false;
    auto result = (GetRemoteDeviceName(deviceName, isRemote) == ERR_OK);
    if (result && isRemote) {
        message.promptText = "PromptText_PasteBoard_Remote";
        message.remoteDeviceName = deviceName;
    } else {
        message.promptText = "PromptText_PasteBoard_Local";
        message.remoteDeviceName = "";
    }
    message.isRemote = isRemote;
    message.progressKey = progressKey;

    FocusedAppInfo appInfo = GetFocusedAppInfo();
    message.windowId = appInfo.windowId;
    message.callerToken = appInfo.abilityToken;
    message.clientCallback = observer;
    PasteBoardDialog::GetInstance().ShowProgress(message);
    return ERR_OK;
}

bool PasteboardService::WriteRawData(const void *data, int64_t size, int &serFd)
{
    MessageParcelWarp messageData;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(data != nullptr, false, PASTEBOARD_MODULE_SERVICE, "data is null");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(0 < size && size <= messageData.GetRawDataSize(), false,
        PASTEBOARD_MODULE_SERVICE, "size invalid, size:%{public}" PRId64, size);

    int fd = AshmemCreate("WriteRawData Ashmem", size);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(fd >= 0, false, PASTEBOARD_MODULE_SERVICE, "ashmem create failed");

    int32_t result = AshmemSetProt(fd, PROT_READ | PROT_WRITE);
    if (result < 0) {
        close(fd);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ashmem set prot failed");
        return false;
    }
    void *ptr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "mmap failed, fd:%{public}d", fd);
        close(fd);
        return false;
    }
    if (!messageData.MemcpyData(ptr, static_cast<size_t>(size), data, size)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "memcpy_s failed, fd:%{public}d", fd);
        ::munmap(ptr, size);
        close(fd);
        return false;
    }
    ::munmap(ptr, size);
    serFd = fd;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Write data end. fd:%{public}d size:%{public}" PRId64, serFd, size);
    return true;
}

CommonInfo PasteboardService::GetCommonState(int64_t dataSize)
{
    CommonInfo commonInfo;
    commonInfo.currentAccountId = GetCurrentAccountId();
    commonInfo.deviceType = DMAdapter::GetInstance().GetLocalDeviceType();
    commonInfo.dataSize = dataSize;
    return commonInfo;
}

void PasteboardService::SetRadarEvent(const AppInfo &appInfo, PasteData &data, bool isPeerOnline,
    RadarReportInfo &radarReportInfo, const std::string &peerNetId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(peerNetId, remoteDevice);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        DeviceManager::GetInstance().GetNetworkTypeByNetworkId(PASTEBOARD_SERVICE_SA_NAME, peerNetId,
            radarReportInfo.pasteInfo.networkType);
    }
#endif
    std::string peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
    radarReportInfo.stageRes = DFX_SUCCESS;
    radarReportInfo.bundleName = appInfo.bundleName;
    radarReportInfo.description = data.GetReportDescription();
    radarReportInfo.pasteInfo.onlineDevNum = DMAdapter::GetInstance().GetNetworkIds().size();
    radarReportInfo.pasteInfo.peerNetId = PasteboardDfxUntil::GetAnonymousID(peerNetId);
    radarReportInfo.pasteInfo.peerUdid = PasteboardDfxUntil::GetAnonymousID(peerUdid);
    radarReportInfo.pasteInfo.peerBundleName = data.GetOriginAuthority().first;
    radarReportInfo.pasteInfo.isPeerOnline = isPeerOnline;
}

void PasteboardService::SetUeEvent(const AppInfo &appInfo, PasteData &data, bool isPeerOnline,
    UeReportInfo &ueReportInfo, const std::string &peerNetId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(peerNetId, remoteDevice);
    if (ret == static_cast<int32_t>(PasteboardError::E_OK)) {
        DeviceManager::GetInstance().GetNetworkTypeByNetworkId(PASTEBOARD_SERVICE_SA_NAME, peerNetId,
            ueReportInfo.pasteInfo.networkType);
    }
#endif
    ueReportInfo.bundleName = appInfo.bundleName;
    ueReportInfo.dataType = data.GenerateDataType();
    ueReportInfo.pasteInfo.peerBundleName = data.GetOriginAuthority().first;
    ueReportInfo.pasteInfo.isDistributed = data.IsRemote();
    ueReportInfo.pasteInfo.isPeerOnline = isPeerOnline;
    ueReportInfo.pasteInfo.onlineDevNum = DMAdapter::GetInstance().GetNetworkIds().size();
    ueReportInfo.description = data.GetReportDescription();
}

int32_t PasteboardService::GetPasteData(int &fd, int64_t &size, std::vector<uint8_t> &rawData,
    const std::string &pasteId, int32_t &syncTime, int32_t &realErrCode)
{
    fd = -1;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(PasteData::IsValidPasteId(pasteId),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Parameter error. invalid pasteId=%{public}s", pasteId.c_str());
    UeReportInfo ueReportInfo;
    int32_t ret = GetPasteDataInner(fd, size, rawData, pasteId, syncTime, ueReportInfo);
    if (fd == -1) {
        fd = AshmemCreate("GetPasteData Ashmem", 1);
    }
    ueReportInfo.ret = (ret == static_cast<int32_t>(PasteboardError::E_OK) ? E_OK_OPERATION : ret);
    ueReportInfo.commonInfo = GetCommonState(size);
    UE_REPORT(UE_PASTE, ueReportInfo,
        "IS_DISTRIBUTED_PASTEBOARD", ueReportInfo.pasteInfo.isDistributed,
        "RECORD_NUM", ueReportInfo.description.recordNum,
        "DATA_SIZE", ueReportInfo.commonInfo.dataSize,
        "CURRENT_ACCOUNT_ID", ueReportInfo.commonInfo.currentAccountId,
        "PEER_BUNDLE_NAME", ueReportInfo.pasteInfo.peerBundleName,
        "IS_PEER_ONLINE", ueReportInfo.pasteInfo.isPeerOnline,
        "ONLINE_DEV_NUM", ueReportInfo.pasteInfo.onlineDevNum,
        "NETWORK_TYPE", ueReportInfo.pasteInfo.networkType,
        "ENTRY_NUM", ueReportInfo.description.entryNum,
        "MIMETYPES", ueReportInfo.description.mimeTypes);
    realErrCode = ret;
    return 0;
}

int32_t PasteboardService::GetPasteDataInner(int &fd, int64_t &size, std::vector<uint8_t> &rawData,
    const std::string &pasteId, int32_t &syncTime, UeReportInfo &ueReportInfo)
{
    PasteboardTrace tracer("PasteboardService GetPasteData");
    PasteData data{};
    data.SetPasteId(pasteId);
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = GetAppInfo(tokenId);
    bool developerMode = OHOS::system::GetBoolParameter("const.security.developermode.state", false);
    bool isTestServerSetPasteData = developerMode && setPasteDataUId_ == TEST_SERVER_UID;
    if (!VerifyPermission(tokenId) && !isTestServerSetPasteData) {
        RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_CHECK_GET_AUTHORITY, DFX_SUCCESS, GET_DATA_APP, appInfo.bundleName,
            RadarReporter::CONCURRENT_ID, data.GetPasteId());
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "check permission failed, callingPid is %{public}d", callPid);
        HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE,
            static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    bool isPeerOnline = false;
    std::string peerNetId = "";
    std::string peerUdid = "";
    RadarReportInfo radarReportInfo;
    radarReportInfo.pasteInfo.pasteId = data.GetPasteId();
    auto ret = GetData(tokenId, data, syncTime, isPeerOnline, peerNetId, peerUdid);
    data.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    SetUeEvent(appInfo, data, isPeerOnline, ueReportInfo, peerNetId);
    SetRadarEvent(appInfo, data, isPeerOnline, radarReportInfo, peerNetId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "data is invalid, ret is %{public}d, callPid is %{public}d, tokenId is %{public}d", ret, callPid, tokenId);
        HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE, ret);
        radarReportInfo.commonInfo = GetCommonState(-1);
        PASTE_RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_GET_DATA_INFO, radarReportInfo);
        return ret;
    }
    delayDataId_ = data.GetDataId();
    delayTokenId_ = tokenId;

    ret = DealData(fd, size, rawData, data);
    radarReportInfo.commonInfo = GetCommonState(size);
    PASTE_RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_GET_DATA_INFO, radarReportInfo);
    return ret;
}

int32_t PasteboardService::DealData(int &fd, int64_t &size, std::vector<uint8_t> &rawData, PasteData &data)
{
    std::vector<uint8_t> pasteDataTlv(0);
    {
        std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
        if (!data.Encode(pasteDataTlv)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to encode pastedata in TLV");
            HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE, ERR_INVALID_VALUE);
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
    }
    int64_t tlvSize = static_cast<int64_t>(pasteDataTlv.size());
    int serviceFd = -1;
    if (tlvSize > MIN_ASHMEM_DATA_SIZE) {
        bool res = WriteRawData(pasteDataTlv.data(), tlvSize, serviceFd);
        if (!res) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to WriteRawData:%{public}" PRId64, tlvSize);
            return static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR);
        }
        pasteDataTlv.clear();
    } else {
        serviceFd = AshmemCreate("DealData Ashmem", 1);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(serviceFd >= 0,
            static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "create fd failed");
    }
    size = tlvSize;
    fd = serviceFd;
    rawData = std::move(pasteDataTlv);
    HiViewAdapter::ReportUseBehaviour(data, HiViewAdapter::PASTE_STATE, ERR_OK);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "DealData fd:%{public}d, size:%{public}" PRId64, fd, size);
    return ERR_OK;
}

void PasteboardService::AddPermissionRecord(uint32_t tokenId, bool isReadGrant, bool isSecureGrant)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return;
    }
    bool isGrant = isReadGrant || isSecureGrant;
    if (!isGrant) {
        return;
    }
    auto permUsedType = static_cast<PermissionUsedType>(AccessTokenKit::GetPermissionUsedType(
        tokenId, isSecureGrant ? SECURE_PASTE_PERMISSION : READ_PASTEBOARD_PERMISSION));
    AddPermParamInfo info;
    info.tokenId = tokenId;
    info.permissionName = READ_PASTEBOARD_PERMISSION;
    info.successCount = 1;
    info.failCount = 0;
    info.type = permUsedType;
    int32_t result = PrivacyKit::AddPermissionUsedRecord(info);
    if (result != RET_SUCCESS) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "add record failed, result is %{public}d", result);
    }
    return;
}

int32_t PasteboardService::CheckAndGrantRemoteUri(PasteData &data, const AppInfo &appInfo,
    const std::string &pasteId, const std::string &deviceId, std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    int64_t fileSize = data.GetFileSize();
    bool isRemoteData = data.IsRemote();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pasteId=%{public}s, isRemote=%{public}s, fileSize=%{public}" PRId64,
        pasteId.c_str(), isRemoteData ? "true" : "false", fileSize);
    GetPasteDataDot(data, appInfo.bundleName, appInfo.userId);
    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (isRemoteData) {
        data.SetPasteId(pasteId);
        data.deviceId_ = deviceId;
        if (pasteBlock) {
            if (!grantUris.empty()) {
                pasteBlock->GetValue();
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "wait P2PEstablish finish");
            } else {
                PasteComplete(deviceId, pasteId);
            }
        }
    }
    ClearP2PEstablishTaskInfo();
    return GrantUriPermission(grantUris, appInfo.bundleName, isRemoteData, appInfo.appIndex);
}

int32_t PasteboardService::GetData(uint32_t tokenId, PasteData &data, int32_t &syncTime, bool &isPeerOnline,
    std::string &peerNetId, std::string &peerUdid)
{
    CalculateTimeConsuming::SetBeginTime();
    auto appInfo = GetAppInfo(tokenId);
    int32_t result = static_cast<int32_t>(PasteboardError::E_OK);
    std::string pasteId = data.GetPasteId();
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = nullptr;
    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
    pasteBlock = EstablishP2PLinkTask(pasteId, distEvt);
    if (distRet != static_cast<int32_t>(PasteboardError::E_OK) ||
        GetCurrentScreenStatus() != ScreenEvent::ScreenUnlocked) {
        result = GetLocalData(appInfo, data);
        if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
            peerNetId = currentEvent_.deviceId;
            peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
        }
    } else {
        result = GetRemoteData(appInfo.userId, distEvt, data, syncTime);
        peerNetId = distEvt.deviceId;
        peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
    }
    
    HandleNotificationsAndStatusChecks(appInfo, data, peerNetId, isPeerOnline);

    PublishServiceState(data, syncTime, peerNetId, pasteBlock);
    
    if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
        HandleGetDataError(result, pasteBlock, distEvt.deviceId, pasteId);
        return result;
    }
    return CheckAndGrantRemoteUri(data, appInfo, pasteId, distEvt.deviceId, pasteBlock);
}

void PasteboardService::HandleNotificationsAndStatusChecks(const AppInfo &appInfo, const PasteData &data,
    const std::string &peerNetId, bool &isPeerOnline)
{
    uint32_t observerMapSize = 0;
    {
        std::lock_guard<std::mutex> lock(observerMutex_);
        observerMapSize = observerEventMap_.size();
    }
    if (observerMapSize != 0) {
        std::string targetBundleName = GetAppBundleName(appInfo);
        NotifyObservers(targetBundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_READ);
    }
    if (!peerNetId.empty()) {
        auto peerNetIds = DMAdapter::GetInstance().GetNetworkIds();
        auto it = std::find(peerNetIds.begin(), peerNetIds.end(), peerNetId);
        isPeerOnline = (it != peerNetIds.end());
    }
}

void PasteboardService::PublishServiceState(const PasteData &data, int32_t syncTime,
    const std::string &peerNetId, std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    auto plugin = GetClipPlugin();
    bool isNeedPublishState = data.IsRemote() && syncTime != 0 && pasteBlock == nullptr && plugin != nullptr;
    if (isNeedPublishState) {
        auto status = plugin->PublishServiceState(peerNetId, ClipPlugin::ServiceStatus::IDLE);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state idle error, status:%{public}d", status);
        }
    }
}

void PasteboardService::HandleGetDataError(int32_t result, std::shared_ptr<BlockObject<int32_t>> pasteBlock,
    const std::string &deviceId, const std::string &pasteId)
{
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get local or remote data err:%{public}d", result);
    if (pasteBlock) {
        PasteComplete(deviceId, pasteId);
    }
    ClearP2PEstablishTaskInfo();
}

PasteboardService::RemoteDataTaskManager::DataTask PasteboardService::RemoteDataTaskManager::GetRemoteDataTask(
    const Event &event)
{
    auto key = event.deviceId + std::to_string(event.seqId);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dataTasks_.find(key);
    if (it == dataTasks_.end()) {
        it = dataTasks_.emplace(key, std::make_shared<TaskContext>()).first;
    }

    if (it == dataTasks_.end()) {
        return std::make_pair(nullptr, false);
    }

    return std::make_pair(it->second, it->second->pasting_.exchange(true));
}

void PasteboardService::RemoteDataTaskManager::Notify(const Event &event, std::shared_ptr<PasteDateTime> data)
{
    auto key = event.deviceId + std::to_string(event.seqId);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dataTasks_.find(key);
    if (it == dataTasks_.end()) {
        return;
    }
    auto &task = it->second;
    task->data_ = data;
    task->getDataBlocks_.ForEach([](const auto &key, auto value) -> bool {
        value->SetValue(true);
        return false;
    });
}

std::shared_ptr<PasteDateTime> PasteboardService::RemoteDataTaskManager::WaitRemoteData(const Event &event)
{
    std::shared_ptr<PasteboardService::RemoteDataTaskManager::TaskContext> task;
    {
        auto key = event.deviceId + std::to_string(event.seqId);
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = dataTasks_.find(key);
        if (it == dataTasks_.end()) {
            return nullptr;
        }

        task = it->second;
    }

    auto key = ++mapKey_;
    auto block = std::make_shared<BlockObject<bool>>(GET_REMOTE_DATA_WAIT_TIME);
    task->getDataBlocks_.InsertOrAssign(key, block);
    block->GetValue();

    task->getDataBlocks_.Erase(key);
    return task->data_;
}

void PasteboardService::RemoteDataTaskManager::ClearRemoteDataTask(const Event &event)
{
    auto key = event.deviceId + std::to_string(event.seqId);
    std::lock_guard<std::mutex> lock(mutex_);
    dataTasks_.erase(key);
}

int32_t PasteboardService::GetRemoteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime)
{
    syncTime = -1;
    auto [task, isPasting] = taskMgr_.GetRemoteDataTask(event);
    if (task == nullptr) {
        return static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR);
    }

    if (isPasting) {
        auto value = taskMgr_.WaitRemoteData(event);
        if (value != nullptr && value->data != nullptr) {
            syncTime = value->syncTime;
            data = *(value->data);
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        return static_cast<int32_t>(PasteboardError::TASK_PROCESSING);
    }

    auto [distRet, distEvt] = GetValidDistributeEvent(userId);
    if (distRet != static_cast<int32_t>(PasteboardError::E_OK) || !(distEvt == event)) {
        int32_t ret = distRet == static_cast<int32_t>(PasteboardError::E_OK) ?
            static_cast<int32_t>(PasteboardError::INVALID_EVENT_ERROR) : distRet;
        auto it = clips_.Find(userId);
        if (it.first) {
            data = *it.second;
            ret = static_cast<int32_t>(PasteboardError::E_OK);
        }
        taskMgr_.ClearRemoteDataTask(event);
        return ret;
    }

    return GetRemotePasteData(userId, event, data, syncTime);
}

int32_t PasteboardService::GetRemotePasteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime)
{
    auto block = std::make_shared<BlockObject<std::shared_ptr<PasteDateTime>>>(GET_REMOTE_DATA_WAIT_TIME);
    std::thread thread([this, event, block, userId]() mutable {
        auto result = GetDistributedData(event, userId);
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        std::shared_ptr<PasteDateTime> pasteDataTime = std::make_shared<PasteDateTime>();
        if (result.first != nullptr) {
            result.first->SetRemote(true);
            if (distEvt == event) {
                clips_.InsertOrAssign(userId, result.first);
                IncreaseChangeCount(userId);
                auto curTime =
                    static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
                copyTime_.InsertOrAssign(userId, curTime);
                SetDataExpirationTimer(userId);
            }
            pasteDataTime->syncTime = result.second.syncTime;
            pasteDataTime->data = result.first;
            pasteDataTime->errorCode = result.second.errorCode;
            taskMgr_.Notify(event, pasteDataTime);
        } else {
            pasteDataTime->data = nullptr;
            pasteDataTime->errorCode = result.second.errorCode;
            taskMgr_.Notify(event, pasteDataTime);
        }
        block->SetValue(pasteDataTime);
        taskMgr_.ClearRemoteDataTask(event);
    });
    thread.detach();
    auto value = block->GetValue();
    if (value != nullptr && value->data != nullptr) {
        syncTime = value->syncTime;
        data = std::move(*(value->data));
        return value->errorCode;
    } else if (value != nullptr && value->data == nullptr) {
        return value->errorCode;
    }
    return static_cast<int32_t>(PasteboardError::TIMEOUT_ERROR);
}

int32_t PasteboardService::GetLocalData(const AppInfo &appInfo, PasteData &data)
{
    std::string pasteId = data.GetPasteId();
    auto it = clips_.Find(appInfo.userId);
    auto tempTime = copyTime_.Find(appInfo.userId);
    if (!it.first || !tempTime.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "no data userId is %{public}d.", appInfo.userId);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    auto ret = IsDataValid(*(it.second), appInfo.tokenId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "paste data is invalid. ret = %{public}d "
            "appInfo.userId = %{public}d", ret, appInfo.userId);
        return ret;
    }
    data = *(it.second);
    auto originBundleName = it.second->GetBundleName();
    if (it.second->IsDelayData()) {
        GetDelayPasteData(appInfo.userId, data);
        RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_CHECK_GET_DELAY_PASTE, DFX_SUCCESS, CONCURRENT_ID, pasteId);
    }
    if (it.second->IsDelayRecord()) {
        GetDelayPasteRecord(appInfo.userId, data);
    }
    data.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    auto result = copyTime_.Find(appInfo.userId);
    if (!result.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId not found userId is %{public}d", appInfo.userId);
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    auto curTime = result.second;
    if (tempTime.second == curTime) {
        bool isNotify = false;
        clips_.ComputeIfPresent(appInfo.userId, [&data, &isNotify](auto &key, auto &value) {
            if (value->IsDelayData()) {
                value = std::make_shared<PasteData>(data);
                isNotify = true;
            }
            if (value->IsDelayRecord()) {
                value = std::make_shared<PasteData>(data);
            }
            return true;
        });
        if (isNotify) {
            NotifyObservers(originBundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_WRITE);
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPasteData success. appInfo.userId = %{public}d", appInfo.userId);
    SetLocalPasteFlag(data.IsRemote(), appInfo.tokenId, data);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::GetDelayPasteData(int32_t userId, PasteData &data)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "get delay data start");
    delayGetters_.ComputeIfPresent(userId, [this, &data, userId](auto, auto &delayGetter) {
        PasteData delayData;
        if (delayGetter.first != nullptr) {
            delayGetter.first->GetPasteData("", delayData);
        }
        if (delayGetter.second != nullptr && delayGetter.first != nullptr) {
            delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
        }
        delayData.SetDelayData(false);
        delayData.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
        delayData.SetOriginAuthority(data.GetOriginAuthority());
        delayData.SetTime(data.GetTime());
        delayData.SetTokenId(data.GetTokenId());
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(delayData, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(delayData, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(delayData);
        data = delayData;
        return false;
    });
}

int32_t PasteboardService::GetDelayPasteRecord(int32_t userId, PasteData &data)
{
    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    auto delayEntryInfos = DelayManager::GetPrimaryDelayEntryInfo(data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!delayEntryInfos.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no delay entry");
    DelayManager::GetLocalEntryValue(delayEntryInfos, getter.first, data);
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::ClearP2PEstablishTaskInfo()
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pEstablishInfo_.networkId.clear();
    p2pEstablishInfo_.pasteBlock = nullptr;
}

void PasteboardService::OpenP2PLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
#endif
    auto plugin = GetClipPlugin();
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
    int32_t status = plugin->ApplyAdvancedResource(networkId);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == RESULT_OK, PASTEBOARD_MODULE_SERVICE,
        "apply resource failed, deviceId=%{public}.5s, status=%{public}d", networkId.c_str(), status);

    status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == RESULT_OK, PASTEBOARD_MODULE_SERVICE,
        "publish CONNECT_SUCC failed, deviceId=%{public}.5s, status=%{public}d", networkId.c_str(), status);

#ifdef PB_DEVICE_MANAGER_ENABLE
    status = DistributedFileDaemonManager::GetInstance().OpenP2PConnection(remoteDevice);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status:%{public}d", status);
        plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
#endif
}

void PasteboardService::EstablishP2PLink(const std::string &networkId, const std::string &pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Compute(networkId, [pasteId, callPid](const auto &key, auto &value) {
            value.Compute(pasteId, [callPid](const auto &key, auto &value) {
                value.callPid = callPid;
                value.isSuccess = false;
                return true;
            });
            return true;
        });
    }
    if (ffrtTimer_) {
        FFRTTask task = [this, networkId, pasteId] {
            std::thread thread([=]() {
                PasteComplete(networkId, pasteId);
            });
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    OpenP2PLink(networkId);
#endif
}

std::shared_ptr<BlockObject<int32_t>> PasteboardService::CheckAndReuseP2PLink(
    const std::string &networkId, const std::string &pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [pasteId, callPid](const auto &key, auto &value) {
        value.Compute(pasteId, [callPid](const auto &key, auto &value) {
            value.callPid = callPid;
            value.isSuccess = false;
            return true;
        });
        return true;
    });
    if (ffrtTimer_) {
        FFRTTask task = [this, networkId, pasteId] {
            std::thread thread([=]() {
                PasteComplete(networkId, pasteId);
            });
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    auto p2pNetwork = p2pMap_.Find(networkId);
    bool isP2pSuccess = p2pNetwork.first && p2pNetwork.second.Find(P2P_PRESYNC_ID).first &&
        p2pNetwork.second.Find(P2P_PRESYNC_ID).second.isSuccess == true;
    if (isP2pSuccess) {
        if (ffrtTimer_) {
            std::string taskName = P2P_PRESYNC_ID + networkId;
            ffrtTimer_->CancelTimer(taskName);
        }
        p2pMap_.ComputeIfPresent(networkId, [this](const auto &key, auto &value) {
            value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
                return false;
            });
            return true;
        });
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "No Need P2pEstablish");
        std::shared_ptr<BlockObject<int32_t>> result = nullptr;
        auto p2pIter = preSyncP2pMap_.find(networkId);
        if (p2pIter != preSyncP2pMap_.end()) {
            result = p2pIter->second;
        }
        preSyncP2pMap_.erase(networkId);
        return result;
    }
    return nullptr;
#else
    return nullptr;
#endif
}

bool PasteboardService::IsContainUri(const std::vector<std::string> &dataType)
{
    std::vector<std::string> keyVecs;
    keyVecs.push_back(MIMETYPE_TEXT_URI);
    keyVecs.push_back(MIMETYPE_TEXT_HTML);
    bool result = std::any_of(keyVecs.begin(), keyVecs.end(), [dataType](const std::string &key) {
        return std::find(dataType.begin(), dataType.end(), key) != dataType.end();
    });
    return result;
}

void PasteboardService::OnEstablishP2PLinkTask(const std::string &networkId,
    std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteBlock != nullptr, PASTEBOARD_MODULE_SERVICE, "block is nullptr");
    OpenP2PLink(networkId);
    pasteBlock->SetValue(SET_VALUE_SUCCESS);
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    auto findResult = p2pMap_.Find(networkId);
    if (!findResult.first || findResult.second.Empty()) {
        CloseP2PLink(networkId);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "P2pEstablish Finish");
}

std::shared_ptr<BlockObject<int32_t>> PasteboardService::EstablishP2PLinkTask(
    const std::string &pasteId, const ClipPlugin::GlobalEvent &event)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    const std::string &networkId = event.deviceId;
    if (networkId.empty() || networkId == DMAdapter::GetInstance().GetLocalNetworkId()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "local device");
        return nullptr;
    }
    if (!IsContainUri(event.dataType)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "no MIMETYPE_TEXT_URI and no MIMETYPE_TEXT_HTML");
        return nullptr;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLinkTask enter");
    std::shared_ptr<BlockObject<int32_t>> result = CheckAndReuseP2PLink(networkId, pasteId);
    if (result) {
        return result;
    }
    if (!ffrtTimer_) {
        return nullptr;
    }
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
    if (!pasteBlock) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc BlockObject");
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pEstablishInfo_.networkId = networkId;
        p2pEstablishInfo_.pasteBlock = pasteBlock;
    }
    FFRTTask p2pTask = [networkId, pasteBlock, this] {
        std::thread thread([=]() {
            OnEstablishP2PLinkTask(networkId, pasteBlock);
        });
        thread.detach();
    };
    std::string taskName = pasteId + P2P_ESTABLISH_STR;
    ffrtTimer_->SetTimer(taskName, p2pTask);
    return pasteBlock;
#else
    return nullptr;
#endif
}

void PasteboardService::CloseP2PLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseP2PLink enter");
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
        return;
    }
    auto status = DistributedFileDaemonManager::GetInstance().CloseP2PConnection(remoteDevice);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "close p2p error, status:%{public}d", status);
    }
    auto plugin = GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(plugin != nullptr, PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
    status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state idle error, status:%{public}d", status);
    }
#endif
}

int32_t PasteboardService::PasteStart(const std::string &pasteId)
{
    if (ffrtTimer_) {
        ffrtTimer_->CancelTimer(pasteId);
    }
    return ERR_OK;
}

int32_t PasteboardService::PasteComplete(const std::string &deviceId, const std::string &pasteId)
{
    if (deviceId.empty()) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "deviceId is empty");
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "deviceId is %{public}.6s, taskId is %{public}s", deviceId.c_str(),
        pasteId.c_str());
    RADAR_REPORT(RadarReporter::DFX_GET_PASTEBOARD, RadarReporter::DFX_DISTRIBUTED_FILE_END, RadarReporter::DFX_SUCCESS,
        RadarReporter::BIZ_STATE, RadarReporter::DFX_END, RadarReporter::CONCURRENT_ID, pasteId);
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(deviceId, [pasteId, deviceId, this](const auto &key, auto &value) {
        value.ComputeIfPresent(pasteId, [deviceId](const auto &key, auto &value) {
            return false;
        });
        if (value.Empty()) {
            CloseP2PLink(deviceId);
            return false;
        }
        return true;
    });
    return ERR_OK;
}

int32_t PasteboardService::GetRemoteDeviceName(std::string &deviceName, bool &isRemote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter");
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    auto event = GetValidDistributeEvent(appInfo.userId);
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    if (!event.second.deviceId.empty()) {
        auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(event.second.deviceId, remoteDevice);
        if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
            return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
        }
        deviceName = remoteDevice.deviceName;
        isRemote = true;
    } else {
        deviceName = "local";
        isRemote = false;
    }
#endif
    if (deviceName.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to get remote device name");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    return ERR_OK;
}

int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    const std::string &targetBundleName, int32_t appIndex)
{
    size_t offset = 0;
    size_t length = grantUris.size();
    size_t count = PasteData::URI_BATCH_SIZE;
    bool hasGranted = false;
    int32_t permissionCode = 0;
    int32_t ret = 0;
    int32_t userId = GetCurrentAccountId();
    auto [hasData, data] = clips_.Find(userId);
    uint32_t srcTokenId = (hasData && data) ? data->GetTokenId() : 0;
    while (length > offset) {
        if (length - offset < PasteData::URI_BATCH_SIZE) {
            count = length - offset;
        }
        auto sendValues = std::vector<Uri>(grantUris.begin() + offset, grantUris.begin() + offset + count);
        if (isRemoteData) {
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermissionPrivileged(
                sendValues, permFlag, targetBundleName, appIndex);
        } else {
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermission(
                sendValues, permFlag, targetBundleName, appIndex, srcTokenId);
        }
        hasGranted = hasGranted || (permissionCode == 0);
        ret = permissionCode == 0 ? ret : permissionCode;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "permissionCode is %{public}d", permissionCode);
        offset += count;
    }
    if (hasGranted) {
        std::lock_guard<std::mutex> lock(readBundleMutex_);
        if (readBundles_.count({ targetBundleName, appIndex }) == 0) {
            readBundles_.insert({ targetBundleName, appIndex });
        }
    }
    return ret;
}

int32_t PasteboardService::GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    const std::string &targetBundleName, bool isRemoteData, int32_t appIndex)
{
    std::vector<Uri> readUris = grantUris[PasteDataRecord::READ_PERMISSION];
    std::vector<Uri> writeUris = grantUris[PasteDataRecord::READ_WRITE_PERMISSION];
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(!readUris.empty() ||
        !writeUris.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no uri");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "readUris=%{public}zu, writeUris=%{public}zu, target=%{public}s, appIndex=%{public}d",
        readUris.size(), writeUris.size(), targetBundleName.c_str(), appIndex);
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(callingUid != ANCO_SERVICE_BROKER_UID,
        static_cast<int32_t>(PasteboardError::E_OK), PASTEBOARD_MODULE_SERVICE, "callingUid = ANCO_SERVICE_BROKER_UID");
    int32_t ret = 0;
    auto permFlag = PasteDataRecord::READ_PERMISSION;
    ret = GrantPermission(readUris, permFlag, isRemoteData, targetBundleName, appIndex);
    if (!isRemoteData) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "NeedPersistance, permFlag is %{public}d", permFlag);
        permFlag = PasteDataRecord::READ_WRITE_PERMISSION;
    }
    auto result = GrantPermission(writeUris, permFlag, isRemoteData, targetBundleName, appIndex);
    ret = result == 0 ? ret : result;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, ret=%{public}d", ret);
    return ret == 0 ? static_cast<int32_t>(PasteboardError::E_OK) : ret;
}

std::map<uint32_t, std::vector<Uri>> PasteboardService::CheckUriPermission(PasteData &data,
    const std::pair<std::string, int32_t> &targetBundleAndIndex)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter");
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    std::map<uint32_t, std::vector<Uri>> result;
    std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
    for (size_t i = 0; i < data.GetRecordCount(); i++) {
        auto item = data.GetRecordAt(i);
        if (item == nullptr || (!data.IsRemote() && targetBundleAndIndex == data.GetOriginAuthority())) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "local dev & local app");
            continue;
        }
        std::shared_ptr<OHOS::Uri> uri = nullptr;
        if (!item->isConvertUriFromRemote && !item->GetConvertUri().empty()) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "clear local disUri");
            item->SetConvertUri("");
        }
        if (item->isConvertUriFromRemote && !item->GetConvertUri().empty()) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "get remote disUri");
            uri = std::make_shared<OHOS::Uri>(item->GetConvertUri());
        } else if (!item->isConvertUriFromRemote && item->GetOriginUri() != nullptr) {
            uri = item->GetOriginUri();
        }
        if (uri == nullptr) {
            continue;
        }
        auto hasGrantUriPermission = item->HasGrantUriPermission();
        const std::string &bundleName = data.GetOriginAuthority().first;
        if (!IsBundleOwnUriPermission(bundleName, *uri) && !hasGrantUriPermission) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri:%{private}s, bundleName:%{public}s, appIndex:%{public}d,"
                " has grant:%{public}d", uri->ToString().c_str(), bundleName.c_str(), data.GetOriginAuthority().second,
                hasGrantUriPermission);
            continue;
        }
        if (data.IsRemote()) {
            readUris.emplace_back(*uri);
            continue;
        }
        auto uriPermission = item->GetUriPermission();
        if (uriPermission == PasteDataRecord::READ_PERMISSION) {
            readUris.emplace_back(*uri);
        } else if (uriPermission == PasteDataRecord::READ_WRITE_PERMISSION) {
            writeUris.emplace_back(*uri);
        }
    }
    result[PasteDataRecord::READ_PERMISSION] = readUris;
    result[PasteDataRecord::READ_WRITE_PERMISSION] = writeUris;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, readUris:%{public}zu, writeUris:%{public}zu",
        readUris.size(), writeUris.size());
    return result;
}

bool PasteboardService::IsBundleOwnUriPermission(const std::string &bundleName, Uri &uri)
{
    return (bundleName.compare(uri.GetAuthority()) == 0);
}

void PasteboardService::ShowHintToast(uint32_t tokenId, uint32_t pid)
{
    PasteBoardDialog::ToastMessageInfo message;
    message.appName = GetAppLabel(tokenId);
    PasteBoardDialog::GetInstance().ShowToast(message);
}

int32_t PasteboardService::HasPasteData(bool &funcResult)
{
    funcResult = HasPasteData();
    return ERR_OK;
}

bool PasteboardService::HasPasteData()
{
    auto userId = GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }

    if (GetCurrentScreenStatus() == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            return true;
        }
    }

    auto it = clips_.Find(userId);
    if (it.first && (it.second != nullptr)) {
        auto tokenId = IPCSkeleton::GetCallingTokenID();
        auto ret = IsDataValid(*(it.second), tokenId);
        if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "pasteData is invalid, tokenId: %{public}d, userId: %{public}d,"
                "ret is %{public}d", tokenId, userId, ret);
            return false;
        }
        return true;
    }
    return false;
}

int32_t PasteboardService::GetDataTokenId(PasteData &pasteData)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto originTokenId = pasteData.GetOriginTokenId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(
        originTokenId != PasteData::INVALID_TOKEN_ID, tokenId, PASTEBOARD_MODULE_SERVICE, "originTokenId invalid");
    auto isUriProxyGrant = PermissionUtils::IsPermissionGranted(
        PermissionUtils::PERMISSION_PROXY_AUTHORIZATION_URI, tokenId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(
        !isUriProxyGrant, tokenId, PASTEBOARD_MODULE_SERVICE, "No permission, callingTokenId= %{public}u", tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "originTokenId= %{public}u.", originTokenId);
    return originTokenId;
}

int32_t PasteboardService::SaveData(PasteData &pasteData, int64_t dataSize,
    const sptr<IPasteboardDelayGetter> delayGetter, const sptr<IPasteboardEntryGetter> entryGetter)
{
    PasteboardTrace tracer("PasteboardService, SetPasteData");
    auto tokenId = pasteData.GetTokenId();
    if (!IsCopyable(tokenId)) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_SET_AUTHORITY, DFX_SUCCESS);
        return static_cast<int32_t>(PasteboardError::PROHIBIT_COPY);
    }
    if (setting_.exchange(true)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "is setting.");
        return static_cast<int32_t>(PasteboardError::TASK_PROCESSING);
    }
    CalculateTimeConsuming::SetBeginTime();
    auto appInfo = GetAppInfo(tokenId);
    if (appInfo.userId == ERROR_USERID) {
        setting_.store(false);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    pasteData.userId_ = appInfo.userId;
    SetPasteDataInfo(pasteData, appInfo);
    auto authority = std::make_pair(appInfo.bundleName, appInfo.appIndex);
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authority);
    bool hasSplited = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex,
        appInfo.userId);
    PasteboardWebController::GetInstance().SetWebviewPasteData(pasteData, bundleIndex);
    PasteboardWebController::GetInstance().CheckAppUriPermission(pasteData);
    if (hasSplited || dataSize > static_cast<int64_t>(MessageParcelWarp::GetRawDataSize() * RECALCULATE_DATA_SIZE)) {
        int64_t newDataSize = static_cast<int64_t>(pasteData.Count());
        if (newDataSize > MessageParcelWarp::GetRawDataSize()) {
            setting_.store(false);
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid data size, dataSize=%{public}" PRId64, newDataSize);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
        }
        pasteData.rawDataSize_ = newDataSize;
    }
    setPasteDataUId_ = IPCSkeleton::GetCallingUid();
    RemovePasteData(appInfo);
    clips_.InsertOrAssign(appInfo.userId, std::make_shared<PasteData>(pasteData));
    IncreaseChangeCount(appInfo.userId);
    RadarReportInfo radarReportInfo;
    radarReportInfo.stageRes = static_cast<int32_t>(pasteData.IsDelayData());
    radarReportInfo.bundleName = appInfo.bundleName;
    radarReportInfo.description = pasteData.GetReportDescription();
    radarReportInfo.commonInfo = GetCommonState(dataSize);
    COPY_RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_SET_DELAY_COPY, radarReportInfo);
    HandleDelayDataAndRecord(pasteData, delayGetter, entryGetter, appInfo);
    auto curTime = static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    copyTime_.InsertOrAssign(appInfo.userId, curTime);
    SetDataExpirationTimer(appInfo.userId);
    if (!(pasteData.IsDelayData())) {
        SetDistributedData(appInfo.userId, pasteData);
        NotifyObservers(appInfo.bundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_WRITE);
    }
    SetPasteDataDot(pasteData, appInfo.userId);
    setting_.store(false);
    SubscribeKeyboardEvent();
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::ClearAgedData(int32_t userId)
{
    auto data = clips_.Find(userId);
    if (data.first) {
        clips_.Erase(userId);
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    copyTime_.Erase(userId);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "data is out of the time");
    RADAR_REPORT(DFX_CLEAR_PASTEBOARD, DFX_AUTO_CLEAR, DFX_SUCCESS);
}

void PasteboardService::SetDataExpirationTimer(int32_t userId)
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }

    FFRTTask task = [this, userId]() {
        std::thread thread([=]() {
            ClearAgedData(userId);
        });
        thread.detach();
    };

    std::string taskName = "data_expiration[userId=" + std::to_string(userId) + "]";
    ffrtTimer_->SetTimer(taskName, task, static_cast<uint32_t>(agedTime_.load()));
}

void PasteboardService::SetPasteDataInfo(PasteData &pasteData, const AppInfo &appInfo)
{
    pasteData.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    pasteData.SetOriginAuthority(std::make_pair(appInfo.bundleName, appInfo.appIndex));
    pasteData.SetTime(GetTime());
    pasteData.SetScreenStatus(GetCurrentScreenStatus());
    auto dataId = ++dataId_;
    pasteData.SetDataId(dataId);
    for (auto &record : pasteData.AllRecords()) {
        record->SetDataId(dataId);
    }
    if (pasteData.GetRecordCount() != 0) {
        size_t counts = pasteData.GetRecordCount() - 1;
        std::shared_ptr<PasteDataRecord> records = pasteData.GetRecordAt(counts);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(records != nullptr, PASTEBOARD_MODULE_SERVICE,
            "records[%{public}d] is nullptr.", static_cast<int32_t>(counts));
        std::string text = records->ConvertToText();
        pasteData.SetTextSize(text.size());
    }
}

void PasteboardService::HandleDelayDataAndRecord(PasteData &pasteData, const sptr<IPasteboardDelayGetter> delayGetter,
    const sptr<IPasteboardEntryGetter> entryGetter, const AppInfo &appInfo)
{
    if (pasteData.IsDelayData() && delayGetter != nullptr) {
        sptr<DelayGetterDeathRecipient> deathRecipient = new (std::nothrow)
            DelayGetterDeathRecipient(appInfo.userId, *this);
        delayGetter->AsObject()->AddDeathRecipient(deathRecipient);
        delayGetters_.InsertOrAssign(appInfo.userId, std::make_pair(delayGetter, deathRecipient));
    }
    if (pasteData.IsDelayRecord() && entryGetter != nullptr) {
        sptr<EntryGetterDeathRecipient> deathRecipient = new (std::nothrow)
            EntryGetterDeathRecipient(appInfo.userId, *this);
        entryGetter->AsObject()->AddDeathRecipient(deathRecipient);
        entryGetters_.InsertOrAssign(appInfo.userId, std::make_pair(entryGetter, deathRecipient));
    }
}

bool PasteboardService::IsBasicType(const std::string &mimeType)
{
    if (mimeType == MIMETYPE_TEXT_HTML || mimeType == MIMETYPE_TEXT_PLAIN || mimeType == MIMETYPE_TEXT_URI) {
        return true;
    }
    return false;
}

int32_t PasteboardService::GetMimeTypes(std::vector<std::string> &funcResult)
{
    if (GetCurrentScreenStatus() == ScreenEvent::ScreenUnlocked) {
        auto userId = GetCurrentAccountId();
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            PasteData data;
            int32_t syncTime = 0;
            int32_t ret = GetRemoteData(userId, distEvt, data, syncTime);
            PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK),
                static_cast<int32_t>(PasteboardError::GET_REMOTE_DATA_ERROR),
                PASTEBOARD_MODULE_SERVICE, "get remote data failed, ret=%{public}d", ret);
        }
    }
    funcResult = GetLocalMimeTypes();
    return ERR_OK;
}

int32_t PasteboardService::HasDataType(const std::string &mimeType, bool &funcResult)
{
    auto ret = PasteBoardCommon::IsValidMimeType(mimeType);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR),
        PASTEBOARD_MODULE_SERVICE, "Parameter error. MimeType size=%{public}zu.", mimeType.size());
    funcResult = HasDataType(mimeType);
    return ERR_OK;
}

bool PasteboardService::HasDataType(const std::string &mimeType)
{
    if (GetCurrentScreenStatus() == ScreenEvent::ScreenUnlocked) {
        auto userId = GetCurrentAccountId();
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            auto it = std::find(distEvt.dataType.begin(), distEvt.dataType.end(), mimeType);
            if (it != distEvt.dataType.end()) {
                return true;
            }
            if (IsBasicType(mimeType)) {
                return false;
            }
            PasteData data;
            int32_t syncTime = 0;
            if (GetRemoteData(userId, distEvt, data, syncTime) != static_cast<int32_t>(PasteboardError::E_OK)) {
                return false;
            }
        }
    }
    return HasLocalDataType(mimeType);
}

int32_t PasteboardService::HasUtdType(const std::string &utdType, bool &funcResult)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!utdType.empty(), false, PASTEBOARD_MODULE_SERVICE, "parameter is invalid");
    funcResult = HasUtdType(utdType);
    return ERR_OK;
}

bool PasteboardService::HasUtdType(const std::string &utdType)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    auto userId = appInfo.userId;
    auto screenStatus = GetCurrentScreenStatus();
    PasteData data;
    if (screenStatus == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            int32_t syncTime = 0;
            if (GetRemoteData(userId, distEvt, data, syncTime) != static_cast<int32_t>(PasteboardError::E_OK)) {
                return false;
            }
            return data.HasUtdType(utdType);
        }
    }
    auto it = clips_.Find(userId);
    if (!it.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "can not find data. userId: %{public}d, utdType: %{public}s",
            userId, utdType.c_str());
        return false;
    }
    if (it.second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is nullptr. userId: %{public}d, utdType: %{public}s",
            userId, utdType.c_str());
        return false;
    }
    auto ret = IsDataValid(*(it.second), tokenId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "pasteData is invalid, tokenId is %{public}d, userId: %{public}d,"
            "utdType: %{public}s, ret is %{public}d",
            tokenId, userId, utdType.c_str(), ret);
        return false;
    }
    if (it.second->GetScreenStatus() > screenStatus) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "current screen is %{public}d, set data screen is %{public}d."
            "userId: %{public}d, utdType: %{public}s",
            screenStatus, it.second->GetScreenStatus(), userId, utdType.c_str());
        return false;
    }
    return it.second->HasUtdType(utdType);
}

int32_t PasteboardService::DetectPatterns(const std::vector<Pattern> &patternsToCheck,
    std::vector<Pattern> &funcResult)
{
    bool hasPlain = HasLocalDataType(MIMETYPE_TEXT_PLAIN);
    bool hasHTML = HasLocalDataType(MIMETYPE_TEXT_HTML);
    if (!hasHTML && !hasPlain) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no text");
        std::vector<Pattern>().swap(funcResult);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    int32_t userId = GetCurrentAccountId();
    auto it = clips_.Find(userId);
    if (!it.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "error, no PasteData!");
        std::vector<Pattern>().swap(funcResult);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    std::shared_ptr<PasteData> pasteData = it.second;
    const std::set<Pattern> patterns(patternsToCheck.begin(), patternsToCheck.end());
    std::set<Pattern> result = {};
    result = OHOS::MiscServices::PatternDetection::Detect(patterns, *pasteData, hasHTML, hasPlain);
    funcResult.assign(result.begin(), result.end());
    return ERR_OK;
}

std::pair<int32_t, ClipPlugin::GlobalEvent> PasteboardService::GetValidDistributeEvent(int32_t user)
{
    Event evt;
    auto plugin = GetClipPlugin();
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is null");
        return std::make_pair(static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL), evt);
    }
    auto events = plugin->GetTopEvents(1, user);
    if (events.empty()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "plugin event is empty");
        return std::make_pair(static_cast<int32_t>(PasteboardError::PLUGIN_EVENT_EMPTY), evt);
    }

    evt = events[0];
    if (evt.deviceId == DMAdapter::GetInstance().GetLocalNetworkId() || evt.expiration < currentEvent_.expiration) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get local data");
        return std::make_pair(static_cast<int32_t>(PasteboardError::GET_LOCAL_DATA), evt);
    }
    if (evt.account != AccountManager::GetInstance().GetCurrentAccount()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "account error");
        return std::make_pair(static_cast<int32_t>(PasteboardError::INVALID_EVENT_ACCOUNT), evt);
    }
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    int32_t ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(evt.deviceId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "deviceId: %{public}.6s is offline", evt.deviceId.c_str());
        return std::make_pair(ret, evt);
    }

    if (evt.deviceId == currentEvent_.deviceId && evt.seqId == currentEvent_.seqId &&
        evt.expiration == currentEvent_.expiration) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get same remote data");
        return std::make_pair(static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA), evt);
    }

    uint64_t curTime =
        static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
    ret = evt.status == ClipPlugin::EVT_NORMAL ? ret : static_cast<int32_t>(PasteboardError::INVALID_EVENT_STATUS);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((curTime != 0 && evt.expiration != EXPIRATION_INTERVAL),
        std::make_pair(static_cast<int32_t>(PasteboardError::GET_BOOTTIME_FAILED), evt),
        PASTEBOARD_MODULE_SERVICE, "Failed to get the time."
        "expiration = %{public}" PRIu64 ", curTime = %{public}" PRIu64, evt.expiration, curTime);
    ret = curTime < evt.expiration ? ret : static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR);
    return std::make_pair(ret, evt);
#else
    return std::make_pair(static_cast<int32_t>(PasteboardError::NOT_SUPPORT), evt);
#endif
}

std::vector<std::string> PasteboardService::GetLocalMimeTypes()
{
    auto userId = GetCurrentAccountId();
    auto it = clips_.Find(userId);
    if (!it.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "can not find data. userId: %{public}d", userId);
        return {};
    }
    if (it.second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is nullptr. userId: %{public}d", userId);
        return {};
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto ret = IsDataValid(*(it.second), tokenId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "pasteData is invalid, tokenId is %{public}d, userId: %{public}d, ret is %{public}d",
            tokenId, userId, ret);
        return {};
    }
    return it.second->GetMimeTypes();
}

bool PasteboardService::HasLocalDataType(const std::string &mimeType)
{
    auto userId = GetCurrentAccountId();
    auto it = clips_.Find(userId);
    if (!it.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "can not find data. userId: %{public}d, mimeType: %{public}s",
            userId, mimeType.c_str());
        return false;
    }
    if (it.second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is nullptr. userId: %{public}d, mimeType: %{public}s",
            userId, mimeType.c_str());
        return false;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto ret = IsDataValid(*(it.second), tokenId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "pasteData is invalid, tokenId is %{public}d, userId: %{public}d,"
            "mimeType: %{public}s, ret is %{public}d",
            tokenId, userId, mimeType.c_str(), ret);
        return false;
    }
    auto screenStatus = GetCurrentScreenStatus();
    if (it.second->GetScreenStatus() > screenStatus) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "current screen is %{public}d, set data screen is %{public}d."
            "userId: %{public}d, mimeType: %{public}s",
            screenStatus, it.second->GetScreenStatus(), userId, mimeType.c_str());
        return false;
    }
    std::vector<std::string> mimeTypes = it.second->GetMimeTypes();
    auto isExistType = std::find(mimeTypes.begin(), mimeTypes.end(), mimeType) != mimeTypes.end();
    return isExistType;
}

int32_t PasteboardService::IsRemoteData(bool &funcResult)
{
    funcResult = IsRemoteData();
    return ERR_OK;
}

bool PasteboardService::IsRemoteData()
{
    auto userId = GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId is error.");
        return false;
    }
    auto it = clips_.Find(userId);
    if (!it.first) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        return distRet == static_cast<int32_t>(PasteboardError::E_OK);
    }
    return it.second->IsRemote();
}

int32_t PasteboardService::GetDataSource(std::string &bundleName)
{
    auto userId = GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    auto it = clips_.Find(userId);
    if (!it.first) {
        return static_cast<int32_t>(PasteboardError::NO_USER_DATA_ERROR);
    }
    auto data = it.second;
    if (data->IsRemote()) {
        return static_cast<int32_t>(PasteboardError::REMOTE_EXCEPTION);
    }
    auto tokenId = data->GetTokenId();
    bundleName = GetAppLabel(tokenId);
    if (bundleName.empty() || bundleName.length() > MAX_BUNDLE_NAME_LENGTH) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Failed to get bundleName");
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    }
    return ERR_OK;
}

void PasteboardService::CloseSharedMemFd(int fd)
{
    if (fd >= 0) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Close fd:%{public}d", fd);
        close(fd);
    }
}

int32_t PasteboardService::WritePasteData(
    int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer, PasteData &pasteData, bool &hasData)
{
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        auto actualSize = AshmemGetSize(fd);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(actualSize >= 0 && rawDataSize <= actualSize,
            static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
            "rawDataSize invalid, actualSize=%{public}d, rawDataSize:%{public}" PRId64, actualSize, rawDataSize);
        void *ptr = ::mmap(nullptr, rawDataSize, PROT_READ, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "mmap failed, size:%{public}" PRId64, rawDataSize);
            CloseSharedMemFd(fd);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
        }
        const uint8_t *rawData = reinterpret_cast<const uint8_t *>(ptr);
        if (rawData == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "rawData is nullptr, size:%{public}" PRId64, rawDataSize);
            ::munmap(ptr, rawDataSize);
            CloseSharedMemFd(fd);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
        }
        std::vector<uint8_t> pasteDataTlv(rawData, rawData + rawDataSize);
        hasData = pasteData.Decode(pasteDataTlv);
        ::munmap(ptr, rawDataSize);
    } else {
        hasData = pasteData.Decode(buffer);
    }
    CloseSharedMemFd(fd);
    pasteData.rawDataSize_ = rawDataSize;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set local data, dataSize=%{public}" PRId64, rawDataSize);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::SubscribeDisposableObserver(const sptr<IPasteboardDisposableObserver> &observer,
    int32_t targetWindowId, DisposableType type, uint32_t maxLength)
{
    constexpr pid_t SELECTION_SERVICE_UID = 1080;
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uid == SELECTION_SERVICE_UID,
        static_cast<int32_t>(PasteboardError::NOT_SUPPORT), PASTEBOARD_MODULE_SERVICE, "not support");

    pid_t pid = IPCSkeleton::GetCallingPid();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    DisposableInfo info(pid, tokenId, targetWindowId, type, maxLength, observer);
    int32_t ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "add observer info failed, ret=%{public}d", ret);
    return ERR_OK;
}

int32_t PasteboardService::SetPasteData(int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer,
    const sptr<IPasteboardDelayGetter> &delayGetter, const sptr<IPasteboardEntryGetter> &entryGetter)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        fd >= 0, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE, "fd invalid");
    MessageParcelWarp messageData;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "fd=%{public}d, agedTime_ = %{public}d,"
        "rawDataSize=%{public}" PRId64, fd, agedTime_.load(), rawDataSize);
    SetCriticalTimer();
    if (rawDataSize <= 0 || rawDataSize > messageData.GetRawDataSize()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Invalid raw data size:%{public}" PRId64, rawDataSize);
        CloseSharedMemFd(fd);
        return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
    }
    PasteData pasteData{};
    bool result = false;
    auto ret = WritePasteData(fd, rawDataSize, buffer, pasteData, result);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "Failed to write paste data");
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "Failed to decode paste data in TLV");

    auto tokenId = GetDataTokenId(pasteData);
    pasteData.SetTokenId(tokenId);
    UpdateShareOption(pasteData);
    if (DisposableManager::GetInstance().TryProcessDisposableData(pasteData, delayGetter, entryGetter)) {
        return ERR_OK;
    }
    ret = SaveData(pasteData, rawDataSize, delayGetter, entryGetter);
    if (entityObserverMap_.Size() != 0 && pasteData.HasMimeType(MIMETYPE_TEXT_PLAIN)) {
        RecognizePasteData(pasteData);
    }
    ReportUeCopyEvent(pasteData, rawDataSize, ret);
    HiViewAdapter::ReportUseBehaviour(pasteData, HiViewAdapter::COPY_STATE, ret);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "Failed to save data, ret=%{public}d", ret);
    return ERR_OK;
}

int32_t PasteboardService::SetPasteDataDelayData(int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer,
    const sptr<IPasteboardDelayGetter> &delayGetter)
{
    return SetPasteData(fd, rawDataSize, buffer, delayGetter, nullptr);
}

int32_t PasteboardService::SetPasteDataEntryData(int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer,
    const sptr<IPasteboardEntryGetter> &entryGetter)
{
    return SetPasteData(fd, rawDataSize, buffer, nullptr, entryGetter);
}

int32_t PasteboardService::SetPasteDataOnly(int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer)
{
    return SetPasteData(fd, rawDataSize, buffer, nullptr, nullptr);
}

void PasteboardService::RemovePasteData(const AppInfo &appInfo)
{
    delayGetters_.ComputeIfPresent(appInfo.userId, [](auto, auto &delayGetter) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_SET_DELAY_COPY, DFX_SUCCESS, COVER_DELAY_DATA, DFX_SUCCESS);
        if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
            delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
        }
        return false;
    });
    entryGetters_.ComputeIfPresent(appInfo.userId, [](auto, auto &entryGetter) {
        if (entryGetter.first != nullptr && entryGetter.second != nullptr) {
            entryGetter.first->AsObject()->RemoveDeathRecipient(entryGetter.second);
        }
        return false;
    });
}

int32_t PasteboardService::GetCurrentAccountId()
{
    if (currentUserId_ != ERROR_USERID) {
        return currentUserId_;
    }
    std::vector<int32_t> accountIds;
    auto ret = AccountSA::OsAccountManager::QueryActiveOsAccountIds(accountIds);
    if (ret != ERR_OK || accountIds.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query active user failed errCode=%{public}d", ret);
        return ERROR_USERID;
    }
    currentUserId_ = accountIds.front();
    return currentUserId_;
}

ScreenEvent PasteboardService::GetCurrentScreenStatus()
{
    if (currentScreenStatus != ScreenEvent::Default) {
        return currentScreenStatus;
    }

    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query current screen status failed.");
    return ScreenEvent::Default;
}

bool PasteboardService::IsCopyable(uint32_t tokenId) const
{
#ifdef WITH_DLP
    bool copyable = false;
    auto ret = Security::DlpPermission::DlpPermissionKit::QueryDlpFileCopyableByTokenId(copyable, tokenId);
    if (ret != Security::DlpPermission::DLP_OK || !copyable) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "tokenId = 0x%{public}x ret = %{public}d, copyable = %{public}d.",
            tokenId, ret, copyable);
        return false;
    }
#endif
    return true;
}

void PasteboardService::SetInputMethodPid(int32_t userId, pid_t callPid)
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return;
    }
    auto isImePid = imc->IsCurrentImeByPid(callPid);
    if (isImePid) {
        imeMap_.InsertOrAssign(userId, callPid);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set inputMethod userId = %{public}d, pid = %{public}d",
            userId, callPid);
    }
}

void PasteboardService::ClearInputMethodPidByPid(int32_t userId, pid_t callPid)
{
    auto [hasPid, pid] = imeMap_.Find(userId);
    if (hasPid && callPid == pid) {
        imeMap_.Erase(userId);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "clear inputMethod userId = %{public}d, pid = %{public}d",
            userId, callPid);
    }
}

void PasteboardService::ClearInputMethodPid()
{
    imeMap_.Clear();
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clear inputMethod pid!");
}

int32_t PasteboardService::SubscribeObserver(PasteboardObserverType type,
    const sptr<IPasteboardChangedObserver> &observer)
{
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : appInfo.userId;
    SetInputMethodPid(userId, callPid);
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    bool addSucc = false;
    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_LOCAL)) {
        addSucc = AddObserver(userId, observer, observerLocalChangedMap_) || addSucc;
    }

    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_REMOTE)) {
        addSucc = AddObserver(userId, observer, observerRemoteChangedMap_) || addSucc;
    }

    if (isEventType && IsCallerUidValid()) {
        addSucc = AddObserver(userId, observer, observerEventMap_) || addSucc;
    }
    return addSucc ? ERR_OK : static_cast<int32_t>(PasteboardError::ADD_OBSERVER_FAILED);
}

int32_t PasteboardService::ResubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer)
{
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    if (appInfo.tokenType == ATokenTypeEnum::TOKEN_HAP) {
        return SubscribeObserver(type, observer);
    }
    return ERR_OK;
}

int32_t PasteboardService::UnsubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer)
{
    auto callPid = IPCSkeleton::GetCallingPid();
    auto appInfo = GetAppInfo(IPCSkeleton::GetCallingTokenID());
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : appInfo.userId;
    ClearInputMethodPidByPid(userId, callPid);
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_LOCAL)) {
        RemoveSingleObserver(userId, observer, observerLocalChangedMap_);
    }

    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_REMOTE)) {
        RemoveSingleObserver(userId, observer, observerRemoteChangedMap_);
    }

    if (isEventType && IsCallerUidValid()) {
        RemoveSingleObserver(userId, observer, observerEventMap_);
    }
    return ERR_OK;
}

int32_t PasteboardService::UnsubscribeAllObserver(PasteboardObserverType type)
{
    ClearInputMethodPid();
    bool isEventType = static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_EVENT);
    int32_t userId = isEventType ? COMMON_USERID : GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_LOCAL)) {
        RemoveAllObserver(userId, observerLocalChangedMap_);
    }

    if (static_cast<uint32_t>(type) & static_cast<uint32_t>(PasteboardObserverType::OBSERVER_REMOTE)) {
        RemoveAllObserver(userId, observerRemoteChangedMap_);
    }

    if (isEventType && IsCallerUidValid()) {
        RemoveAllObserver(userId, observerEventMap_);
    }
    return ERR_OK;
}

uint32_t PasteboardService::GetAllObserversSize(int32_t userId, pid_t pid)
{
    auto localObserverSize = GetObserversSize(userId, pid, observerLocalChangedMap_);
    auto remoteObserverSize = GetObserversSize(userId, pid, observerRemoteChangedMap_);
    auto eventObserverSize = GetObserversSize(COMMON_USERID, pid, observerEventMap_);
    return localObserverSize + remoteObserverSize + eventObserverSize;
}

uint32_t PasteboardService::GetObserversSize(int32_t userId, pid_t pid, ObserverMap &observerMap)
{
    auto countKey = std::make_pair(userId, pid);
    auto it = observerMap.find(countKey);
    if (it != observerMap.end()) {
        return it->second->size();
    }
    return 0;
}

bool PasteboardService::AddObserver(
    int32_t userId, const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap)
{
    if (observer == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer null.");
        return false;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callPid = IPCSkeleton::GetCallingPid();
    auto callObserverKey = std::make_pair(userId, callPid);
    auto it = observerMap.find(callObserverKey);
    std::shared_ptr<std::set<sptr<IPasteboardChangedObserver>, classcomp>> observers;
    if (it != observerMap.end()) {
        observers = it->second;
    } else {
        observers = std::make_shared<std::set<sptr<IPasteboardChangedObserver>, classcomp>>();
        observerMap.insert(std::make_pair(callObserverKey, observers));
    }
    auto allObserverCount = GetAllObserversSize(userId, callPid);
    if (allObserverCount >= MAX_OBSERVER_COUNT) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer count over limit. callPid:%{public}d", callPid);
        return false;
    }
    observers->insert(observer);
    RADAR_REPORT(DFX_OBSERVER, DFX_ADD_OBSERVER, DFX_SUCCESS);
    PASTEBOARD_HILOGI(
        PASTEBOARD_MODULE_SERVICE, "observers->size = %{public}u.", static_cast<unsigned int>(observers->size()));
    return true;
}

void PasteboardService::RemoveSingleObserver(
    int32_t userId, const sptr<IPasteboardChangedObserver> &observer, ObserverMap &observerMap)
{
    if (observer == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "observer null.");
        return;
    }
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callPid = IPCSkeleton::GetCallingPid();
    auto callObserverKey = std::make_pair(userId, callPid);
    auto it = observerMap.find(callObserverKey);
    if (it == observerMap.end()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id not found userId is %{public}d", userId);
        return;
    }
    auto observers = it->second;
    PASTEBOARD_HILOGD(
        PASTEBOARD_MODULE_SERVICE, "observers size: %{public}u.", static_cast<unsigned int>(observers->size()));
    auto eraseNum = observers->erase(observer);
    RADAR_REPORT(DFX_OBSERVER, DFX_REMOVE_SINGLE_OBSERVER, DFX_SUCCESS);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "observers size = %{public}u, eraseNum = %{public}zu",
        static_cast<unsigned int>(observers->size()), eraseNum);
}

void PasteboardService::RemoveAllObserver(int32_t userId, ObserverMap &observerMap)
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    for (auto it = observerMap.begin(); it != observerMap.end();) {
        if (it->first.first == userId) {
            it = observerMap.erase(it);
        } else {
            ++it;
        }
    }
    RADAR_REPORT(DFX_OBSERVER, DFX_REMOVE_ALL_OBSERVER, DFX_SUCCESS);
}

int32_t PasteboardService::SetGlobalShareOption(const std::unordered_map<uint32_t, int32_t> &globalShareOptions)
{
    if (!IsCallerUidValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No Permission");
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    std::map<uint32_t, ShareOption> shareOptions;
    for (const auto& pair : globalShareOptions) {
        uint32_t key = pair.first;
        int32_t value = pair.second;
        if (value >= InApp && value <= CrossDevice) {
            shareOptions[key] = static_cast<ShareOption>(value);
        }
    }
    for (const auto &[tokenId, shareOption] : shareOptions) {
        GlobalShareOption option = {.source = MDM, .shareOption = shareOption};
        globalShareOptions_.InsertOrAssign(tokenId, option);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Set %{public}zu global shareOption.", globalShareOptions.size());
    return ERR_OK;
}

int32_t PasteboardService::RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds)
{
    if (!IsCallerUidValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No Permission");
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    int32_t count = 0;
    for (const uint32_t &tokenId : tokenIds) {
        globalShareOptions_.ComputeIfPresent(tokenId, [&count](const uint32_t &key, GlobalShareOption &value) {
            count++;
            return false;
        });
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Remove %{public}d global shareOption.", count);
    return ERR_OK;
}

int32_t PasteboardService::GetGlobalShareOption(const std::vector<uint32_t> &tokenIds,
    std::unordered_map<uint32_t, int32_t>& funcResult)
{
    if (!IsCallerUidValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No Permission");
        funcResult = {};
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    std::map<uint32_t, ShareOption> result;
    if (tokenIds.empty()) {
        globalShareOptions_.ForEach([&result](const uint32_t &key, GlobalShareOption &value) {
            result[key] = value.shareOption;
            return false;
        });
        for (const auto &pair : result) {
            funcResult[pair.first] = static_cast<int32_t>(pair.second);
        }
        return ERR_OK;
    }
    for (const uint32_t &tokenId : tokenIds) {
        globalShareOptions_.ComputeIfPresent(tokenId, [&result](const uint32_t &key, GlobalShareOption &value) {
            result[key] = value.shareOption;
            return true;
        });
    }
    for (const auto &pair : result) {
        funcResult[pair.first] = static_cast<int32_t>(pair.second);
    }
    return ERR_OK;
}

bool PasteboardService::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "called token id: %{public}" PRIu64, tokenId);
    return (tokenId & SYSTEM_APP_MASK) == SYSTEM_APP_MASK;
}

int32_t PasteboardService::SetAppShareOptions(int32_t shareOptions)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(PasteData::IsValidShareOption(shareOptions),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "shareOptions invalid, shareOptions=%{public}d", shareOptions);
    auto fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (!IsSystemAppByFullTokenID(fullTokenId)) {
        if (shareOptions != ShareOption::InApp) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "param is invalid");
            return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
        }
        auto isManageGrant = PermissionUtils::IsPermissionGranted(MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION,
            tokenId);
        if (!isManageGrant) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No permission, token id: 0x%{public}x.", tokenId);
            return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
        }
    }
    GlobalShareOption option = {.source = APP, .shareOption = static_cast<ShareOption>(shareOptions)};
    auto isAbsent = globalShareOptions_.ComputeIfAbsent(tokenId, [&option](const uint32_t &tokenId) {
        return option;
    });
    if (!isAbsent) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Settings already exist, token id: 0x%{public}x.", tokenId);
        return static_cast<int32_t>(PasteboardError::INVALID_OPERATION_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Set token id: 0x%{public}x share options: %{public}d success.",
        tokenId, shareOptions);
    return 0;
}

int32_t PasteboardService::RemoveAppShareOptions()
{
    auto fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (!IsSystemAppByFullTokenID(fullTokenId)) {
        auto isManageGrant = PermissionUtils::IsPermissionGranted(MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION,
            tokenId);
        if (!isManageGrant) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No permission, token id: 0x%{public}x.", tokenId);
            return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
        }
    }
    std::map<uint32_t, GlobalShareOption> result;
    globalShareOptions_.ComputeIfPresent(tokenId, [&result](const uint32_t &key, GlobalShareOption &value) {
        result[key] = value;
        return true;
    });
    if (!result.empty()) {
        if (result[tokenId].source == APP) {
            globalShareOptions_.Erase(tokenId);
            PASTEBOARD_HILOGI(
                PASTEBOARD_MODULE_SERVICE, "Remove token id: 0x%{public}x share options success.", tokenId);
            return 0;
        } else {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Can not remove token id: 0x%{public}x.", tokenId);
            return 0;
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "This token id: 0x%{public}x not set.", tokenId);
    return 0;
}

void PasteboardService::UpdateShareOption(PasteData &pasteData)
{
    globalShareOptions_.ComputeIfPresent(
        pasteData.GetTokenId(), [&pasteData](const uint32_t &tokenId, GlobalShareOption &option) {
            pasteData.SetShareOption(option.shareOption);
            return true;
        });
}

bool PasteboardService::CheckMdmShareOption(PasteData &pasteData)
{
    bool result = false;
    globalShareOptions_.ComputeIfPresent(
        pasteData.GetTokenId(), [&result](const uint32_t &tokenId, GlobalShareOption &option) {
            if (option.source == MDM) {
                result = true;
            }
            return true;
        });
    return result;
}

bool PasteboardService::IsCallerUidValid()
{
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid == EDM_UID || (uid_ != -1 && callingUid == uid_)) {
        return true;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "callingUid error: %{public}d.", callingUid);
    return false;
}

void PasteboardService::ThawInputMethod(pid_t imePid)
{
    auto type = ResourceSchedule::ResType::RES_TYPE_SA_CONTROL_APP_EVENT;
    auto status = ResourceSchedule::ResType::SaControlAppStatus::SA_START_APP;

    std::unordered_map<std::string, std::string> payload = {
        { "saId", std::to_string(PASTEBOARD_SERVICE_ID) },
        { "saName", PASTEBOARD_SERVICE_SA_NAME },
        { "extensionType", std::to_string(static_cast<int32_t>(AppExecFwk::ExtensionAbilityType::INPUTMETHOD)) },
        { "pid", std::to_string(imePid) },
        { "isDelay", std::to_string(true) } };
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "report RSS need thaw:pid = %{public}d", imePid);
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, status, payload);
}

bool PasteboardService::IsNeedThaw()
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return false;
    }

    std::shared_ptr<Property> property;
    int32_t ret = imc->GetDefaultInputMethod(property);
    if (ret != ErrorCode::NO_ERROR || property == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "default input method is nullptr!");
        return false;
    }
    return true;
}

void PasteboardService::NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    auto [hasPid, pid] = imeMap_.Find(userId);
    if (hasPid && IsNeedThaw()) {
        ThawInputMethod(pid);
    }
    std::thread thread([this, bundleName, userId, status]() {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto &observers : observerLocalChangedMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerLocalChangedMap_.second is nullptr");
                continue;
            }
            for (const auto &observer : *(observers.second)) {
                if (status != PasteboardEventStatus::PASTEBOARD_READ && userId == observers.first.first) {
                    observer->OnPasteboardChanged();
                }
            }
        }
        IPasteboardChangedObserver::PasteboardChangedEvent event;
        event.status = static_cast<int32_t>(status);
        event.userId = userId;
        event.bundleName = bundleName;
        for (auto &observers : observerEventMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerEventMap_.second is nullptr");
                continue;
            }
            for (const auto &observer : *(observers.second)) {
                observer->OnPasteboardEvent(event);
            }
        }
    });
    thread.detach();
}

bool PasteboardService::SetPasteboardHistory(HistoryInfo &info)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(info.userId != ERROR_USERID, false,
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::string history = std::move(info.time) + " " + std::move(info.bundleName) + " " + std::move(info.state) + " " +
                          " " + std::move(info.remote) + " userId:" + std::to_string(info.userId);
    constexpr const size_t DATA_HISTORY_SIZE = 10;
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    if (dataHistory_.size() == DATA_HISTORY_SIZE) {
        dataHistory_.erase(dataHistory_.begin());
    }
    dataHistory_.push_back(std::move(history));
    return true;
}

int PasteboardService::Dump(int fd, const std::vector<std::u16string> &args)
{
    int uid = static_cast<int>(IPCSkeleton::GetCallingUid());
    const int maxUid = 10000;
    if (uid > maxUid) {
        return 0;
    }

    std::vector<std::string> argsStr;
    for (auto item : args) {
        argsStr.emplace_back(Str16ToStr8(item));
    }

    if (PasteboardDumpHelper::GetInstance().Dump(fd, argsStr)) {
        return 0;
    }
    return 0;
}

std::string PasteboardService::GetTime()
{
    constexpr int USEC_TO_MSEC = 1000;
    time_t timeSeconds = time(0);
    if (timeSeconds == -1) {
        return FAIL_TO_GET_TIME_STAMP;
    }
    struct tm nowTime;
    localtime_r(&timeSeconds, &nowTime);

    struct timeval timeVal = { 0, 0 };
    gettimeofday(&timeVal, nullptr);

    std::string targetTime = std::to_string(nowTime.tm_year + 1900) + "-" + std::to_string(nowTime.tm_mon + 1) + "-" +
                             std::to_string(nowTime.tm_mday) + " " + std::to_string(nowTime.tm_hour) + ":" +
                             std::to_string(nowTime.tm_min) + ":" + std::to_string(nowTime.tm_sec) + "." +
                             std::to_string(timeVal.tv_usec / USEC_TO_MSEC);
    return targetTime;
}

std::string PasteboardService::DumpHistory() const
{
    std::string result;
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    auto userId = GetCurrentAccountId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID, "Access history fail! invalid userId.",
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    if (!dataHistory_.empty()) {
        result.append("Access history last ten times: ").append("\n");
        for (auto iter = dataHistory_.rbegin(); iter != dataHistory_.rend(); ++iter) {
            std::string userIdPrefix = " userId:" + std::to_string(userId);
            size_t userIdPos = (*iter).find(userIdPrefix);
            if (userIdPos != std::string::npos) {
                std::string historyWithoutUserId = (*iter).substr(0, userIdPos);
                result.append("          ").append(historyWithoutUserId).append("\n");
            }
        }
    } else {
        result.append("Access history fail! dataHistory_ no data.").append("\n");
    }
    return result;
}

std::string PasteboardService::DumpData()
{
    auto userId = GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query active user failed.");
        return "";
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "id = %{public}d", userId);
    auto it = clips_.Find(userId);
    std::string result;
    if (it.first && it.second != nullptr) {
        size_t recordCounts = it.second->GetRecordCount();
        auto property = it.second->GetProperty();
        std::string shareOption;
        PasteData::ShareOptionToString(property.shareOption, shareOption);
        std::string sourceDevice;
        if (property.isRemote) {
            sourceDevice = "remote";
        } else {
            sourceDevice = "local";
        }
        result.append("|Owner       :  ")
            .append(property.bundleName)
            .append("\n")
            .append("|Timestamp   :  ")
            .append(property.setTime)
            .append("\n")
            .append("|Share Option:  ")
            .append(shareOption)
            .append("\n")
            .append("|Record Count:  ")
            .append(std::to_string(recordCounts))
            .append("\n")
            .append("|Mime types  :  {");
        if (!property.mimeTypes.empty()) {
            for (size_t i = 0; i < property.mimeTypes.size(); ++i) {
                result.append(property.mimeTypes[i]).append(",");
            }
        }
        result.append("}").append("\n").append("|source device:  ").append(sourceDevice);
    } else {
        result.append("No copy data.").append("\n");
    }
    return result;
}

bool PasteboardService::IsFocusedApp(uint32_t tokenId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "caller is not application");
        return true;
    }
    FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance().GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance().GetFocusWindowInfo(info);
#endif
    auto callPid = IPCSkeleton::GetCallingPid();
    if (callPid == info.pid_) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pid is same, it is focused app");
        return true;
    }
    bool isFocused = false;
    auto ret = AAFwk::AbilityManagerClient::GetInstance()->CheckUIExtensionIsFocused(tokenId, isFocused);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "check result:%{public}d, isFocused:%{public}d", ret, isFocused);
    return ret == NO_ERROR && isFocused;
}

void PasteboardService::DeletePreSyncP2pFromP2pMap(const std::string &networkId)
{
    std::string taskName = P2P_PRESYNC_ID + networkId;
    if (ffrtTimer_) {
        ffrtTimer_->CancelTimer(taskName);
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(networkId, [this](const auto &key, auto &value) {
        value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
            return false;
        });
        return true;
    });
    DeletePreSyncP2pMap(networkId);
}

void PasteboardService::DeletePreSyncP2pMap(const std::string &networkId)
{
    auto p2pIter = preSyncP2pMap_.find(networkId);
    if (p2pIter != preSyncP2pMap_.end()) {
        if (p2pIter->second) {
            p2pIter->second->SetValue(SET_VALUE_SUCCESS);
        }
        preSyncP2pMap_.erase(networkId);
    }
}

void PasteboardService::AddPreSyncP2pTimeoutTask(const std::string &networkId)
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    std::string taskName = P2P_PRESYNC_ID + networkId;
    ffrtTimer_->CancelTimer(taskName);
    FFRTTask p2pTask = [this, networkId] {
        std::thread thread([=]() {
            PasteComplete(networkId, P2P_PRESYNC_ID);
            std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
            DeletePreSyncP2pMap(networkId);
        });
        thread.detach();
    };
    ffrtTimer_->SetTimer(taskName, p2pTask, PRE_ESTABLISH_P2P_LINK_TIME);
}

void PasteboardService::RegisterPreSyncCallback(std::shared_ptr<ClipPlugin> clipPlugin)
{
    if (!clipPlugin) {
        return;
    }
    clipPlugin->RegisterPreSyncCallback(std::bind(&PasteboardService::PreEstablishP2PLinkCallback,
        this, std::placeholders::_1, std::placeholders::_2));
    clipPlugin->RegisterPreSyncMonitorCallback(std::bind(&PasteboardService::PreSyncSwitchMonitorCallback, this));
}

bool PasteboardService::OpenP2PLinkForPreEstablish(const std::string &networkId, ClipPlugin *clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        DeletePreSyncP2pFromP2pMap(networkId);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist, ret:%{public}d", ret);
        return false;
    }
    auto status = DistributedFileDaemonManager::GetInstance().OpenP2PConnection(remoteDevice);
    if (status != RESULT_OK) {
        DeletePreSyncP2pFromP2pMap(networkId);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status:%{public}d", status);
        return false;
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [](const auto &key, auto &value) {
        value.Compute(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
            value.isSuccess = true;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "preP2pLink isSuccess:%{public}d", value.isSuccess);
            return true;
        });
        return true;
    });
    if (clipPlugin) {
        status = clipPlugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state connect_succ error, status:%{public}d", status);
        }
    }
    AddPreSyncP2pTimeoutTask(networkId);
    return true;
#else
    return false;
#endif
}

void PasteboardService::PreEstablishP2PLink(const std::string &networkId, ClipPlugin *clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink enter");
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = nullptr;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        if (p2pEstablishInfo_.pasteBlock && p2pEstablishInfo_.networkId == networkId) {
            return;
        }
        auto p2pNetwork = p2pMap_.Find(networkId);
        bool isP2pSuccess = p2pNetwork.first && p2pNetwork.second.Find(P2P_PRESYNC_ID).first &&
            p2pNetwork.second.Find(P2P_PRESYNC_ID).second.isSuccess == true;
        if (isP2pSuccess) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Pre P2pEstablish exist");
            AddPreSyncP2pTimeoutTask(networkId);
            return;
        }
        pasteBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
        if (!pasteBlock) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc BlockObject");
            return;
        }
        p2pMap_.Compute(networkId, [this](const auto &key, auto &value) {
            value.Compute(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
                value.callPid = 0;
                value.isSuccess = false;
                return true;
            });
            return true;
        });
        preSyncP2pMap_.emplace(networkId, pasteBlock);
    }
    if (OpenP2PLinkForPreEstablish(networkId, clipPlugin)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink Finish");
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink failed");
    }
    pasteBlock->SetValue(SET_VALUE_SUCCESS);
#endif
}

void PasteboardService::PreEstablishP2PLinkCallback(const std::string &networkId, ClipPlugin *clipPlugin)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLinkCallback enter");
    if (networkId.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLinkCallback failed, networkId is null");
        return;
    }
    if (!clipPlugin) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        return;
    }
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
#ifdef PB_DEVICE_MANAGER_ENABLE
    FFRTTask p2pTask = [this, networkId, clipPlugin] {
        std::thread thread([=]() {
            PreEstablishP2PLink(networkId, clipPlugin);
        });
        thread.detach();
    };
    std::string taskName = "PreEstablishP2PLink_";
    taskName += networkId;
    ffrtTimer_->SetTimer(taskName, p2pTask);
#endif
}

void PasteboardService::PreSyncRemotePasteboardData()
{
    auto clipPlugin = GetClipPlugin();
    if (!clipPlugin) {
        return;
    }
    if (!clipPlugin->NeedSyncTopEvent()) {
        return;
    }
    const int32_t DEFAULT_USER_ID = 0;
    clipPlugin->SendPreSyncEvent(DEFAULT_USER_ID);
}

void PasteboardService::PreSyncSwitchMonitorCallback()
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    FFRTTask monitorTask = [this] {
        std::thread thread([=]() {
            RegisterPreSyncMonitor();
        });
        thread.detach();
    };
    ffrtTimer_->SetTimer(REGISTER_PRESYNC_MONITOR, monitorTask);
}

void PasteboardService::RegisterPreSyncMonitor()
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    if (!MMI::InputManager::GetInstance()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "MMI::InputManager is null");
        return;
    }
    FFRTTask monitorTask = [this] {
        std::thread thread([=]() {
            UnRegisterPreSyncMonitor();
        });
        thread.detach();
    };
    if (subscribeActiveId_ != INVALID_SUBSCRIBE_ID) {
        ffrtTimer_->SetTimer(UNREGISTER_PRESYNC_MONITOR, monitorTask, PRESYNC_MONITOR_TIME);
        return;
    }
    std::shared_ptr<InputEventCallback> preSyncMonitor =
        std::make_shared<InputEventCallback>(InputEventCallback::INPUTTYPE_PRESYNC, this);
    if (!preSyncMonitor) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc InputEventCallback");
        return;
    }
    subscribeActiveId_ = MMI::InputManager::GetInstance()->SubscribeInputActive(
        std::static_pointer_cast<MMI::IInputEventConsumer>(preSyncMonitor), PRESYNC_MONITOR_INTERVAL_MILLISECONDS);
    if (subscribeActiveId_ < 0) {
        subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SubscribeInputActive failed");
        return;
    }
    ffrtTimer_->SetTimer(UNREGISTER_PRESYNC_MONITOR, monitorTask, PRESYNC_MONITOR_TIME);
}

void PasteboardService::UnRegisterPreSyncMonitor()
{
    if (!MMI::InputManager::GetInstance()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "MMI::InputManager is null");
        return;
    }
    if (subscribeActiveId_ != INVALID_SUBSCRIBE_ID) {
        MMI::InputManager::GetInstance()->UnsubscribeInputActive(subscribeActiveId_);
        subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
    }
}

FocusedAppInfo PasteboardService::GetFocusedAppInfo(void) const
{
    FocusedAppInfo appInfo = { 0 };
    FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance().GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance().GetFocusWindowInfo(info);
#endif
    appInfo.windowId = info.windowId_;
    appInfo.abilityToken = info.abilityToken_;
    return appInfo;
}

void PasteboardService::SetPasteDataDot(PasteData &pasteData, const int32_t &userId)
{
    auto bundleName = pasteData.GetBundleName();
    HistoryInfo info{ pasteData.GetTime(), bundleName, "set", "", userId };
    SetPasteboardHistory(info);

    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData Report!");
    Reporter::GetInstance().PasteboardBehaviour().Report(
        { static_cast<int>(BehaviourPasteboardState::BPS_COPY_STATE), bundleName });

    int state = static_cast<int>(StatisticPasteboardState::SPS_COPY_STATE);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData GetTextSize!");
    size_t dataSize = pasteData.GetTextSize();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData timeC!");
    CalculateTimeConsuming timeC(dataSize, state);
}

void PasteboardService::GetPasteDataDot(PasteData &pasteData, const std::string &bundleName, const int32_t &userId)
{
    std::string remote;
    if (pasteData.IsRemote()) {
        remote = "remote";
    }
    std::string time = GetTime();
    HistoryInfo info{ time, bundleName, "get", remote, userId };
    SetPasteboardHistory(info);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetPasteData Report!");
    int pState = StatisticPasteboardState::SPS_INVALID_STATE;
    int bState = BehaviourPasteboardState::BPS_INVALID_STATE;
    if (pasteData.IsRemote()) {
        pState = static_cast<int>(StatisticPasteboardState::SPS_REMOTE_PASTE_STATE);
        bState = static_cast<int>(BehaviourPasteboardState::BPS_REMOTE_PASTE_STATE);
    } else {
        pState = static_cast<int>(StatisticPasteboardState::SPS_PASTE_STATE);
        bState = static_cast<int>(BehaviourPasteboardState::BPS_PASTE_STATE);
    };

    Reporter::GetInstance().PasteboardBehaviour().Report({ bState, bundleName });
    size_t dataSize = pasteData.GetTextSize();
    CalculateTimeConsuming timeC(dataSize, pState);
}

std::pair<std::shared_ptr<PasteData>, PasteDateResult> PasteboardService::GetDistributedData(
    const Event &event, int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    PasteDateResult pasteDateResult;
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR);
        return std::make_pair(nullptr, pasteDateResult);
    }
    std::vector<uint8_t> rawData;
    auto result = clipPlugin->GetPasteData(event, rawData);
    if (result.first != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get data failed");
        Reporter::GetInstance().PasteboardFault().Report({ user, "GET_REMOTE_DATA_FAILED" });
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = result.first;
        return std::make_pair(nullptr, pasteDateResult);
    }

    currentEvent_ = std::move(event);
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->Decode(rawData);
    pasteData->SetOriginAuthority(std::make_pair(pasteData->GetBundleName(), pasteData->GetAppIndex()));
    pasteData->rawDataSize_ = static_cast<int64_t>(rawData.size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set remote data, dataSize=%{public}" PRId64, pasteData->rawDataSize_);
    for (size_t i = 0; i < pasteData->GetRecordCount(); i++) {
        auto item = pasteData->GetRecordAt(i);
        if (item == nullptr || item->GetConvertUri().empty()) {
            continue;
        }
        if (item->GetOriginUri() == nullptr) {
            item->SetConvertUri("");
            continue;
        }
        item->isConvertUriFromRemote = true;
    }
    pasteDateResult.syncTime = result.second;
    pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::E_OK);
    return std::make_pair(pasteData, pasteDateResult);
}

bool PasteboardService::IsConstraintEnabled(int32_t user)
{
    bool isConstraintEnabled = false;
    ErrCode err = AccountSA::OsAccountManager::CheckOsAccountConstraintEnabled(user, CONSTRAINT, isConstraintEnabled);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        err == ERR_OK, false, PASTEBOARD_MODULE_SERVICE, "CheckOsAccountConstraintEnabled failed, %{public}d", err);
    return isConstraintEnabled;
}

bool PasteboardService::IsDisallowDistributed()
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    if (uid == DEVICE_COLLABORATION_UID) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uid from device collaboration");
        return true;
    }
    return false;
}

bool PasteboardService::SetDistributedData(int32_t user, PasteData &data)
{
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!networkId.empty(), false, PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
    Event event;
    event.user = user;
    event.seqId = ++sequenceId_;
    auto expiration = PasteBoardTime::GetBootTimeMs() + EXPIRATION_INTERVAL;
    event.expiration = static_cast<uint64_t>(expiration);
    event.deviceId = networkId;
    event.account = AccountManager::GetInstance().GetCurrentAccount();
    event.status = ClipPlugin::EVT_NORMAL;
    event.dataType = data.GetMimeTypes();
    event.isDelay = data.IsDelayRecord();
    event.dataId = data.GetDataId();
    currentEvent_ = event;

    if (IsConstraintEnabled(user) || IsDisallowDistributed()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "not allowed to send, user:%{public}d", user);
        return false;
    }
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", data.GetDataId());
        return false;
    }
    ShareOption shareOpt = data.GetShareOption();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::InApp, false, PASTEBOARD_MODULE_SERVICE,
        "data share option is in app, dataId:%{public}u", data.GetDataId());
    if (CheckMdmShareOption(data)) {
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::LocalDevice, false, PASTEBOARD_MODULE_SERVICE,
            "data share option is local device, dataId:%{public}u", data.GetDataId());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, isDelay:%{public}d,"
        "expiration:%{public}" PRIu64, event.dataId, event.seqId, event.isDelay, event.expiration);
    return SetCurrentDistributedData(data, event);
}

bool PasteboardService::SetCurrentDistributedData(PasteData &data, Event event)
{
    std::thread thread([this, data, event]() mutable {
        {
            std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
            setDistributedMemory_.event = event;
            setDistributedMemory_.data = std::make_shared<PasteData>(data);
            if (setDistributedMemory_.isRunning) {
                return;
            }
            setDistributedMemory_.isRunning = true;
        }
        bool isNeedCheck = false;
        while (true) {
            auto block = std::make_shared<BlockObject<bool>>(SET_DISTRIBUTED_DATA_INTERVAL, false);
            {
                std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
                if ((event.seqId == setDistributedMemory_.event.seqId && isNeedCheck) ||
                    setDistributedMemory_.data == nullptr) {
                    setDistributedMemory_.data = nullptr;
                    setDistributedMemory_.isRunning = false;
                    break;
                }
                if (!isNeedCheck) {
                    isNeedCheck = true;
                }
                std::thread thread([this, event, block]() mutable {
                    PasteData data;
                    {
                        std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
                        if (setDistributedMemory_.data == nullptr) {
                            block->SetValue(true);
                            return;
                        }
                        event = setDistributedMemory_.event;
                        data = *setDistributedMemory_.data;
                    }
                    auto result = SetCurrentData(event, data);
                    block->SetValue(true);
                });
                thread.detach();
            }
            bool ret = block->GetValue();
            if (!ret) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SetCurrentData timeout,seqId:%{public}hu", event.seqId);
            }
        }
    });
    thread.detach();
    return true;
}

bool PasteboardService::SetCurrentData(Event event, PasteData &data)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", data.GetDataId());
        return false;
    }
    RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_LOAD_DISTRIBUTED_PLUGIN, DFX_SUCCESS);
    bool needFull = data.IsDelayRecord() &&
        moduleConfig_.GetRemoteDeviceMinVersion() == DistributedModuleConfig::FIRST_VERSION;
    if (needFull) {
        GetFullDelayPasteData(event.user, data);
        event.isDelay = false;
        {
            std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
            std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
            PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_);
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }
    GenerateDistributedUri(data);
    std::vector<uint8_t> rawData;
    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    {
        std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
        if (!data.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::SECOND_VERSION)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "distributed data encode failed, dataId:%{public}u, seqId:%{public}hu", event.dataId, event.seqId);
            return false;
        }
    }
    if (data.IsDelayRecord() && !needFull) {
        clipPlugin->RegisterDelayCallback(
            std::bind(&PasteboardService::GetDistributedDelayData, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3),
            std::bind(&PasteboardService::GetDistributedDelayEntry, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }
    clipPlugin->SetPasteData(event, rawData);
    return true;
}

int32_t PasteboardService::GetDistributedDelayEntry(const Event &evt, uint32_t recordId, const std::string &utdId,
    std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64
        ", recordId:%{public}u, type:%{public}s", evt.dataId, evt.seqId, evt.expiration, recordId, utdId.c_str());
    auto [hasData, data] = clips_.Find(evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    auto record = data->GetRecordById(recordId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID),
        PASTEBOARD_MODULE_SERVICE, "recordId=%{public}u invalid, max=%{public}zu", recordId, data->GetRecordCount());

    PasteDataEntry entry;
    entry.SetUtdId(utdId);
    int32_t ret = GetLocalEntryValue(evt.user, *data, *record, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, seqId=%{public}hu, dataId=%{public}u, recordId=%{public}u"
        ", type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_URI) {
        ret = ProcessDistributedDelayUri(evt.user, *data, entry, rawData);
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessDistributedDelayHtml(*data, entry, rawData);
    } else {
        ret = ProcessDistributedDelayEntry(entry, rawData);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "process distributed entry failed, seqId=%{public}hu, dataId=%{public}u, "
        "recordId=%{public}u, type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "type=%{public}s, size=%{public}zu", utdId.c_str(), rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDistributedDelayUri(int32_t userId, PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");

    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    std::string localUri = uri->ToString();
    std::vector<std::string> localUris = { localUri };
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    int32_t ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(localUris, userId, dfsUris);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE,
        "generate distributed uri failed, uri=%{private}s", localUri.c_str());

    auto dfsUri = dfsUris.find(localUri);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(dfsUri != dfsUris.end(), static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "dfsUris is null");
    std::string distributedUri = dfsUri->second.uriStr;
    size_t fileSize = dfsUri->second.fileSize;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri: %{private}s -> %{private}s, fileSize=%{public}zu",
        localUri.c_str(), distributedUri.c_str(), fileSize);

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(distributedUri);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::FILE_URI_PARAM] = distributedUri;
        entry.SetValue(newObject);
        entry.SetFileSize(static_cast<int64_t>(fileSize));
    }

    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode uri failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDistributedDelayHtml(PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_)) {
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }

    PasteData tmp;
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
    tmp.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    if (PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, data.userId_)) {
        PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);
        GenerateDistributedUri(tmp);
    }

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    bool encodeSucc = tmp.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::SECOND_VERSION);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode html failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDistributedDelayEntry(PasteDataEntry &entry, std::vector<uint8_t> &rawData)
{
    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode entry failed, type=%{public}s", entry.GetUtdId().c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetDistributedDelayData(const Event &evt, uint8_t version, std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64,
        evt.dataId, evt.seqId, evt.expiration);
    auto [hasData, data] = clips_.Find(evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    if (version == 0) {
        ret = GetFullDelayPasteData(evt.user, *data);
    } else if (version == 1) {
        ret = GetDelayPasteRecord(evt.user, *data);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get delay data failed, version=%{public}hhu", version);

    auto authorityInfo = data->GetOriginAuthority();
    data->SetBundleInfo(authorityInfo.first, authorityInfo.second);
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authorityInfo);
        PasteboardWebController::GetInstance().SplitWebviewPasteData(*data, bundleIndex, evt.user);
        PasteboardWebController::GetInstance().SetWebviewPasteData(*data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(*data);
    }
    GenerateDistributedUri(*data);

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
    bool encodeSucc = data->Encode(rawData, remoteVersionMin <= DistributedModuleConfig::SECOND_VERSION);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode data failed, dataId:%{public}u, seqId:%{public}hu", evt.dataId, evt.seqId);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "size=%{public}zu", rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetLocalEntryValue(int32_t userId, PasteData &data, PasteDataRecord &record,
    PasteDataEntry &value)
{
    std::string utdId = value.GetUtdId();
    auto entry = record.GetEntry(utdId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, static_cast<int32_t>(PasteboardError::INVALID_MIMETYPE),
        PASTEBOARD_MODULE_SERVICE, "entry is null, recordId=%{public}u, type=%{public}s", record.GetRecordId(),
        utdId.c_str());

    std::string mimeType = entry->GetMimeType();
    value.SetMimeType(mimeType);
    if (entry->HasContent(utdId)) {
        value.SetValue(entry->GetValue());
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    int32_t ret = getter.first->GetRecordValueByType(record.GetRecordId(), value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);

    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + value.rawDataSize_ < MessageParcelWarp::GetRawDataSize()) {
            record.AddEntry(utdId, std::make_shared<PasteDataEntry>(value));
            data.rawDataSize_ += value.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, value.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, value.rawDataSize_);
        }
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetRemoteEntryValue(const AppInfo &appInfo, PasteData &data, PasteDataRecord &record,
    PasteDataEntry &entry)
{
    auto clipPlugin = GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(clipPlugin != nullptr, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL),
        PASTEBOARD_MODULE_SERVICE, "plugin is null");

    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(distRet == static_cast<int32_t>(PasteboardError::E_OK) ||
        distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA), distRet,
        PASTEBOARD_MODULE_SERVICE, "get distribute event failed, ret=%{public}d", distRet);

    std::vector<uint8_t> rawData;
    std::string utdId = entry.GetUtdId();
    int32_t ret = clipPlugin->GetPasteDataEntry(distEvt, record.GetRecordId(), utdId, rawData);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE, "get remote raw data failed");

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessRemoteDelayHtml(distEvt.deviceId, appInfo, rawData, data, record, entry);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "process remote delay html failed");
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    PasteDataEntry tmpEntry;
    tmpEntry.Decode(rawData);
    entry.SetValue(tmpEntry.GetValue());
    entry.rawDataSize_ = static_cast<int64_t>(rawData.size());
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < MessageParcelWarp::GetRawDataSize()) {
            record.AddEntry(utdId, std::make_shared<PasteDataEntry>(entry));
            data.rawDataSize_ += entry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        }
    }

    if (mimeType != MIMETYPE_TEXT_URI) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    return ProcessRemoteDelayUri(distEvt.deviceId, appInfo, data, record, entry);
}

int32_t PasteboardService::ProcessRemoteDelayUri(const std::string &deviceId, const AppInfo &appInfo,
    PasteData &data, PasteDataRecord &record, PasteDataEntry &entry)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");
    std::string distributedUri = uri->ToString();
    record.SetConvertUri(distributedUri);
    record.isConvertUriFromRemote = true;
    record.SetGrantUriPermission(true);

    int64_t uriFileSize = entry.GetFileSize();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri=%{private}s, fileSize=%{public}" PRId64,
        distributedUri.c_str(), uriFileSize);
    if (uriFileSize > 0) {
        int64_t dataFileSize = data.GetFileSize();
        int64_t fileSize = (uriFileSize > INT64_MAX - dataFileSize) ? INT64_MAX : uriFileSize + dataFileSize;
        data.SetFileSize(fileSize);
    }
    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(deviceId, data.GetPasteId());
        int32_t ret = GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant remote uri failed, uri=%{private}s, ret=%{public}d",
            distributedUri.c_str(), ret);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessRemoteDelayHtml(const std::string &remoteDeviceId, const AppInfo &appInfo,
    const std::vector<uint8_t> &rawData, PasteData &data, PasteDataRecord &record, PasteDataEntry &entry)
{
    PasteData tmpData;
    tmpData.Decode(rawData);
    auto htmlRecord = tmpData.GetRecordById(1);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(htmlRecord != nullptr,
        static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED), PASTEBOARD_MODULE_SERVICE, "record is null");
    auto htmlEntry = htmlRecord->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(htmlEntry != nullptr,
        static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED), PASTEBOARD_MODULE_SERVICE, "htmlEntry is null");
    entry.SetValue(htmlEntry->GetValue());
    entry.rawDataSize_ = static_cast<int64_t>(rawData.size());
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < MessageParcelWarp::GetRawDataSize()) {
            record.AddEntry(entry.GetUtdId(), std::make_shared<PasteDataEntry>(entry));
            data.rawDataSize_ += entry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        }

        PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(htmlRecord->GetFrom() != 0, static_cast<int32_t>(PasteboardError::E_OK),
            PASTEBOARD_MODULE_SERVICE, "no uri");

        data.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
        uint32_t htmlRecordId = record.GetRecordId();
        record.SetFrom(htmlRecordId);
        for (auto &recordItem : tmpData.AllRecords()) {
            if (recordItem == nullptr) {
                continue;
            }
            if (!recordItem->GetConvertUri().empty()) {
                recordItem->isConvertUriFromRemote = true;
            }
            if (recordItem->GetFrom() > 0 && recordItem->GetRecordId() != recordItem->GetFrom()) {
                recordItem->SetFrom(htmlRecordId);
                data.AddRecord(*recordItem);
            }
        }
    }
    return ProcessRemoteDelayHtmlInner(remoteDeviceId, appInfo, tmpData, data, entry);
}

int32_t PasteboardService::ProcessRemoteDelayHtmlInner(const std::string &remoteDeviceId, const AppInfo &appInfo,
    PasteData &tmpData, PasteData &data, PasteDataEntry &entry)
{
    int64_t htmlFileSize = tmpData.GetFileSize();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "htmlFileSize=%{public}" PRId64, htmlFileSize);
    if (htmlFileSize > 0) {
        int64_t dataFileSize = data.GetFileSize();
        int64_t fileSize = (htmlFileSize > INT64_MAX - dataFileSize) ? INT64_MAX : htmlFileSize + dataFileSize;
        data.SetFileSize(fileSize);
    }

    bool isInvalid = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!isInvalid, static_cast<int32_t>(PasteboardError::INVALID_URI_ERROR),
        PASTEBOARD_MODULE_SERVICE, "uri invalid");

    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(remoteDeviceId, data.GetPasteId());
        int32_t ret = GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant to %{public}s failed, ret=%{public}d", appInfo.bundleName.c_str(), ret);
    }

    tmpData.SetOriginAuthority(data.GetOriginAuthority());
    tmpData.SetTokenId(data.GetTokenId());
    tmpData.SetRemote(data.IsRemote());
    SetLocalPasteFlag(tmpData.IsRemote(), appInfo.tokenId, tmpData);
    int32_t ret = PostProcessDelayHtmlEntry(tmpData, appInfo, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "post process remote html failed, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetFullDelayPasteData(int32_t userId, PasteData &data)
{
    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    auto delayEntryInfos = DelayManager::GetAllDelayEntryInfo(data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!delayEntryInfos.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no delay entry");
    DelayManager::GetLocalEntryValue(delayEntryInfos, getter.first, data);
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    clips_.ComputeIfPresent(userId, [&data](auto, auto &value) {
        if (data.GetDataId() != value->GetDataId()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "set data fail, data is out time, pre dataId is %{public}d, cur dataId is %{public}d",
                data.GetDataId(), value->GetDataId());
            return true;
        }
        value = std::make_shared<PasteData>(data);
        return true;
    });
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::SyncDelayedData()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    auto [hasData, data] = clips_.Find(appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(tokenId == data->GetTokenId(),
        static_cast<int32_t>(PasteboardError::INVALID_TOKEN_ID), PASTEBOARD_MODULE_SERVICE,
        "tokenId=%{public}u mismatch, local=%{public}u", tokenId, data->GetTokenId());

    int32_t ret = GetFullDelayPasteData(appInfo.userId, *data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get full delay failed, ret=%{public}d", ret);

    std::thread thread([=, userId = appInfo.userId, data = data] {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(data != nullptr, PASTEBOARD_MODULE_SERVICE, "sync delayed data is null");
        data->RemoveEmptyEntry();
        clips_.ComputeIfPresent(userId, [=](auto, auto &value) {
            if (data->GetDataId() == value->GetDataId()) {
                value = std::move(data);
            }
            return true;
        });
    });
    thread.detach();
    return ERR_OK;
}

void PasteboardService::GenerateDistributedUri(PasteData &data)
{
    std::vector<std::string> uris;
    std::vector<size_t> indexes;
    auto userId = GetCurrentAccountId();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
    for (size_t i = 0; i < data.GetRecordCount(); i++) {
        auto item = data.GetRecordAt(i);
        if (item == nullptr) {
            continue;
        }
        item->SetConvertUri("");
        const auto &uri = item->GetOriginUri();
        if (uri == nullptr) {
            continue;
        }
        auto hasGrantUriPermission = item->HasGrantUriPermission();
        const std::string &bundleName = data.GetOriginAuthority().first;
        if (!IsBundleOwnUriPermission(bundleName, *uri) && !hasGrantUriPermission) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "uri:%{private}s, bundleName:%{public}s, appIndex:%{public}d,"
                " has grant:%{public}d", uri->ToString().c_str(), bundleName.c_str(), data.GetOriginAuthority().second,
                hasGrantUriPermission);
            continue;
        }
        uris.emplace_back(uri->ToString());
        indexes.emplace_back(i);
    }
    size_t fileSize = 0;
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    if (!uris.empty()) {
        int ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(uris, userId, dfsUris);
        PASTEBOARD_CHECK_AND_RETURN_LOGE((ret == 0 && !dfsUris.empty()), PASTEBOARD_MODULE_SERVICE,
            "Get remoteUri failed, ret:%{public}d, userId:%{public}d, uri size:%{public}zu.", ret, userId, uris.size());
        for (size_t i = 0; i < indexes.size(); i++) {
            auto item = data.GetRecordAt(indexes[i]);
            if (item == nullptr || item->GetOriginUri() == nullptr) {
                continue;
            }
            auto it = dfsUris.find(item->GetOriginUri()->ToString());
            if (it != dfsUris.end()) {
                item->SetConvertUri(it->second.uriStr);
                fileSize += it->second.fileSize;
            }
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "file size: %{public}zu", fileSize);
    data.SetFileSize(static_cast<int64_t>(fileSize));
}

std::shared_ptr<ClipPlugin> PasteboardService::GetClipPlugin()
{
    auto isOn = moduleConfig_.IsOn();
    if (isOn) {
        auto securityLevel = securityLevel_.GetDeviceSecurityLevel();
#ifdef PB_DATACLASSIFICATION_ENABLE
        if (securityLevel < DATA_SEC_LEVEL3) {
            return nullptr;
        }
#endif
    }
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    if (!isOn || clipPlugin_ != nullptr) {
        return clipPlugin_;
    }
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    RegisterPreSyncCallback(clipPlugin_);
    return clipPlugin_;
}

void PasteboardService::CleanDistributedData(int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->Clear(user);
}

void PasteboardService::CloseDistributedStore(int32_t user, bool isNeedClear)
{
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
    if (isNeedClear) {
        clipPlugin_->Clear(user);
    }
    clipPlugin_->Close(user);
}

void PasteboardService::OnConfigChange(bool isOn)
{
    std::thread thread([=]() {
        OnConfigChangeInner(isOn);
    });
    thread.detach();
}

void PasteboardService::OnConfigChangeInner(bool isOn)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConfigChange isOn: %{public}d.", isOn);
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Clear();
    }
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    if (!isOn) {
        PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        int32_t userId = GetCurrentAccountId();
        clipPlugin_->Close(userId);
        clipPlugin_ = nullptr;
        return;
    }
    SetCriticalTimer();
    auto securityLevel = securityLevel_.GetDeviceSecurityLevel();
#ifdef PB_DATACLASSIFICATION_ENABLE
    if (securityLevel < DATA_SEC_LEVEL3) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "device sec level is %{public}u less than 3.", securityLevel);
        return;
    }
#endif
    if (clipPlugin_ != nullptr) {
        return;
    }
    SubscribeKeyboardEvent();
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    RegisterPreSyncCallback(clipPlugin_);
}

std::string PasteboardService::GetAppLabel(uint32_t tokenId)
{
    auto iBundleMgr = GetAppBundleManager();
    if (iBundleMgr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to cast bundle mgr service.");
        return PasteBoardDialog::DEFAULT_LABEL;
    }
    AppInfo info = GetAppInfo(tokenId);
    AppExecFwk::ApplicationInfo appInfo;
    auto result = iBundleMgr->GetApplicationInfo(info.bundleName, 0, info.userId, appInfo);
    if (!result) {
        return PasteBoardDialog::DEFAULT_LABEL;
    }
    auto &resource = appInfo.labelResource;
    auto label = iBundleMgr->GetStringById(resource.bundleName, resource.moduleName, resource.id, info.userId);
    return label.empty() ? PasteBoardDialog::DEFAULT_LABEL : label;
}

sptr<AppExecFwk::IBundleMgr> PasteboardService::GetAppBundleManager()
{
    auto systemAbilityManager = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to get SystemAbilityManager.");
        return nullptr;
    }
    auto remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to get bundle mgr service.");
        return nullptr;
    }
    return OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
}

void PasteboardService::ChangeStoreStatus(int32_t userId)
{
    PasteboardService::currentUserId_ = userId;
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->ChangeStoreStatus(userId);
}

void PasteBoardCommonEventSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    std::thread thread([=] {
        OnReceiveEventInner(data);
    });
    thread.detach();
}

void PasteBoardCommonEventSubscriber::OnReceiveEventInner(const EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t userId = data.GetCode();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id switched: %{public}d", userId);
        if (pasteboardService_ != nullptr) {
            pasteboardService_->ChangeStoreStatus(userId);
            auto accountId = pasteboardService_->GetCurrentAccountId();
            pasteboardService_->switch_.DeInit();
            pasteboardService_->switch_.Init(accountId);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetSwitch end");
        }
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING) {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t userId = data.GetCode();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id is stopping: %{public}d", userId);
        if (pasteboardService_ != nullptr) {
            pasteboardService_->Clear();
        }
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED) {
        std::lock_guard<std::mutex> lock(mutex_);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen is locked");
        PasteboardService::currentScreenStatus = ScreenEvent::ScreenLocked;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        std::lock_guard<std::mutex> lock(mutex_);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen is unlocked");
        PasteboardService::currentScreenStatus = ScreenEvent::ScreenUnlocked;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
        auto tokenId = want.GetIntParam("accessTokenId", -1);
        if (pasteboardService_ != nullptr) {
            pasteboardService_->ClearUriOnUninstall(tokenId);
        }
    }
}

void PasteboardService::ClearUriOnUninstall(int32_t tokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(tokenId >= 0, PASTEBOARD_MODULE_SERVICE, "tokenId is invalids");
    auto userId = GetCurrentAccountId();
    clips_.ComputeIfPresent(userId, [this, tokenId, userId](auto, auto &pasteData) {
        if (pasteData == nullptr) {
            return true;
        }
        if (pasteData->GetTokenId() != static_cast<uint32_t>(tokenId)) {
            return true;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "clear uri, tokenId=%{public}d", tokenId);
        ClearUriOnUninstall(pasteData);
        delayGetters_.ComputeIfPresent(userId, [](auto, auto &delayGetter) {
            if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
                delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
            }
            return false;
        });
        entryGetters_.ComputeIfPresent(userId, [](auto, auto &entryGetter) {
            if (entryGetter.first != nullptr && entryGetter.second != nullptr) {
                entryGetter.first->AsObject()->RemoveDeathRecipient(entryGetter.second);
            }
            return false;
        });
        return true;
    });
}

void PasteboardService::ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteData != nullptr, PASTEBOARD_MODULE_SERVICE, "pasteData is null");
    std::thread thread([pasteData, this]() {
        {
            std::unique_lock<std::shared_mutex> threadWriteLock(pasteDataMutex_);
            if (!pasteData->HasMimeType(MIMETYPE_TEXT_URI)) {
                return;
            }

            auto emptyUri = std::make_shared<OHOS::Uri>("");
            size_t recordCount = pasteData->GetRecordCount();
            for (size_t i = 0; i < recordCount; i++) {
                auto item = pasteData->GetRecordAt(i);
                if (item == nullptr || item->GetOriginUri() == nullptr) {
                    continue;
                }
                item->SetUri(emptyUri);
            }
        }

        std::lock_guard<decltype(mutex)> lockGuard(mutex);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        clipPlugin_->Clear(pasteData->userId_);
    });
    thread.detach();
}

void PasteBoardAccountStateSubscriber::OnStateChanged(const AccountSA::OsAccountStateData &data)
{
    std::thread thread([=]() {
        OnStateChangedInner(data);
    });
    thread.detach();
}

void PasteBoardAccountStateSubscriber::OnStateChangedInner(const AccountSA::OsAccountStateData &data)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "state: %{public}d, fromId: %{public}d, toId: %{public}d,"
        "callback is nullptr: %{public}d", data.state, data.fromId, data.toId, data.callback == nullptr);
    if (data.state == AccountSA::OsAccountState::STOPPING && pasteboardService_ != nullptr) {
        pasteboardService_->CloseDistributedStore(data.fromId, true);
    }
    if (data.callback != nullptr) {
        data.callback->OnComplete();
    }
}

bool PasteboardService::SubscribeKeyboardEvent()
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    if (inputEventCallback_ != nullptr) {
        return true;
    }
    inputEventCallback_ = std::make_shared<InputEventCallback>();
    int32_t monitorId = MMI::InputManager::GetInstance()->AddMonitor(
        std::static_pointer_cast<MMI::IInputEventConsumer>(inputEventCallback_), MMI::HANDLE_EVENT_TYPE_KEY);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add monitor ret is: %{public}d", monitorId);
    return monitorId >= 0;
}

void PasteboardService::PasteboardEventSubscriber()
{
    EventCenter::GetInstance().Subscribe(PasteboardEvent::DISCONNECT, [this](const OHOS::MiscServices::Event &event) {
        auto &evt = static_cast<const PasteboardEvent &>(event);
        auto networkId = evt.GetNetworkId();
        if (networkId.empty()) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
            return;
        }
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.EraseIf([networkId, this](auto &key, auto &value) {
            if (key == networkId) {
                CloseP2PLink(networkId);
                return true;
            }
            return false;
        });
    });
}

void PasteboardService::CommonEventSubscriber()
{
    if (commonEventSubscriber_ != nullptr) {
        return;
    }
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    commonEventSubscriber_ = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, this);
    EventFwk::CommonEventManager::SubscribeCommonEvent(commonEventSubscriber_);
}

void PasteboardService::AccountStateSubscriber()
{
    if (accountStateSubscriber_ != nullptr) {
        return;
    }
    std::set<AccountSA::OsAccountState> states = { AccountSA::OsAccountState::STOPPING,
        AccountSA::OsAccountState::CREATED, AccountSA::OsAccountState::SWITCHING,
        AccountSA::OsAccountState::SWITCHED, AccountSA::OsAccountState::UNLOCKED,
        AccountSA::OsAccountState::STOPPED, AccountSA::OsAccountState::REMOVED };
    AccountSA::OsAccountSubscribeInfo subscribeInfo(states, true);
    accountStateSubscriber_ = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, this);
    AccountSA::OsAccountManager::SubscribeOsAccount(accountStateSubscriber_);
}

void PasteboardService::RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap &observerMap)
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callObserverKey = std::make_pair(userId, pid);
    auto it = observerMap.find(callObserverKey);
    if (it == observerMap.end()) {
        return;
    }
    observerMap.erase(callObserverKey);
}

int32_t PasteboardService::AppExit(pid_t pid)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid %{public}d exit.", pid);
    int32_t userId = GetCurrentAccountId();
    RemoveObserverByPid(userId, pid, observerLocalChangedMap_);
    RemoveObserverByPid(userId, pid, observerRemoteChangedMap_);
    RemoveObserverByPid(COMMON_USERID, pid, observerEventMap_);
    entityObserverMap_.Erase(pid);
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);
    ClearInputMethodPidByPid(userId, pid);
    std::vector<std::string> networkIds;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.EraseIf([pid, &networkIds, this](auto &networkId, auto &pidMap) {
            pidMap.EraseIf([pid, this](const auto &key, const auto &value) {
                if (value.callPid == pid) {
                    PasteStart(key);
                    return true;
                }
                return false;
            });
            if (pidMap.Empty()) {
                networkIds.emplace_back(networkId);
                return true;
            }
            return false;
        });
    }
    for (const auto &id : networkIds) {
        CloseP2PLink(id);
    }
    bool isExist = clients_.ComputeIfPresent(pid, [pid](auto, auto &value) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "find client death recipient succeed, pid=%{public}d", pid);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(value.first != nullptr && value.second != nullptr, false,
            PASTEBOARD_MODULE_SERVICE, "client death recipient is null");
        value.first->RemoveDeathRecipient(value.second);
        return false;
    });
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(isExist, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "find client death recipient failed, pid=%{public}d", pid);
    return ERR_OK;
}

void PasteboardService::PasteboardDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    (void)remote;
    service_.AppExit(pid_);
}

PasteboardService::PasteboardDeathRecipient::PasteboardDeathRecipient(
    PasteboardService &service, pid_t pid) : service_(service), pid_(pid)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Construct Pasteboard Client Death Recipient, pid: %{public}d", pid);
}

int32_t PasteboardService::RegisterClientDeathObserver(const sptr<IRemoteObject> &observer)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    sptr<PasteboardDeathRecipient> deathRecipient = sptr<PasteboardDeathRecipient>::MakeSptr(*this, pid);
    observer->AddDeathRecipient(deathRecipient);
    clients_.InsertOrAssign(pid, std::make_pair(observer, deathRecipient));
    return ERR_OK;
}

int32_t PasteboardService::DetachPasteboard()
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    return AppExit(pid);
}

std::function<void(const OHOS::MiscServices::Event &)> PasteboardService::RemotePasteboardChange()
{
    return [this](const OHOS::MiscServices::Event &event) {
        (void)event;
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto &observers : observerRemoteChangedMap_) {
            for (const auto &observer : *(observers.second)) {
                observer->OnPasteboardChanged();
            }
        }
    };
}

int32_t PasteboardService::CallbackEnter(uint32_t code)
{
    if (!IPCSkeleton::IsLocalCalling()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid request, only support local, cmd:%{public}u", code);
        return ERR_TRANSACTION_FAILED;
    }
    if (code == static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA)) {
        return ERR_NONE;
    }
    pid_t pid = IPCSkeleton::GetCallingPid();
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid:%{public}d, uid:%{public}d, cmd:%{public}u", pid, uid, code);
    return ERR_NONE;
}

int32_t PasteboardService::CallbackExit(uint32_t code, int32_t result)
{
    if (code == static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA)) {
        return ERR_NONE;
    }
    pid_t pid = IPCSkeleton::GetCallingPid();
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid:%{public}d, uid:%{public}d, cmd:%{public}u, ret:%{public}d",
        pid, uid, code, result);
    return ERR_NONE;
}

void InputEventCallback::OnKeyInputEventForPaste(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
    auto keyItems = keyEvent->GetKeyItems();
    if (keyItems.size() != CTRLV_EVENT_SIZE) {
        return;
    }
    if ((keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_DOWN) &&
        ((keyItems[0].GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_LEFT) ||
        (keyItems[0].GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_RIGHT)) &&
        keyItems[1].GetKeyCode() == MMI::KeyEvent::KEYCODE_V) {
        int32_t windowId = keyEvent->GetTargetWindowId();
        std::unique_lock<std::shared_mutex> lock(inputEventMutex_);
        windowPid_ = MMI::InputManager::GetInstance()->GetWindowPid(windowId);
        actionTime_ =
            static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
        std::shared_ptr<BlockObject<int32_t>> block = nullptr;
        {
            std::unique_lock<std::shared_mutex> blockMapLock(blockMapMutex_);
            auto it = blockMap_.find(windowPid_);
            if (it != blockMap_.end()) {
                block = it->second;
            } else {
                block = std::make_shared<BlockObject<int32_t>>(WAIT_TIME_OUT, 0);
                blockMap_.insert(std::make_pair(windowPid_, block));
            }
        }
        if (block != nullptr) {
            block->SetValue(SET_VALUE_SUCCESS);
        }
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "keyEvent, inputType_ = %{public}d", inputType_);
    if (inputType_ == INPUTTYPE_PASTE) {
        OnKeyInputEventForPaste(keyEvent);
    } else if (inputType_ == INPUTTYPE_PRESYNC) {
        if (pasteboardService_) {
            pasteboardService_->PreSyncRemotePasteboardData();
        }
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid inputType_ = %{public}d", inputType_);
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pointerEvent, inputType_ = %{public}d", inputType_);
    if (inputType_ == INPUTTYPE_PRESYNC) {
        if (pasteboardService_) {
            pasteboardService_->PreSyncRemotePasteboardData();
        }
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const {}

bool InputEventCallback::IsCtrlVProcess(uint32_t callingPid, bool isFocused)
{
    std::shared_ptr<BlockObject<int32_t>> block = nullptr;
    {
        std::unique_lock<std::shared_mutex> blockMapLock(blockMapMutex_);
        auto it = blockMap_.find(callingPid);
        if (it != blockMap_.end()) {
            block = it->second;
        } else {
            block = std::make_shared<BlockObject<int32_t>>(WAIT_TIME_OUT, 0);
            blockMap_.insert(std::make_pair(callingPid, block));
        }
    }
    if (block != nullptr) {
        block->GetValue();
    }
    std::shared_lock<std::shared_mutex> lock(inputEventMutex_);
    auto curTime = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto ret = (callingPid == static_cast<uint32_t>(windowPid_) || isFocused) && curTime >= actionTime_ &&
        curTime - actionTime_ < EVENT_TIME_OUT;
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "windowPid is: %{public}d, callingPid is: %{public}d,"
            "curTime is: %{public}" PRIu64 ", actionTime is: %{public}" PRIu64 ", isFocused is: %{public}d",
            windowPid_, callingPid, curTime, actionTime_, isFocused);
    }
    return ret;
}

void InputEventCallback::Clear()
{
    std::unique_lock<std::shared_mutex> lock(inputEventMutex_);
    actionTime_ = 0;
    windowPid_ = 0;
    std::unique_lock<std::shared_mutex> blockMapLock(blockMapMutex_);
    blockMap_.clear();
}
} // namespace MiscServices
} // namespace OHOS
