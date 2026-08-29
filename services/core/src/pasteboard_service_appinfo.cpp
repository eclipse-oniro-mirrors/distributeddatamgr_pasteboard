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

#ifdef WITH_DLP
#include "dlp_permission_kit.h"
#include "dlp_permission.h"
#endif // WITH_DLP
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "pasteboard_dialog.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
using namespace Security::AccessToken;
namespace {
constexpr int32_t INVALID_VERSION = -1;
} // namespace

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

bool PasteboardService::IsCallerUidValid()
{
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid == EDM_UID || (uid_ != -1 && callingUid == uid_) || callingUid == RSS_UID) {
        return true;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "callingUid error: %{public}d.", callingUid);
    return false;
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
} // namespace MiscServices
} // namespace OHOS
