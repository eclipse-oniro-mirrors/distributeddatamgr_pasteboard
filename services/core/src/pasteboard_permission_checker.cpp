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

#include "pasteboard_permission_checker.h"
#include "pasteboard_hilog.h"
#include "permission_utils.h"

namespace OHOS {
namespace MiscServices {

PasteboardPermissionChecker::PasteboardPermissionChecker(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardPermissionChecker constructed.");
}

PasteboardPermissionChecker::~PasteboardPermissionChecker()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardPermissionChecker destructed.");
}

bool PasteboardPermissionChecker::VerifyPermission(uint32_t tokenId)
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
    service_.AddPermissionRecord(tokenId, isReadGrant, isSecureGrant);
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
        std::lock_guard<std::mutex> lock(service_.eventMutex_);
        if (service_.inputEventCallback_ != nullptr) {
            isCtrlVAction = service_.inputEventCallback_->IsCtrlVProcess(callPid, service_.IsFocusedApp(tokenId));
            service_.inputEventCallback_->Clear();
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

int32_t PasteboardPermissionChecker::IsDataValid(PasteData& pasteData, uint32_t tokenId, int32_t userId)
{
    if (pasteData.IsDraggedData() || !pasteData.IsValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is invalid");
        return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
    }
    if (service_.IsDataAged(userId)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "data is aged");
        return static_cast<int32_t>(PasteboardError::DATA_EXPIRED_ERROR);
    }
    auto screenStatus = service_.GetScreenStatus(userId);
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

bool PasteboardPermissionChecker::IsPermissionGranted(const std::string& perm, uint32_t tokenId)
{
    return PermissionUtils::IsPermissionGranted(perm, tokenId);
}

bool PasteboardPermissionChecker::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "called token id: %{public}" PRIu64, tokenId);
    return (tokenId & SYSTEM_APP_MASK) == SYSTEM_APP_MASK;
}

bool PasteboardPermissionChecker::IsCopyable(uint32_t tokenId) const
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

int32_t PasteboardPermissionChecker::GetSdkVersion(uint32_t tokenId)
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

bool PasteboardPermissionChecker::IsCallerUidValid()
{
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid == EDM_UID || (service_.uid_ != -1 && callingUid == service_.uid_)) {
        return true;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "callingUid error: %{public}d.", callingUid);
    return false;
}

bool PasteboardPermissionChecker::CheckMdmShareOption(PasteData& pasteData)
{
    bool result = false;
    globalShareOptions_.ComputeIfPresent(
        pasteData.GetTokenId(), [&result](const uint32_t& tokenId, GlobalShareOption& option) {
            if (option.source == MDM) {
                result = true;
            }
            return true;
        });
    return result;
}

void PasteboardPermissionChecker::UpdateShareOption(PasteData& pasteData)
{
    globalShareOptions_.ComputeIfPresent(
        pasteData.GetTokenId(), [&pasteData](const uint32_t& tokenId, GlobalShareOption& option) {
            pasteData.SetShareOption(option.shareOption);
            return true;
        });
}

int32_t PasteboardPermissionChecker::SetGlobalShareOption(const std::unordered_map<uint32_t, int32_t>& globalShareOptions)
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
    for (const auto& [tokenId, shareOption] : shareOptions) {
        GlobalShareOption option = {.source = MDM, .shareOption = shareOption};
        globalShareOptions_.InsertOrAssign(tokenId, option);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Set %{public}zu global shareOption.", globalShareOptions.size());
    return ERR_OK;
}

int32_t PasteboardPermissionChecker::RemoveGlobalShareOption(const std::vector<uint32_t>& tokenIds)
{
    if (!IsCallerUidValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No Permission");
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    int32_t count = 0;
    for (const uint32_t& tokenId : tokenIds) {
        globalShareOptions_.ComputeIfPresent(tokenId, [&count](const uint32_t& key, GlobalShareOption& value) {
            count++;
            return false;
        });
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Remove %{public}d global shareOption.", count);
    return ERR_OK;
}

int32_t PasteboardPermissionChecker::GetGlobalShareOption(const std::vector<uint32_t>& tokenIds,
    std::unordered_map<uint32_t, int32_t>& funcResult)
{
    if (!IsCallerUidValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No Permission");
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    for (const uint32_t& tokenId : tokenIds) {
        globalShareOptions_.ComputeIfPresent(
            tokenId, [&funcResult](const uint32_t& key, GlobalShareOption& option) {
                funcResult[key] = static_cast<int32_t>(option.shareOption);
                return true;
            });
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Get %{public}zu global shareOption.", funcResult.size());
    return ERR_OK;
}

int32_t PasteboardPermissionChecker::SetAppShareOptions(int32_t shareOptions)
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
    auto isAbsent = globalShareOptions_.ComputeIfAbsent(tokenId, [&option](const uint32_t& tokenId) {
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

int32_t PasteboardPermissionChecker::RemoveAppShareOptions()
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
    globalShareOptions_.ComputeIfPresent(tokenId, [&result](const uint32_t& key, GlobalShareOption& value) {
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

} // namespace MiscServices
} // namespace OHOS