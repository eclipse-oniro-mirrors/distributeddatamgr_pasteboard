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
constexpr const char *PASTEBOARD_SERVICE_SA_NAME = "pasteboard_service";
constexpr const char *PASTEBOARD_SERVICE_NAME = "PasteboardService";
} // namespace
using namespace Security::AccessToken;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
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

bool PasteboardService::IsDataAged(int32_t userId)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "IsDataAged start");
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
    }
    bool isFocused = false;
    int32_t ret = PasteboardAbilityManager::CheckUIExtensionIsFocused(tokenId, displayId, isFocused);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "check result:%{public}d, isFocused:%{public}d", ret, isFocused);
    return ret == NO_ERROR && isFocused;
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
} // namespace MiscServices
} // namespace OHOS
