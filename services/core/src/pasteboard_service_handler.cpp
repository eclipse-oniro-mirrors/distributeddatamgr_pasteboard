/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
constexpr int32_t COMMON_USERID = 0;
constexpr const char *PASTEBOARD_SERVICE_SA_NAME = "pasteboard_service";
constexpr const char *PASTEBOARD_SERVICE_NAME = "PasteboardService";
constexpr const char *FAIL_TO_GET_TIME_STAMP = "FAIL_TO_GET_TIME_STAMP";
constexpr const char *MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION =
    "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION";
constexpr const char *COVER_DELAY_DATA = "COVER_DELAY_DATA";
constexpr int32_t WIFI_DISABLED = 1;
constexpr int32_t CTRLV_EVENT_SIZE = 2;
constexpr uint32_t EVENT_TIME_OUT = 2000;
constexpr uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);
constexpr uint32_t MAX_BUNDLE_NAME_LENGTH = 127;
constexpr int64_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
constexpr int32_t SET_VALUE_SUCCESS = 1;
constexpr uint16_t MAX_TRANSFER_SIZE = 1300;
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;

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
    auto [found, status] = screenStatusMap_.Find(userId);
    if (found) {
        return status;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "screen status not found for userId=%{public}d", userId);
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
    int32_t userId = isEventType ? COMMON_USERID : GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
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
    if (callingUid == EDM_UID || (uid_ != -1 && callingUid == uid_) || callingUid == RSS_UID) {
        return true;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "callingUid error: %{public}d.", callingUid);
    return false;
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
    auto [hasPid, pid] = imeMap_.Find(userId);
    if (hasPid && IsNeedThaw(status)) {
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
    PasteBoardCommonUtils::SetThreadTaskName(thread, "NotifyObservers");
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

std::string PasteboardService::DumpUserHistory(int32_t userId) const
{
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID, "Access history fail! invalid userId.",
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::string result;
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

std::string PasteboardService::DumpHistory() const
{
    auto foregroundUsers = ResolveForegroundUsers();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!foregroundUsers.empty(), "Access history fail! no foreground user.",
        PASTEBOARD_MODULE_SERVICE, "no foreground user");
    std::string result;
    for (const auto &ctx : foregroundUsers) {
        if (ctx.userId == ERROR_USERID) {
            continue;
        }
        result += "UserId: " + std::to_string(ctx.userId) + "\n";
        result += DumpUserHistory(ctx.userId);
    }
    return result;
}

std::string PasteboardService::DumpUserData(int32_t userId)
{
    auto it = clips_.Find(userId);
    if (!it.first || it.second == nullptr) {
        return "No copy data.\n";
    }
    size_t recordCounts = it.second->GetRecordCount();
    auto property = it.second->GetProperty();
    std::string shareOption;
    PasteData::ShareOptionToString(property.shareOption, shareOption);
    std::string sourceDevice = property.isRemote ? "remote" : "local";
    std::string result;
    result.append("|Owner       :  ").append(property.bundleName).append("\n")
        .append("|Timestamp   :  ").append(property.setTime).append("\n")
        .append("|Share Option:  ").append(shareOption).append("\n")
        .append("|Record Count:  ").append(std::to_string(recordCounts)).append("\n")
        .append("|Mime types  :  {");
    if (!property.mimeTypes.empty()) {
        for (size_t i = 0; i < property.mimeTypes.size(); ++i) {
            result.append(property.mimeTypes[i]).append(",");
        }
    }
    result.append("}").append("\n").append("|source device:  ").append(sourceDevice);
    return result;
}

std::string PasteboardService::DumpData()
{
    auto foregroundUsers = ResolveForegroundUsers();
    if (foregroundUsers.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query foreground users failed.");
        return "";
    }
    std::string result;
    for (const auto &ctx : foregroundUsers) {
        if (ctx.userId == ERROR_USERID) {
            continue;
        }
        result += "UserId: " + std::to_string(ctx.userId) + "\n";
        result += DumpUserData(ctx.userId);
    }
    return result;
}

bool PasteboardService::IsFocusedApp(uint32_t tokenId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "caller is not application");
        return true;
    }
    int32_t userId = GetAppInfo(tokenId).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return false;
    }
    FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
#endif
    auto callPid = IPCSkeleton::GetCallingPid();
    if (callPid == info.pid_) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pid is same, it is focused app");
        return true;
    }
    uint64_t displayId = 0;
    auto dispRet = AccountSA::OsAccountManager::GetForegroundOsAccountDisplayId(userId, displayId);
    if (dispRet != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get foreground display id failed, ret=%{public}d", dispRet);
        return false;
    }
    bool isFocused = false;
    int32_t ret = PasteboardAbilityManager::CheckUIExtensionIsFocused(tokenId, displayId, isFocused);
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
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete03");
        thread.detach();
    };
    ffrtTimer_->SetTimer(taskName, p2pTask, PRE_ESTABLISH_P2P_LINK_TIME);
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
    auto status = DistributedFileDaemonManager::GetInstance().ConnectDfs(networkId);
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
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PreEstablishP2P");
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

FocusedAppInfo PasteboardService::GetFocusedAppInfo() const
{
    FocusedAppInfo appInfo = { 0 };
    int32_t userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return appInfo;
    }
    FocusChangeInfo info;
    std::vector<sptr<WindowVisibilityInfo>> windowVisibilityInfos;
    WMError result = WMError::WM_OK;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance(userId).GetFocusWindowInfo(info);
    result = WindowManagerLite::GetInstance(userId).GetVisibilityWindowInfo(windowVisibilityInfos);
#else
    WindowManager::GetInstance(userId).GetFocusWindowInfo(info);
    result = WindowManager::GetInstance(userId).GetVisibilityWindowInfo(windowVisibilityInfos);
#endif
    if (result == WMError::WM_OK) {
        for (const auto& windowInfo : windowVisibilityInfos) {
            if (windowInfo == nullptr) {
                continue;
            }
            if (windowInfo->windowId_ == static_cast<uint32_t>(info.windowId_)) {
                appInfo.left = windowInfo->rect_.posX_;
                appInfo.top = windowInfo->rect_.posY_;
                appInfo.width = windowInfo->rect_.width_;
                appInfo.height = windowInfo->rect_.height_;
                break;
            }
        }
    }
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

std::string PasteboardService::GetAppLabel(uint32_t tokenId)
{
    auto iBundleMgr = GetAppBundleManager();
    if (iBundleMgr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to cast bundle mgr service.");
        return PasteboardDialog::DEFAULT_LABEL;
    }
    AppInfo info = GetAppInfo(tokenId);
    AppExecFwk::ApplicationInfo appInfo;
    auto result = iBundleMgr->GetApplicationInfo(info.bundleName, 0, info.userId, appInfo);
    if (!result) {
        return PasteboardDialog::DEFAULT_LABEL;
    }
    auto &resource = appInfo.labelResource;
    auto label = iBundleMgr->GetStringById(resource.bundleName, resource.moduleName, resource.id, info.userId);
    return label.empty() ? PasteboardDialog::DEFAULT_LABEL : label;
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
    PasteboardService::currentUserId_.store(userId);
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->ChangeStoreStatus(userId);
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

void PasteboardService::RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap &observerMap)
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callObserverKey = std::make_pair(userId, pid);
    auto it = observerMap.find(callObserverKey);
    if (it == observerMap.end()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
            "RemoveObserverByPid: no observer found for userId=%{public}d, pid=%{public}d", userId, pid);
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "RemoveObserverByPid: removing observer for userId=%{public}d, pid=%{public}d", userId, pid);
    observerMap.erase(callObserverKey);
}

int32_t PasteboardService::AppExit(pid_t pid, int32_t userId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid %{public}d exit, userId %{public}d.", pid, userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID,
        static_cast<int32_t>(PasteboardError::INVALID_USERID_ERROR), PASTEBOARD_MODULE_SERVICE, "invalid userId");
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
            pidMap.EraseIf([pid, this, networkId](const auto &key, const auto &value) {
                if (value.callPid == pid) {
                    PasteStart(networkId);
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
