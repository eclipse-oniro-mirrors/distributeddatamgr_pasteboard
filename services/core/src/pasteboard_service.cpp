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
#include "pasteboard_event_dfx.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_pattern.h"
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
constexpr const char *SSL_SO_PATH = "libcrypto_openssl.z.so";
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
constexpr int32_t WIFI_DISABLED = 1;
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
constexpr uint16_t MAX_TRANSFER_SIZE = 1300;

const bool G_REGISTER_RESULT = SystemAbility::MakeAndRegisterAbility(new PasteboardService());
const std::string CONSTRAINT = "constraint.distributed.transmission.outgoing";
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
std::shared_mutex PasteboardService::pasteDataMutex_;
std::atomic<int32_t> PasteboardService::currentUserId_{ERROR_USERID};

const std::string PasteboardService::REGISTER_PRESYNC_MONITOR = "RegisterPresyncMonitor";
const std::string PasteboardService::UNREGISTER_PRESYNC_MONITOR = "UnregisterPresyncMonitor";
const std::string PasteboardService::P2P_ESTABLISH_STR = "P2pEstablish";
const std::string PasteboardService::P2P_PRESYNC_ID = "P2pPreSyncId_";

PasteboardService::PasteboardService(): SystemAbility(PASTEBOARD_SERVICE_ID, true)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardService Start.");
    PasteboardService::state_ = ServiceRunningState::STATE_NOT_START;
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
    systemEventListener_->InitScreenStatus();
    return ERR_OK;
}

void PasteboardService::UpdateAgedTime()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardService OnStart.");
    std::lock_guard<std::mutex> lock(saMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(PasteboardService::state_ != ServiceRunningState::STATE_RUNNING,
        PASTEBOARD_MODULE_SERVICE, "PasteboardService is already running.");
    IPCSkeleton::SetMaxWorkThreadNum(MAX_IPC_THREAD_NUM);
    InitServiceHandler();
    
    distributedManager_ = std::make_unique<PasteboardDistributedManager>(*this);
    p2pManager_ = std::make_unique<PasteboardP2PManager>(*this);
    uriHandler_ = std::make_unique<PasteboardUriHandler>(*this);
    observerManager_ = std::make_unique<PasteboardObserverManager>(*this);
    entityRecognizer_ = std::make_unique<PasteboardEntityRecognizer>(*this);
    permissionChecker_ = std::make_unique<PasteboardPermissionChecker>(*this);
    appInfoHelper_ = std::make_unique<PasteboardAppInfoHelper>(*this);
    delayDataHandler_ = std::make_unique<PasteboardDelayDataHandler>(*this);
    systemEventListener_ = std::make_unique<PasteboardSystemEventListener>(*this);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Managers initialized.");
    
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
    systemEventListener_->AddSysAbilityListener();

    if (Init() != ERR_OK && serviceHandler_ != nullptr) {
        HandleInitFailure();
        return;
    }
    auto callback = [this]() {
        auto userId = ResolveMainDisplayUserId();
        if (userId != ERROR_USERID) {
            switch_.Init(userId);
        }
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
    dumpManager_ = std::make_unique<PasteboardDumpManager>(*this);
    dumpManager_->InitializeDumpCommands();
    systemEventListener_->CommonEventSubscriber();
    systemEventListener_->AccountStateSubscriber();
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    systemEventListener_->DistributedAccountSubscriber();
#endif
    systemEventListener_->PasteboardEventSubscriber();
}

void PasteboardService::OnStop()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStop Started.");
    std::lock_guard<std::mutex> lock(saMutex_);
    if (PasteboardService::state_ != ServiceRunningState::STATE_RUNNING) {
        return;
    }
    
    distributedManager_.reset();
    p2pManager_.reset();
    uriHandler_.reset();
    observerManager_.reset();
    entityRecognizer_.reset();
    permissionChecker_.reset();
    appInfoHelper_.reset();
    delayDataHandler_.reset();
    systemEventListener_.reset();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Managers destroyed.");
    
    serviceHandler_ = nullptr;
    PasteboardService::state_ = ServiceRunningState::STATE_NOT_START;
    DMAdapter::GetInstance().DeInitialize();
    if (systemEventListener_ && systemEventListener_->commonEventSubscriber_) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(systemEventListener_->commonEventSubscriber_);
    }
    systemEventListener_.reset();
    moduleConfig_.DeInit();
    switch_.DeInit();
    EventCenter::GetInstance().Unsubscribe(PasteboardEvent::DISCONNECT);
    EventCenter::GetInstance().Unsubscribe(OHOS::MiscServices::Event::EVT_REMOTE_CHANGE);
    
    dumpManager_.reset();
    CancelCriticalTimer();
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, PASTEBOARD_SERVICE_ID);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnStop End.");
}

void PasteboardService::UpdateAgedTime()
{
    int32_t agedTime = system::GetIntParameter("const.pasteboard.local_data_aging_time", ONE_HOUR_MINUTES,
        MIN_AGED_TIME, MAX_AGED_TIME);
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

bool PasteboardService::HasActivePasteboardWork()
{
    bool hasClip = false;
    clips_.ForEachCopies([&hasClip](const auto &, auto &data) {
        if (data != nullptr) {
            hasClip = true;
            return true;
        }
        return false;
    });
    return hasClip;
}

void PasteboardService::RefreshCriticalState()
{
    bool isCritical = HasActivePasteboardWork();
    Memory::MemMgrClient::GetInstance().SetCritical(getpid(), isCritical, PASTEBOARD_SERVICE_ID);
    isCritical_.store(isCritical);
}

void PasteboardService::SetCriticalTimer()
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ffrtTimer_ != nullptr, PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");

    FFRTTask task = [this] {
        std::thread thread([=]() {
            if (!HasActivePasteboardWork()) {
                Memory::MemMgrClient::GetInstance().SetCritical(getpid(), false, PASTEBOARD_SERVICE_ID);
                isCritical_.store(false);
            }
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "SetCriticalTime");
        thread.detach();
    };

    ffrtTimer_->SetTimer(SET_CRITICAL_ID, task, static_cast<uint32_t>(agedTime_.load()));

    if (!isCritical_.load()) {
        Memory::MemMgrClient::GetInstance().SetCritical(getpid(), true, PASTEBOARD_SERVICE_ID);
        isCritical_.store(true);
    }
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
    reportInfo.timestamp = pasteData.GetProperty().timestamp;
    UE_REPORT(UE_COPY, reportInfo,
        "RECORD_NUM", reportInfo.description.recordNum,
        "DATA_SIZE", reportInfo.commonInfo.dataSize,
        "CURRENT_ACCOUNT_ID", reportInfo.commonInfo.currentAccountId,
        "ENTRY_NUM", reportInfo.description.entryNum,
        "MIMETYPES", reportInfo.description.mimeTypes,
        "DATA_TIMESTAMP", reportInfo.timestamp);
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
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = clips_.Find(GetCompositeKey(userId));
#else
    auto [hasData, data] = clips_.Find(GetCompositeKey(userId));
#endif
    if (hasData) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearInner: found data for userId=%{public}d, erasing", userId);
        clips_.Erase(GetCompositeKey(userId));
        delayDataId_ = 0;
        delayTokenId_ = 0;
    }
    distributedManager_->CleanDistributedData(userId);
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

void PasteboardService::NotifyEntityObservers(std::string &entity, EntityType entityType, uint32_t dataLength)
{
    observerManager_->NotifyEntityObservers(entity, entityType, dataLength);
}

int32_t PasteboardService::SubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver> &observer)
{
    return observerManager_->SubscribeEntityObserver(entityType, expectedDataLength, observer);
}

int32_t PasteboardService::UnsubscribeEntityObserver(
    EntityType entityType, uint32_t expectedDataLength, const sptr<IEntityRecognitionObserver> &observer)
{
    return observerManager_->UnsubscribeEntityObserver(entityType, expectedDataLength, observer);
}

void PasteboardService::UnsubscribeAllEntityObserver()
{
    observerManager_->UnsubscribeAllEntityObserver();
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
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = clips_.Find(GetCompositeKey(appInfo.userId));
#else
    auto [hasData, data] = clips_.Find(GetCompositeKey(appInfo.userId));
#endif
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
        int32_t ret = delayDataHandler_->GetRemoteEntryValue(appInfo, *data, *record, value);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "get remote entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    int32_t ret = delayDataHandler_->GetLocalEntryValue(appInfo.userId, *data, *record, value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);

    std::string mimeType = value.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_HTML) {
        return delayDataHandler_->ProcessDelayHtmlEntry(*data, appInfo, value);
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
    return permissionChecker_->VerifyPermission(tokenId);
}

int32_t PasteboardService::IsDataValid(PasteData &pasteData, uint32_t tokenId, int32_t userId)
{
    return permissionChecker_->IsDataValid(pasteData, tokenId, userId);
}

int32_t PasteboardService::GetSdkVersion(uint32_t tokenId)
{
    return permissionChecker_->GetSdkVersion(tokenId);
}

bool PasteboardService::IsPermissionGranted(const std::string &perm, uint32_t tokenId)
{
    return permissionChecker_->IsPermissionGranted(perm, tokenId);
}

bool PasteboardService::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    return permissionChecker_->IsSystemAppByFullTokenID(tokenId);
}

AppInfo PasteboardService::GetAppInfo(uint32_t tokenId) const
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAppInfo: tokenId=%{public}u", tokenId);
    AppInfo info;
    info.tokenId = tokenId;
    info.tokenType = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (info.tokenType == ATokenTypeEnum::TOKEN_HAP) {
        FillHapAppInfo(tokenId, info);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "GetAppInfo complete: bundleName=%{public}s, userId=%{public}d, tokenType=%{public}d",
            info.bundleName.c_str(), info.userId, info.tokenType);
        return info;
    }
    info.userId = ResolveMainDisplayUserId();
    if (info.tokenType == ATokenTypeEnum::TOKEN_NATIVE || info.tokenType == ATokenTypeEnum::TOKEN_SHELL) {
        FillNativeAppInfo(tokenId, info);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "GetAppInfo complete: bundleName=%{public}s, userId=%{public}d, tokenType=%{public}d",
        info.bundleName.c_str(), info.userId, info.tokenType);
    return info;
}

void PasteboardService::FillHapAppInfo(uint32_t tokenId, AppInfo &info) const
{
    HapTokenInfo hapInfo;
    if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get hap token info fail.");
        info.userId = -1;
        return;
    }
    info.bundleName = hapInfo.bundleName;
    info.appIndex = hapInfo.instIndex;
    info.userId = hapInfo.userID;
}

void PasteboardService::FillNativeAppInfo(uint32_t tokenId, AppInfo &info) const
{
    NativeTokenInfo tokenInfo;
    if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get native token info fail.");
        return;
    }
    info.bundleName = tokenInfo.processName;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "GetAppInfo Native: bundleName=%{public}s, userId=%{public}d", info.bundleName.c_str(), info.userId);
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
    PasteboardDialog::ProgressMessageInfo message;
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
    message.left = appInfo.left;
    message.top = appInfo.top;
    message.width = static_cast<int32_t>(appInfo.width);
    message.height = static_cast<int32_t>(appInfo.height);
    message.callerToken = appInfo.abilityToken;
    message.clientCallback = observer;
    PasteboardDialog::ShowProgress(message);
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
    fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);

    int32_t result = AshmemSetProt(fd, PROT_READ | PROT_WRITE);
    if (result < 0) {
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ashmem set prot failed");
        return false;
    }
    void *ptr = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "mmap failed, fd:%{public}d", fd);
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
        return false;
    }
    if (!messageData.MemcpyData(ptr, static_cast<size_t>(size), data, size)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "memcpy_s failed, fd:%{public}d", fd);
        ::munmap(ptr, size);
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
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
    commonInfo.currentAccountId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
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
    radarReportInfo.pasteInfo.onlineDevNum = DMAdapter::GetInstance().GetDeviceNum();
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
    ueReportInfo.pasteInfo.onlineDevNum = DMAdapter::GetInstance().GetDeviceNum();
    ueReportInfo.description = data.GetReportDescription();
    ueReportInfo.timestamp = data.GetProperty().timestamp;
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
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(fd >= 0, static_cast<int32_t>(PasteboardError::SERIALIZATION_ERROR),
            PASTEBOARD_MODULE_SERVICE, "ashmem create failed");
        fdsan_exchange_owner_tag(fd, 0, PASTEBOARD_FD_TAG);
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
        "MIMETYPES", ueReportInfo.description.mimeTypes,
        "DATA_TIMESTAMP", ueReportInfo.timestamp);
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
    bool isTestServerSetPasteData = developerMode && setPasteDataUId_.load() == TEST_SERVER_UID;
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
        fdsan_exchange_owner_tag(serviceFd, 0, PASTEBOARD_FD_TAG);
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
    const std::string &pasteId, std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    return uriHandler_->CheckAndGrantRemoteUri(data, appInfo, pasteId, pasteBlock);
}

bool PasteboardService::RemoteDataTaskManager::IsRemoteDataPasting(const Event &event)
{
    auto key = event.deviceId + std::to_string(event.seqId);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dataTasks_.find(key);
    if (it == dataTasks_.end() || it->second == nullptr) {
        return false;
    }
    return it->second->pasting_;
}

bool PasteboardService::RemoteDataTaskManager::HasRunningTask()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &[key, task] : dataTasks_) {
        if (task != nullptr && task->pasting_) {
            return true;
        }
    }
    return false;
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
    if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
        auto isPasting = taskMgr_.IsRemoteDataPasting(distEvt);
        if (isPasting) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "wait remote data, seqId=%{public}u", distEvt.seqId);
            taskMgr_.WaitRemoteData(distEvt);
        }
    }
    if (distRet != static_cast<int32_t>(PasteboardError::E_OK) ||
        GetScreenStatus(appInfo.userId) != ScreenEvent::ScreenUnlocked) {
        auto currentEvent = GetCurrentEvent();
        pasteBlock = p2pManager_->EstablishP2PLinkTask(pasteId, currentEvent);
        result = GetLocalData(appInfo, data);
        if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
            peerNetId = currentEvent.deviceId;
            peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
        }
    } else {
        pasteBlock = p2pManager_->EstablishP2PLinkTask(pasteId, distEvt);
        result = GetRemoteData(appInfo.userId, distEvt, data, syncTime);
        if (result == static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED)) {
            HandleGetDataError(result, pasteBlock, distEvt.deviceId, pasteId);
            result = GetLocalData(appInfo, data);
        } else {
            peerNetId = distEvt.deviceId;
            peerUdid = DMAdapter::GetInstance().GetUdidByNetworkId(peerNetId);
        }
    }
    HandleNotificationsAndStatusChecks(appInfo, data, peerNetId, isPeerOnline);
    PublishServiceState(data, syncTime, peerNetId, pasteBlock);
    
    if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
        HandleGetDataError(result, pasteBlock, distEvt.deviceId, pasteId);
        return result;
    }
    return CheckAndGrantRemoteUri(data, appInfo, pasteId, pasteBlock);
}

void PasteboardService::HandleNotificationsAndStatusChecks(const AppInfo &appInfo, const PasteData &data,
    const std::string &peerNetId, bool &isPeerOnline)
{
    if (observerManager_->HasEventObservers()) {
        std::string targetBundleName = GetAppBundleName(appInfo);
        observerManager_->NotifyObservers(targetBundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_READ);
    }
    if (!peerNetId.empty()) {
        isPeerOnline = DMAdapter::GetInstance().IsDeviceOnline(peerNetId);
    }
}

void PasteboardService::PublishServiceState(const PasteData &data, int32_t syncTime,
    const std::string &peerNetId, std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    auto plugin = distributedManager_->GetClipPlugin();
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
        #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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
        auto result = distributedManager_->GetDistributedData(event, userId);
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        std::shared_ptr<PasteDateTime> pasteDataTime = std::make_shared<PasteDateTime>();
        if (result.first != nullptr) {
            result.first->SetRemote(true);
            if (distEvt == event) {
                #ifdef PB_COCKPIT_PLATFORM_ENABLE
                clips_.InsertOrAssign(GetCompositeKey(userId), result.first);
#else
                #ifdef PB_COCKPIT_PLATFORM_ENABLE
    clips_.InsertOrAssign(GetCompositeKey(userId), result.first);
#else
    clips_.InsertOrAssign(GetCompositeKey(userId), result.first);
#endif
#endif
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
    PasteBoardCommonUtils::SetThreadTaskName(thread, "GetRemotePaste");
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
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(appInfo.userId));
#else
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(appInfo.userId));
#else
    auto it = clips_.Find(GetCompositeKey(appInfo.userId));
#endif
#endif
    auto tempTime = copyTime_.Find(appInfo.userId);
    if (!it.first || !tempTime.first) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "no data userId is %{public}d.", appInfo.userId);
        return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
    }
    auto ret = IsDataValid(*(it.second), appInfo.tokenId, appInfo.userId);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "paste data is invalid. ret = %{public}d "
            "appInfo.userId = %{public}d", ret, appInfo.userId);
        return ret;
    }
    data = *(it.second);
    auto originBundleName = it.second->GetBundleName();
    if (it.second->IsDelayData()) {
        delayDataHandler_->GetDelayPasteData(appInfo.userId, data);
        RADAR_REPORT(DFX_GET_PASTEBOARD, DFX_CHECK_GET_DELAY_PASTE, DFX_SUCCESS, CONCURRENT_ID, pasteId);
    }
    if (it.second->IsDelayRecord()) {
        delayDataHandler_->GetDelayPasteRecord(appInfo.userId, data);
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
        #ifdef PB_COCKPIT_PLATFORM_ENABLE
        clips_.ComputeIfPresent(GetCompositeKey(appInfo.userId), [&data, &isNotify](auto &key, auto &value) {
#else
        clips_.ComputeIfPresent(GetCompositeKey(appInfo.userId), [&data, &isNotify](auto &key, auto &value) {
#endif
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
    return p2pManager_->OnPasteComplete(deviceId, pasteId);
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

void PasteboardService::RemoveInvalidRemoteUri(std::vector<Uri> &grantUris)
{
    uriHandler_->RemoveInvalidRemoteUri(grantUris);
}

int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId)
{
    return uriHandler_->GrantPermission(grantUris, permFlag, isRemoteData, targetTokenId);
}

int32_t PasteboardService::GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, bool isRemoteData)
{
    return uriHandler_->GrantUriPermission(grantUris, targetTokenId, isRemoteData);
}

std::map<uint32_t, std::vector<Uri>> PasteboardService::CheckUriPermission(PasteData &data,
    const std::pair<std::string, int32_t> &targetBundleAndIndex)
{
    return uriHandler_->CheckUriPermission(data, targetBundleAndIndex);
}

bool PasteboardService::IsBundleOwnUriPermission(const std::string &bundleName, Uri &uri)
{
    return uriHandler_->IsBundleOwnUriPermission(bundleName, uri);
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

    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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

bool PasteboardService::HasRemoteUri(std::shared_ptr<PasteData> data)
{
    return uriHandler_->HasRemoteUri(data);
}

int32_t PasteboardService::HasRemoteData(bool &funcResult)
{
    funcResult = HasRemoteData();
    return ERR_OK;
}

bool PasteboardService::HasRemoteData()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(GetScreenStatus(appInfo.userId) == ScreenEvent::ScreenUnlocked, false,
        PASTEBOARD_MODULE_SERVICE, "screen is locked.");
    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
    if (distRet == static_cast<int32_t>(PasteboardError::E_OK)) {
        return true;
    }
    if (distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA)) {
        auto isPasting = taskMgr_.IsRemoteDataPasting(distEvt);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!isPasting, true, PASTEBOARD_MODULE_SERVICE, "remote data is pasting.");
    }
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = clips_.Find(GetCompositeKey(appInfo.userId));
#else
    auto [hasData, data] = clips_.Find(GetCompositeKey(appInfo.userId));
#endif
    if (!hasData || data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "local data is null");
        return false;
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(data->IsRemote(), false,
        PASTEBOARD_MODULE_SERVICE, "not contains remote data.");
    bool hasRemoteUri = HasRemoteUri(data);
    return hasRemoteUri;
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
    pasteData.deviceId_ = DMAdapter::GetInstance().GetLocalNetworkId();
    SetPasteDataInfo(pasteData, appInfo);
    auto authority = std::make_pair(appInfo.bundleName, appInfo.appIndex);
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authority);
    bool hasSplited = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex,
        appInfo.userId);
    PasteboardWebController::GetInstance().SetWebviewPasteData(pasteData, bundleIndex);
    PasteboardWebController::GetInstance().CheckAppUriPermission(pasteData);
    if (hasSplited || dataSize > static_cast<int64_t>(maxLocalCapacity_.load() * RECALCULATE_DATA_SIZE)) {
        int64_t newDataSize = static_cast<int64_t>(pasteData.Count());
        if (newDataSize > maxLocalCapacity_.load()) {
            setting_.store(false);
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid data size, dataSize=%{public}" PRId64, newDataSize);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
        }
        pasteData.rawDataSize_ = newDataSize;
    }
    setPasteDataUId_.store(IPCSkeleton::GetCallingUid());
    RemovePasteData(appInfo);
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    clips_.InsertOrAssign(GetCompositeKey(appInfo.userId), std::make_shared<PasteData>(pasteData));
#else
    clips_.InsertOrAssign(GetCompositeKey(appInfo.userId), std::make_shared<PasteData>(pasteData));
#endif
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
        distributedManager_->SetDistributedData(appInfo.userId, pasteData);
        NotifyObservers(appInfo.bundleName, appInfo.userId, PasteboardEventStatus::PASTEBOARD_WRITE);
    }
    SetPasteDataDot(pasteData, appInfo.userId);
    setting_.store(false);
    SubscribeKeyboardEvent();
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardService::ClearAgedData(int32_t userId)
{
    auto data = clips_.Find(GetCompositeKey(userId));
    if (data.first) {
        clips_.Erase(GetCompositeKey(userId));
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

void PasteboardService::SetPasteDataInfo(PasteData &pasteData, const AppInfo &appInfo)
{
    pasteData.SetBundleInfo(appInfo.bundleName, appInfo.appIndex);
    pasteData.SetOriginAuthority(std::make_pair(appInfo.bundleName, appInfo.appIndex));
    pasteData.SetTime(GetTime());
    pasteData.SetScreenStatus(GetScreenStatus(appInfo.userId));
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
    if (mimeType == MIMETYPE_TEXT_HTML || mimeType == MIMETYPE_TEXT_PLAIN || mimeType == MIMETYPE_TEXT_URI ||
        mimeType == MIMETYPE_PIXELMAP || mimeType == MIMETYPE_AUTOFILL_SECURE) {
        return true;
    }
    return false;
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
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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
    std::shared_ptr<ClipPlugin> plugin = nullptr;
    {
        std::lock_guard<decltype(mutex)> lockGuard(mutex);
        plugin = clipPlugin_;
    }
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is null");
        return std::make_pair(static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL), evt);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(plugin->IsWiFiEnable(),
        std::make_pair(static_cast<int32_t>(PasteboardError::GET_LOCAL_DATA), evt), PASTEBOARD_MODULE_SERVICE,
        "wifi is disabled");
    auto events = plugin->GetTopEvents(1, user);
    if (events.empty()) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "plugin event is empty");
        return std::make_pair(static_cast<int32_t>(PasteboardError::PLUGIN_EVENT_EMPTY), evt);
    }
    evt = events[0];
    auto currentEvent = GetCurrentEvent();
    if (evt.deviceId == DMAdapter::GetInstance().GetLocalNetworkId() || evt.expiration < currentEvent.expiration) {
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

    if (evt.deviceId == currentEvent.deviceId && evt.seqId == currentEvent.seqId &&
        evt.expiration == currentEvent.expiration) {
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

int32_t PasteboardService::GetRemoteMimeTypes(std::vector<std::string> &mimeTypes, const Event &event)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "version=%{public}d, get remote mimeTypes", event.version);
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        return static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL);
    }
    std::vector<uint8_t> rawData;
    auto result = clipPlugin->GetMimeTypes(rawData, event);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result == static_cast<int32_t>(PasteboardError::E_OK),
        result, PASTEBOARD_MODULE_SERVICE, "get mimeTypes from plugin failed, result=%{public}d.", result);
    if (event.version == ClipPlugin::InfoType::MIMETYPE) {
        mimeTypes = DecodeMimeTypes(rawData);
    } else {
        PasteData pasteData;
        pasteData.Decode(rawData);
        mimeTypes = pasteData.GetMimeTypes();
    }
    return ERR_OK;
}

bool PasteboardService::HasRemoteDataType(const std::string &mimeType, const Event &event)
{
    std::vector<std::string> mimeTypes;
    if (GetRemoteMimeTypes(mimeTypes, event) != ERR_OK) {
        return false;
    }
    return std::find(mimeTypes.begin(), mimeTypes.end(), mimeType) != mimeTypes.end();
}

std::vector<std::string> PasteboardService::GetLocalMimeTypes()
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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

int32_t PasteboardService::IsRemoteData(bool &funcResult)
{
    funcResult = IsRemoteData();
    return ERR_OK;
}

bool PasteboardService::IsRemoteData()
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId is error");
        return false;
    }
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
    if (!it.first) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        return distRet == static_cast<int32_t>(PasteboardError::E_OK);
    }
    return it.second->IsRemote();
}

int32_t PasteboardService::GetDataSource(std::string &bundleName)
{
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto it = clips_.Find(GetCompositeKey(userId));
#else
    auto it = clips_.Find(GetCompositeKey(userId));
#endif
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
        fdsan_close_with_tag(fd, PASTEBOARD_FD_TAG);
    }
}

int32_t PasteboardService::WritePasteData(
    int fd, int64_t rawDataSize, const std::vector<uint8_t> &buffer, PasteData &pasteData, bool &hasData)
{
    if (rawDataSize > MIN_ASHMEM_DATA_SIZE) {
        auto actualSize = AshmemGetSize(fd);
        if (actualSize < 0 || rawDataSize > actualSize) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "rawDataSize invalid, actualSize=%{public}d, rawDataSize:%{public}" PRId64, actualSize, rawDataSize);
            CloseSharedMemFd(fd);
            return static_cast<int32_t>(PasteboardError::INVALID_DATA_SIZE);
        }
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
    return observerManager_->SubscribeDisposableObserver(observer, targetWindowId, type, maxLength);
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
    if (observerManager_->HasEntityObservers() && pasteData.HasMimeType(MIMETYPE_TEXT_PLAIN)) {
        entityRecognizer_->RecognizePasteData(pasteData);
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

int32_t PasteboardService::GetCurrentAccountId() const
{
    if (userContextResolver_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "resolver is null.");
        return ERROR_USERID;
    }
    auto context = userContextResolver_->ResolveCallingUser();
    int32_t userId = context.isValid ? context.userId : ERROR_USERID;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetCurrentAccountId: return userId=%{public}d, isValid=%{public}d",
        userId, context.isValid);
    return userId;
}

UserContext PasteboardService::ResolveEventUser(const EventFwk::CommonEventData &data) const
{
    if (userContextResolver_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "resolver is null.");
        return {};
    }
    return userContextResolver_->ResolveEventUser(data);
}

UserContext PasteboardService::ResolveUserIdFromWant(const AAFwk::Want &want) const
{
    if (userContextResolver_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "resolver is null.");
        return {};
    }
    return userContextResolver_->ResolveUserIdFromWant(want);
}

std::vector<UserContext> PasteboardService::ResolveForegroundUsers() const
{
    if (userContextResolver_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "resolver is null.");
        return {};
    }
    return userContextResolver_->ResolveForegroundUsers();
}

int32_t PasteboardService::ResolveMainDisplayUserId() const
{
    if (userContextResolver_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "resolver is null.");
        return ERROR_USERID;
    }
    auto context = userContextResolver_->ResolveMainDisplayUser();
    int32_t userId = context.isValid ? context.userId : ERROR_USERID;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "ResolveMainDisplayUserId: return userId=%{public}d, isValid=%{public}d", userId, context.isValid);
    return userId;
}

int32_t PasteboardService::ClearByEventUser(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter, clips_.Size=%{public}zu, userId=%{public}d",
        clips_.Size(), userId);
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR);
    }
    AppInfo appInfo;
    appInfo.bundleName = PASTEBOARD_SERVICE_NAME;
    appInfo.tokenType = ATokenTypeEnum::TOKEN_NATIVE;
    appInfo.userId = userId;
    appInfo.tokenId = IPCSkeleton::GetSelfTokenID();
    return ClearInner(userId, appInfo);
}

void PasteboardService::ClearByResolvedUser(int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByResolvedUser: userId=%{public}d", userId);
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clear resolved user failed, userId invalid");
        return;
    }
    AppInfo appInfo;
    appInfo.userId = userId;
    appInfo.bundleName = PASTEBOARD_SERVICE_NAME;
    appInfo.tokenType = ATokenTypeEnum::TOKEN_NATIVE;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByResolvedUser: calling ClearInner for userId=%{public}d",
        userId);
    ClearInner(userId, appInfo);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByResolvedUser completed: userId=%{public}d", userId);
}

ScreenEvent PasteboardService::GetScreenStatus(int32_t userId)
{
    return systemEventListener_->GetScreenStatus(userId);
}

bool PasteboardService::IsCopyable(uint32_t tokenId) const
{
    return permissionChecker_->IsCopyable(tokenId);
}

void PasteboardService::SetInputMethodPid(int32_t userId, pid_t callPid)
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return;
    }
    auto isImePid = imc->IsCurrentImeByPid(callPid, userId);
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
    return observerManager_->SubscribeObserver(type, observer);
}

int32_t PasteboardService::ResubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer)
{
    return observerManager_->ResubscribeObserver(type, observer);
}

int32_t PasteboardService::UnsubscribeObserver(
    PasteboardObserverType type, const sptr<IPasteboardChangedObserver> &observer)
{
    return observerManager_->UnsubscribeObserver(type, observer);
}

int32_t PasteboardService::UnsubscribeAllObserver(PasteboardObserverType type)
{
    return observerManager_->UnsubscribeAllObserver(type);
}



int32_t PasteboardService::SetGlobalShareOption(const std::unordered_map<uint32_t, int32_t> &globalShareOptions)
{
    return permissionChecker_->SetGlobalShareOption(globalShareOptions);
}

int32_t PasteboardService::RemoveGlobalShareOption(const std::vector<uint32_t> &tokenIds)
{
    return permissionChecker_->RemoveGlobalShareOption(tokenIds);
}

int32_t PasteboardService::GetGlobalShareOption(const std::vector<uint32_t> &tokenIds,
    std::unordered_map<uint32_t, int32_t>& funcResult)
{
    return permissionChecker_->GetGlobalShareOption(tokenIds, funcResult);
}

int32_t PasteboardService::SetAppShareOptions(int32_t shareOptions)
{
    return permissionChecker_->SetAppShareOptions(shareOptions);
}

int32_t PasteboardService::RemoveAppShareOptions()
{
    return permissionChecker_->RemoveAppShareOptions();
}

void PasteboardService::ThawInputMethod(pid_t imePid)
{
    auto type = ResourceSchedule::ResType::RES_TYPE_SA_CONTROL_APP_EVENT;
    auto statusStart = ResourceSchedule::ResType::SaControlAppStatus::SA_START_APP;
    auto statusStop = ResourceSchedule::ResType::SaControlAppStatus::SA_STOP_APP;

    std::unordered_map<std::string, std::string> payload = {
        { "saId", std::to_string(PASTEBOARD_SERVICE_ID) },
        { "saName", PASTEBOARD_SERVICE_SA_NAME },
        { "extensionType", std::to_string(static_cast<int32_t>(AppExecFwk::ExtensionAbilityType::INPUTMETHOD)) },
        { "pid", std::to_string(imePid) } };
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "report RSS need thaw:pid = %{public}d", imePid);
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, statusStart, payload);
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, statusStop, payload); // will stop after 6s
}

bool PasteboardService::IsNeedThaw(PasteboardEventStatus status)
{
    if (status == PasteboardEventStatus::PASTEBOARD_READ) {
        return false;
    }
    int32_t userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return false;
    }
    std::shared_ptr<Property> property;
    int32_t ret = imc->GetDefaultInputMethod(property, userId);
    if (ret != ErrorCode::NO_ERROR || property == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "default input method is nullptr!");
        return false;
    }
    return true;
}

void PasteboardService::NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    observerManager_->NotifyObservers(bundleName, userId, status);
}

bool PasteboardService::SetPasteboardHistory(HistoryInfo &info)
{
    return dumpManager_->SetPasteboardHistory(info);
}

int PasteboardService::Dump(int fd, const std::vector<std::u16string> &args)
{
    if (fd < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid fd: %{public}d", fd);
        return 0;
    }
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
    return PasteboardDumpManager::GetTime();
}

std::string PasteboardService::DumpUserHistory(int32_t userId) const
{
    return dumpManager_->DumpUserHistory(userId);
}

std::string PasteboardService::DumpHistory() const
{
    return dumpManager_->DumpHistory();
}

std::string PasteboardService::DumpUserData(int32_t userId)
{
    return dumpManager_->DumpUserData(userId);
}

std::string PasteboardService::DumpData()
{
    return dumpManager_->DumpData();
}

bool PasteboardService::IsFocusedApp(uint32_t tokenId)
{
    return appInfoHelper_->IsFocusedApp(tokenId);
}

FocusedAppInfo PasteboardService::GetFocusedAppInfo() const
{
    return appInfoHelper_->GetFocusedAppInfo();
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

void PasteboardService::GenerateDistributedUri(PasteData &data)
{
    uriHandler_->GenerateDistributedUri(data);
}

void PasteboardService::OnConfigChange(bool isOn)
{
    systemEventListener_->OnConfigChange(isOn);
}

std::string PasteboardService::GetAppLabel(uint32_t tokenId)
{
    return appInfoHelper_->GetAppLabel(tokenId);
}

sptr<AppExecFwk::IBundleMgr> PasteboardService::GetAppBundleManager()
{
    return appInfoHelper_->GetAppBundleManager();
}

ClipPlugin::GlobalEvent PasteboardService::GetCurrentEvent() const
{
    std::lock_guard<std::mutex> lock(currentEventMutex_);
    return currentEvent_;
}

void PasteboardService::SetCurrentEvent(ClipPlugin::GlobalEvent event)
{
    std::lock_guard<std::mutex> lock(currentEventMutex_);
    currentEvent_ = std::move(event);
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
        pasteboardService_->distributedManager_->ChangeStoreStatus(context.userId);
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
    uriHandler_->ClearUriOnUninstall(userId, tokenId);
}

void PasteboardService::ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData)
{
    uriHandler_->ClearUriOnUninstall(pasteData);
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
        pasteboardService_->distributedManager_->CloseDistributedStore(data.fromId, true);
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

#ifdef PB_COCKPIT_PLATFORM_ENABLE
int32_t PasteboardService::GetActiveSubspaceId(int32_t userId) const
{
    auto subspaceId = activeSubspaceMap_.Find(userId);
    if (subspaceId.first) {
        return subspaceId.second;
    }
    return DEFAULT_SUBSPACE_ID;
}

CompositeKey PasteboardService::GetCompositeKey(int32_t userId) const
{
    return CompositeKey(userId, GetActiveSubspaceId(userId));
}

void PasteboardService::UpdateActiveSubspace(int32_t userId, int32_t subspaceId)
{
    activeSubspaceMap_.Insert(userId, subspaceId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "UpdateActiveSubspace: userId=%{public}d, subspaceId=%{public}d",
        userId, subspaceId);
}
#endif

void PasteboardService::RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap &observerMap)
{
    observerManager_->RemoveObserverByPid(userId, pid, observerMap);
}

int32_t PasteboardService::AppExit(pid_t pid, int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid %{public}d exit, userId %{public}d.", pid, userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE, "invalid userId");
    observerManager_->RemoveAllObserversByPid(userId, pid);
    observerManager_->RemoveEntityObserverByPid(pid);
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);
    ClearInputMethodPidByPid(userId, pid);
    std::vector<std::string> networkIds;
    p2pManager_->RemoveP2PLinksByPid(pid, networkIds);
    for (const auto &id : networkIds) {
        p2pManager_->CloseP2PLink(id);
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
    service_.AppExit(pid_, userId_);
}

PasteboardService::PasteboardDeathRecipient::PasteboardDeathRecipient(
    PasteboardService &service, pid_t pid, int32_t userId) : service_(service), pid_(pid), userId_(userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "Construct Pasteboard Client Death Recipient, pid: %{public}d, userId: %{public}d", pid, userId);
}

int32_t PasteboardService::RegisterClientDeathObserver(const sptr<IRemoteObject> &observer)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    int32_t userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE, "invalid userId");
    sptr<PasteboardDeathRecipient> deathRecipient = sptr<PasteboardDeathRecipient>::MakeSptr(*this, pid, userId);
    observer->AddDeathRecipient(deathRecipient);
    clients_.InsertOrAssign(pid, std::make_pair(observer, deathRecipient));
    return ERR_OK;
}

int32_t PasteboardService::DetachPasteboard()
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    return AppExit(pid, GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId);
}

std::function<void(const OHOS::MiscServices::Event &)> PasteboardService::RemotePasteboardChange()
{
    return [this](const OHOS::MiscServices::Event &event) {
        (void)event;
        observerManager_->NotifyRemoteObservers();
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

std::vector<uint8_t> PasteboardService::EncodeMimeTypes(const std::vector<std::string> &mimeTypes)
{
    std::vector<uint8_t> result;
    result.reserve(MAX_TRANSFER_SIZE);
    for (const auto &mimeType : mimeTypes) {
        auto len = mimeType.size();
        if (len > UINT16_MAX) {
            continue;
        }
        uint16_t strLen = static_cast<uint16_t>(len);
        if (result.size() + strLen + 2 > MAX_TRANSFER_SIZE) {
            break;
        }
        result.emplace_back(static_cast<uint8_t>(len & 0xFF));
        result.emplace_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        const uint8_t *data = reinterpret_cast<const uint8_t *>(mimeType.data());
        result.insert(result.end(), data, data + strLen);
    }
    result.shrink_to_fit();
    return result;
}

std::vector<std::string> PasteboardService::DecodeMimeTypes(const std::vector<uint8_t> &rawData)
{
    std::vector<std::string> mimeTypes;
    const uint8_t *data = rawData.data();
    size_t size = rawData.size();
    size_t index = 0;
    while (index + 2 <= size) {
        uint16_t len = static_cast<uint16_t>(data[index]) | (static_cast<uint16_t>(data[index + 1]) << 8);
        index += 2;
        if (index + len > size) {
            break;
        }
        mimeTypes.emplace_back(reinterpret_cast<const char *>(data + index), len);
        index += len;
    }
    return mimeTypes;
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
