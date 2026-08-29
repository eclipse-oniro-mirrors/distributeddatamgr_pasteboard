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

#include <cinttypes>

#include "errors.h"
#include "ipc_skeleton.h"
#include "paste_data.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "permission/permission_utils.h"

namespace OHOS {
namespace MiscServices {
namespace {
constexpr const char *MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION =
    "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION";
constexpr uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);
} // namespace

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
    return ERR_OK;
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
            return ERR_OK;
        } else {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Can not remove token id: 0x%{public}x.", tokenId);
            return ERR_OK;
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "This token id: 0x%{public}x not set.", tokenId);
    return ERR_OK;
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
} // namespace MiscServices
} // namespace OHOS
