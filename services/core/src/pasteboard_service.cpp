/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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
#include "fd_san.h"
#include "hiview_adapter.h"
#include "input_method_controller.h"
#include "ipasteboard_changed_observer.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "mem_mgr_client.h"
#include "message_parcel_warp.h"
#include "os_account_manager.h"
#include "parameters.h"
#include "pasteboard_ability_manager.h"
#include "pasteboard_common.h"
#include "common/pasteboard_common_utils.h"
#include "pasteboard_delay_manager.h"
#include "pasteboard_dialog.h"
#include "pasteboard_disposable_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "paste_data_info.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_pattern.h"
#include "pasteboard_mime_utils.h"
#include "pasteboard_time.h"
#include "pasteboard_trace.h"
#include "pasteboard_web_controller.h"
#include "permission/permission_utils.h"
#include "remote_file_share.h"
#include "res_sched_client.h"
#include "reporter.h"
#include "distributed_module_config.h"
#include "file_mount_manager.h"
#ifdef PB_SCREENLOCK_MGR_ENABLE
#include "screenlock_manager.h"
#endif // PB_SCREENLOCK_MGR_ENABLE
#include "tokenid_kit.h"
#include "uri_permission_manager_client.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif // SCENE_BOARD_ENABLE
#ifdef PB_COCKPIT_PLATFORM_ENABLE
#include "pasteboard_subprofile_subscriber.h"
#include "os_account_subprofile_client.h"
#endif // PB_COCKPIT_PLATFORM_ENABLE

namespace OHOS {
namespace MiscServices {
using namespace Rosen;
using namespace std::chrono;
using namespace Storage::DistributedFile;
using namespace RadarReporter;
using namespace UeReporter;
namespace {
constexpr int32_t INIT_INTERVAL = 10000L;
constexpr uint32_t MAX_IPC_THREAD_NUM = 32;
constexpr const char *PASTEBOARD_SERVICE_NAME = "PasteboardService";
constexpr const char *SECURE_PASTE_PERMISSION = "ohos.permission.SECURE_PASTE";
constexpr const char *READ_PASTEBOARD_PERMISSION = "ohos.permission.READ_PASTEBOARD";
constexpr const char *TRANSMIT_CONTROL_PROP_KEY = "persist.distributed_scene.datafiles_trans_ctrl";
constexpr const char *NETWORK_DEV_NUM = "NETWORK_DEV_NUM";
constexpr int32_t INVALID_VERSION = -1;
constexpr int32_t WIFI_DISABLED = 1;
constexpr int32_t ADD_PERMISSION_CHECK_SDK_VERSION = 12;
constexpr int32_t CTRLV_EVENT_SIZE = 2;
constexpr int32_t CONTROL_TYPE_ALLOW_SEND_RECEIVE = 1;
constexpr uint32_t EVENT_TIME_OUT = 2000;
constexpr uint32_t MAX_BUNDLE_NAME_LENGTH = 127;
constexpr int32_t SET_VALUE_SUCCESS = 1;

const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(new PasteboardService());
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
std::mutex PasteboardService::historyMutex_;
std::shared_mutex PasteboardService::pasteDataMutex_;
std::vector<std::string> PasteboardService::dataHistory_;
std::shared_ptr<Command> PasteboardService::copyHistory;
std::shared_ptr<Command> PasteboardService::copyData;
std::atomic<int32_t> PasteboardService::currentUserId_{ERROR_USERID};

const std::string PasteboardService::REGISTER_PRESYNC_MONITOR = "RegisterPresyncMonitor";
const std::string PasteboardService::UNREGISTER_PRESYNC_MONITOR = "UnregisterPresyncMonitor";

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
        auto userId = ResolveMainDisplayUserId();
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
    auto foregroundUsers = ResolveForegroundUsers();
    for (const auto &ctx : foregroundUsers) {
        if (!ctx.isValid) {
            continue;
        }
        bool isLocked = false;
        auto ret = screenLockManager->IsLockedWithUserId(ctx.userId, isLocked);
        if (ret == ERR_OK) {
            screenStatusMap_.InsertOrAssign(ctx.userId,
                isLocked ? ScreenEvent::ScreenLocked : ScreenEvent::ScreenUnlocked);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen status userId=%{public}d is %{public}d",
                ctx.userId, isLocked ? ScreenEvent::ScreenLocked : ScreenEvent::ScreenUnlocked);
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "IsLockedWithUserId failed, userId=%{public}d, ret=%{public}d", ctx.userId, ret);
        }
    }
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
    Loader loader;
    uid_ = loader.LoadUid();
    int32_t capacity = OHOS::system::GetIntParameter("const.pasteboard.local_data_capacity",
        DEFAULT_LOCAL_CAPACITY);
    int64_t maxLocalCapacity =
        (capacity >= MIN_LOCAL_CAPACITY && capacity <= MAX_LOCAL_CAPACITY) ? capacity : DEFAULT_LOCAL_CAPACITY;
    maxLocalCapacity_.store(maxLocalCapacity * SIZE_K * SIZE_K);
    moduleConfig_.Init();
    moduleConfig_.Watch(std::bind(&PasteboardService::OnConfigChange, this, std::placeholders::_1));
    ffrtTimer_ = FFRTPool::GetTimer("pasteboard_service");
    UpdateAgedTime();
    AddSysAbilityListener();

    if (Init() != ERR_OK && serviceHandler_ != nullptr) {
        HandleInitFailure();
        return;
    }
    std::thread thread([this]() {
        auto userId = ResolveMainDisplayUserId();
        if (userId != ERROR_USERID) {
            switch_.Init(userId);
        }
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "SwitchInit");
    thread.detach();
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

void PasteboardService::OnStop()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStop Started.");
    std::lock_guard<std::mutex> lock(saMutex_);
    if (PasteboardService::state_ != ServiceRunningState::STATE_RUNNING) {
        return;
    }

#ifdef PB_COCKPIT_PLATFORM_ENABLE
    SubProfileUnsubscriber();
#endif

    serviceHandler_ = nullptr;
    PasteboardService::state_ = ServiceRunningState::STATE_NOT_START;
    DMAdapter::GetInstance().DeInitialize();
    if (commonEventSubscriber_ != nullptr) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(commonEventSubscriber_);
    }
    moduleConfig_.DeInit();
    switch_.DeInit();
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner: userId=%{public}d, bundleName=%{public}s",
        userId, appInfo.bundleName.c_str());
    RADAR_REPORT(DFX_CLEAR_PASTEBOARD, DFX_MANUAL_CLEAR, DFX_SUCCESS);
    auto [hasData, data] = clips_.Find(userId);
    if (hasData) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner: found data for userId=%{public}d, erasing", userId);
        clips_.Erase(userId);
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    CleanDistributedData(userId);
    if (hasData) {
        std::string bundleName = GetAppBundleName(appInfo);
        NotifyObservers(bundleName, userId, PasteboardEventStatus::PASTEBOARD_CLEAR);
    }
    if (!HasActivePasteboardWork()) {
        CancelCriticalTimer();
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner leave: clips_.Size=%{public}zu, userId=%{public}d",
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
        fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);
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
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}d", appInfo.userId);
    auto validRet = IsDataValid(*data, tokenId, appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(validRet == static_cast<int32_t>(PasteboardError::E_OK), validRet,
        PASTEBOARD_MODULE_SERVICE, "paste data is invalid, ret=%{public}d", validRet);
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
    return GrantUriPermission(grantUris, appInfo.tokenId, isRemoteData);
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

int32_t PasteboardService::IsDataValid(PasteData &pasteData, uint32_t tokenId, int32_t userId)
{
    if (pasteData.IsDraggedData() || !pasteData.IsValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is invalid");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    if (IsDataAged(userId)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is aged");
        return static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR);
    }
    auto screenStatus = GetScreenStatus(userId);
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
        isPeerOnline = DMAdapter::GetInstance().IsDeviceOnline(peerNetId);
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

void PasteboardService::RemoveInvalidRemoteUri(std::vector<Uri> &grantUris)
{
    auto newEnd = std::remove_if(grantUris.begin(), grantUris.end(),
        [](const Uri& uri) {
            std::string puri = uri.ToString();
            return puri.find("networkid=") == std::string::npos;
        });
    grantUris.erase(newEnd, grantUris.end());
}

int32_t PasteboardService::HasPasteData(bool &funcResult)
{
    funcResult = HasPasteData();
    return ERR_OK;
}

bool PasteboardService::HasPasteData()
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }

    if (GetScreenStatus(userId) == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            return true;
        }
    }

    auto it = clips_.Find(userId);
    if (it.first && (it.second != nullptr)) {
        auto tokenId = IPCSkeleton::GetCallingTokenID();
        auto ret = IsDataValid(*(it.second), tokenId, userId);
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
        isUriProxyGrant, tokenId, PASTEBOARD_MODULE_SERVICE, "No permission, callingTokenId= %{public}u", tokenId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "originTokenId= %{public}u.", originTokenId);
    return originTokenId;
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
    RefreshCriticalState();
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
        PasteBoardCommonUtils::SetThreadTaskName(thread, "ClearAgedData");
        thread.detach();
    };

    std::string taskName = "data_expiration[userId=" + std::to_string(userId) + "]";
    ffrtTimer_->SetTimer(taskName, task, static_cast<uint32_t>(agedTime_.load()));
}

int32_t PasteboardService::GetMimeTypes(std::vector<std::string> &funcResult)
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (GetScreenStatus(userId) == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            if (distEvt.version != ClipPlugin::InfoType::DEFAULT) {
                return GetRemoteMimeTypes(funcResult, distEvt);
            }
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

int32_t PasteboardService::GetPasteDataInfo(PasteDataInfo &pasteDataInfo)
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetDataInfo userId: %{public}d, clips_ find: %{public}d",
        userId, clips_.Find(userId).first);
    auto it = clips_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(it.first, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "Can not find data. userId: %{public}d", userId);
    if (it.second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is nullptr. userId: %{public}d", userId);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    auto &pasteData = *(it.second);
    auto ret = IsDataValid(pasteData, IPCSkeleton::GetCallingTokenID(), userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "pasteData is invalid, ret is %{public}d", ret);
    
    pasteDataInfo.isDelayedData = pasteData.IsDelayData();
    pasteDataInfo.isDelayedRecord = pasteData.IsDelayRecord();
    pasteDataInfo.mimeTypes = pasteData.GetMimeTypes();
    pasteDataInfo.rawDataSize = pasteData.rawDataSize_;

    int32_t textSize = 0;
    int32_t htmlSize = 0;
    for (size_t i = 0; i < pasteData.GetRecordCount(); ++i) {
        auto record = pasteData.GetRecordAt(i);
        if (record == nullptr) {
            continue;
        }
        auto plainText = record->GetPlainTextV0();
        if (plainText != nullptr) {
            textSize += static_cast<int32_t>(plainText->size());
        }
        auto htmlText = record->GetHtmlTextV0();
        if (htmlText != nullptr) {
            htmlSize += static_cast<int32_t>(htmlText->size());
        }
    }
    pasteDataInfo.textDataSize = textSize;
    pasteDataInfo.htmlDataSize = htmlSize;

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
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto userId = GetAppInfo(tokenId).userId;
    if (GetScreenStatus(userId) == ScreenEvent::ScreenUnlocked) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
            auto it = std::find(distEvt.dataType.begin(), distEvt.dataType.end(), mimeType);
            if (it != distEvt.dataType.end()) {
                return true;
            }
            if (IsBasicType(mimeType)) {
                return false;
            }
            if (distEvt.version != ClipPlugin::InfoType::DEFAULT) {
                return HasRemoteDataType(mimeType, distEvt);
            }
            PasteData data;
            int32_t syncTime = 0;
            if (GetRemoteData(userId, distEvt, data, syncTime) != static_cast<int32_t>(PasteboardError::E_OK)) {
                return false;
            }
        }
    }
    return HasLocalDataType(mimeType, tokenId, userId);
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
    auto screenStatus = GetScreenStatus(appInfo.userId);
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
    auto ret = IsDataValid(*(it.second), tokenId, appInfo.userId);
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
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto userId = GetAppInfo(tokenId).userId;
    bool hasPlain = HasLocalDataType(MIMETYPE_TEXT_PLAIN, tokenId, userId);
    bool hasHTML = HasLocalDataType(MIMETYPE_TEXT_HTML, tokenId, userId);
    if (!hasHTML && !hasPlain) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no text");
        std::vector<Pattern>().swap(funcResult);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
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

std::vector<std::string> PasteboardService::GetLocalMimeTypes()
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
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
    auto ret = IsDataValid(*(it.second), tokenId, userId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "pasteData is invalid, tokenId is %{public}d, userId: %{public}d, ret is %{public}d",
            tokenId, userId, ret);
        return {};
    }
    return it.second->GetMimeTypes();
}

bool PasteboardService::HasLocalDataType(const std::string &mimeType, uint32_t tokenId, int32_t userId)
{
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
    auto ret = IsDataValid(*(it.second), tokenId, userId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "pasteData is invalid, tokenId is %{public}d, userId: %{public}d,"
            "mimeType: %{public}s, ret is %{public}d",
            tokenId, userId, mimeType.c_str(), ret);
        return false;
    }
    auto screenStatus = GetScreenStatus(userId);
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

int32_t PasteboardService::GetDataSource(std::string &bundleName)
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
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

int32_t PasteboardService::SetPasteData(int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer,
    const sptr<IPasteboardDelayGetter> &delayGetter, const sptr<IPasteboardEntryGetter> &entryGetter)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        fd >= 0, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE, "fd invalid");
    fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);
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

void PasteboardService::InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin)
{
    if (!clipPlugin) {
        return;
    }
    clipPlugin->RegisterPreSyncCallback(std::bind(&PasteboardService::PreEstablishP2PLinkCallback,
        this, std::placeholders::_1, std::placeholders::_2));
    clipPlugin->RegisterPreSyncMonitorCallback(std::bind(&PasteboardService::PreSyncSwitchMonitorCallback, this));
    clipPlugin->SetMaxLocalCapacity(maxLocalCapacity_.load() / SIZE_K / SIZE_K);
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
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PreSyncSwitchMo");
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
        PasteBoardCommonUtils::SetThreadTaskName(thread, "RegisterPreSync");
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
    PasteBoardCommonUtils::SetThreadTaskName(thread, "SyncDelayedData");
    thread.detach();
    return ERR_OK;
}

void PasteBoardCommonEventSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    std::thread thread([=] {
        OnReceiveEventInner(data);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "OnReceiveEvent");
    thread.detach();
}

void PasteBoardCommonEventSubscriber::OnReceiveEventInner(const EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    int32_t eventState = data.GetCode();
    int32_t userId = data.GetCode();

    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        HandleUserSwitched(data);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING) {
        HandleUserStopping(data);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED) {
        HandleScreenLocked(data);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        HandleScreenUnlocked(data);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
        HandlePackageRemoved(want);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_POWER_STATE && eventState == WIFI_DISABLED) {
        HandleWifiDisabled(userId);
    }
}

void PasteBoardCommonEventSubscriber::HandleUserSwitched(const EventFwk::CommonEventData &data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (pasteboardService_ != nullptr) {
        auto context = pasteboardService_->ResolveEventUser(data);
        if (!context.isValid) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "user switched userId invalid.");
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id switched: %{public}d", context.userId);
        pasteboardService_->ChangeStoreStatus(context.userId);
        pasteboardService_->switch_.DeInit();
        pasteboardService_->switch_.Init(context.userId);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetSwitch end, userId=%{public}d", context.userId);
    }
}

void PasteBoardCommonEventSubscriber::HandleUserStopping(const EventFwk::CommonEventData &data)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (pasteboardService_ != nullptr) {
        auto context = pasteboardService_->ResolveEventUser(data);
        if (!context.isValid) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "user stopping userId invalid.");
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id is stopping: %{public}d", context.userId);
        pasteboardService_->ClearByEventUser(context.userId);
    }
}

void PasteBoardCommonEventSubscriber::HandleScreenLocked(const EventFwk::CommonEventData &data)
{
    if (pasteboardService_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "pasteboardService_ is null.");
        return;
    }
    auto context = pasteboardService_->ResolveUserIdFromWant(data.GetWant());
    if (!context.isValid) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "screen locked userId invalid.");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen is locked, userId=%{public}d", context.userId);
    pasteboardService_->screenStatusMap_.InsertOrAssign(context.userId, ScreenEvent::ScreenLocked);
}

void PasteBoardCommonEventSubscriber::HandleScreenUnlocked(const EventFwk::CommonEventData &data)
{
    if (pasteboardService_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "pasteboardService_ is null.");
        return;
    }
    auto context = pasteboardService_->ResolveUserIdFromWant(data.GetWant());
    if (!context.isValid) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "screen unlocked userId invalid.");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen is unlocked, userId=%{public}d", context.userId);
    pasteboardService_->screenStatusMap_.InsertOrAssign(context.userId, ScreenEvent::ScreenUnlocked);
}

void PasteBoardCommonEventSubscriber::HandlePackageRemoved(const EventFwk::Want &want)
{
    auto tokenId = want.GetIntParam("accessTokenId", -1);
    if (pasteboardService_ != nullptr) {
        auto context = pasteboardService_->ResolveUserIdFromWant(want);
        if (!context.isValid) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "package removed userId invalid.");
            return;
        }
        pasteboardService_->ClearUriOnUninstall(context.userId, tokenId);
    }
}

void PasteBoardCommonEventSubscriber::HandleWifiDisabled(int32_t userId)
{
    pasteboardService_->HandleWifiOffAndClearDistributedEvent(userId);
}

void PasteboardService::ClearUriOnUninstall(int32_t userId, int32_t tokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(tokenId >= 0, PASTEBOARD_MODULE_SERVICE, "tokenId is invalid");
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "userId is invalid");
    clips_.ComputeIfPresent(userId, [this, tokenId, userId](auto, auto &pasteData) {
        if (pasteData == nullptr) {
            return true;
        }
        if (pasteData->GetTokenId() != static_cast<uint32_t>(tokenId)) {
            return true;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "clear uri, tokenId=%{public}d, userId=%{public}d",
            tokenId, userId);
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
    PasteBoardCommonUtils::SetThreadTaskName(thread, "ClearUriUninsta");
    thread.detach();
}

void PasteBoardAccountStateSubscriber::OnStateChanged(const AccountSA::OsAccountStateData &data)
{
    std::thread thread([=]() {
        OnStateChangedInner(data);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "OnStateChanged");
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
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_POWER_STATE);
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

#ifdef PB_COCKPIT_PLATFORM_ENABLE
void PasteboardService::SubProfileSubscriber()
{
    if (subProfileSubscriber_ != nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "subProfileSubscriber_ already exists, skip subscribe");
        return;
    }

    std::set<AccountSA::OsAccountSubProfileEventType> types = {
        AccountSA::OsAccountSubProfileEventType::CREATED,
        AccountSA::OsAccountSubProfileEventType::DELETED,
        AccountSA::OsAccountSubProfileEventType::SWITCHING,
        AccountSA::OsAccountSubProfileEventType::SWITCHED
    };

    subProfileSubscriber_ = std::make_shared<PasteboardSubProfileSubscriber>(this);
    ErrCode err = AccountSA::OsAccountSubProfileClient::GetInstance().SubscribeOsAccountSubProfileEvents(
        types, subProfileSubscriber_);
    if (err != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SubscribeOsAccountSubProfileEvents failed, err=%{public}d", err);
        subProfileSubscriber_ = nullptr;
        return;
    }

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SubProfileSubscriber initialized successfully");
}

void PasteboardService::SubProfileUnsubscriber()
{
    if (subProfileSubscriber_ == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "subProfileSubscriber_ is nullptr, skip unsubscribe");
        return;
    }
    
    ErrCode err = AccountSA::OsAccountSubProfileClient::GetInstance().UnsubscribeOsAccountSubProfileEvents(
        subProfileSubscriber_);
    if (err != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "UnsubscribeOsAccountSubProfileEvents failed, err=%{public}d", err);
    }
    
    subProfileSubscriber_ = nullptr;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Unsubscribed sub profile events");
}
#endif // PB_COCKPIT_PLATFORM_ENABLE

PasteboardService::PasteboardDeathRecipient::PasteboardDeathRecipient(
    PasteboardService &service, pid_t pid, int32_t userId) : service_(service), pid_(pid), userId_(userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "Construct Pasteboard Client Death Recipient, pid: %{public}d, userId: %{public}d", pid, userId);
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
        (((keyItems[0].GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_LEFT) ||
        (keyItems[0].GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_RIGHT)) &&
        keyItems[1].GetKeyCode() == MMI::KeyEvent::KEYCODE_V)) {
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