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

#include "pasteboard_app_info_helper.h"
#include "pasteboard_hilog.h"
#include "pasteboard_dialog.h"

namespace OHOS {
namespace MiscServices {

PasteboardAppInfoHelper::PasteboardAppInfoHelper(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardAppInfoHelper constructed.");
}

PasteboardAppInfoHelper::~PasteboardAppInfoHelper()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardAppInfoHelper destructed.");
}

AppInfo PasteboardAppInfoHelper::GetAppInfo(uint32_t tokenId) const
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
    info.userId = service_.ResolveMainDisplayUserId();
    if (info.tokenType == ATokenTypeEnum::TOKEN_NATIVE || info.tokenType == ATokenTypeEnum::TOKEN_SHELL) {
        FillNativeAppInfo(tokenId, info);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "GetAppInfo complete: bundleName=%{public}s, userId=%{public}d, tokenType=%{public}d",
        info.bundleName.c_str(), info.userId, info.tokenType);
    return info;
}

void PasteboardAppInfoHelper::FillHapAppInfo(uint32_t tokenId, AppInfo& info) const
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

void PasteboardAppInfoHelper::FillNativeAppInfo(uint32_t tokenId, AppInfo& info) const
{
    NativeTokenInfo tokenInfo;
    if (AccessTokenKit::GetNativeTokenInfo(tokenId, tokenInfo) != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get native token info fail.");
        return;
    }
    info.bundleName = tokenInfo.processName;
}

std::string PasteboardAppInfoHelper::GetAppBundleName(const AppInfo& appInfo)
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

std::string PasteboardAppInfoHelper::GetAppLabel(uint32_t tokenId)
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
    auto& resource = appInfo.labelResource;
    auto label = iBundleMgr->GetStringById(resource.bundleName, resource.moduleName, resource.id, info.userId);
    return label.empty() ? PasteboardDialog::DEFAULT_LABEL : label;
}

sptr<AppExecFwk::IBundleMgr> PasteboardAppInfoHelper::GetAppBundleManager()
{
    std::lock_guard<std::mutex> lock(bundleMutex_);
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

bool PasteboardAppInfoHelper::IsFocusedApp(uint32_t tokenId)
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

FocusedAppInfo PasteboardAppInfoHelper::GetFocusedAppInfo() const
{
    FocusedAppInfo appInfo = { 0 };
    int32_t userId = service_.GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
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

void PasteboardAppInfoHelper::SetLocalPasteFlag(bool isCrossPaste, uint32_t tokenId, PasteData& pasteData)
{
    pasteData.SetLocalPasteFlag(!isCrossPaste && tokenId == pasteData.GetTokenId());
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "isLocalPaste = %{public}d.", pasteData.IsLocalPaste());
}

} // namespace MiscServices
} // namespace OHOS