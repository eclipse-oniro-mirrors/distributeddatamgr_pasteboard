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

#include "ipc_skeleton.h"
#include "pasteboard_common.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "uri_permission_manager_client.h"

namespace OHOS {
namespace MiscServices {
namespace {
constexpr int32_t MAX_REMOTE_FILE_MANAGER_URI_COUNT = 256;
constexpr const char *FILE_DOCS_URI_PREFIX = "file://docs/";
constexpr const char *FILEMANAGER_KEY = "filemanager";
} // namespace

int32_t PasteboardService::CheckAndGrantRemoteUri(PasteData &data, const AppInfo &appInfo,
    const std::string &pasteId, std::shared_ptr<BlockObject<int32_t>> pasteBlock)
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
        if (pasteBlock) {
            if (!grantUris.empty()) {
                pasteBlock->GetValue();
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "wait P2PEstablish finish");
            } else {
                PasteComplete(data.deviceId_, pasteId);
            }
        }
    }
    ClearP2PEstablishTaskInfo();
    return GrantUriPermission(grantUris, appInfo.tokenId, isRemoteData);
}

bool PasteboardService::IsFileManagerApp(const std::string &bundleName)
{
    auto fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    return IsSystemAppByFullTokenID(fullTokenId) && bundleName.find(FILEMANAGER_KEY) != std::string::npos;
}

bool PasteboardService::StartWith(const std::string &str, const std::string &prefix)
{
    if (prefix.size() > str.size()) {
        return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
}

int32_t PasteboardService::CheckRemoteFileDocsUriLimit(const std::vector<Uri> &grantUris, const std::string &bundleName)
{
    if (IsFileManagerApp(bundleName) || grantUris.size() <= MAX_REMOTE_FILE_MANAGER_URI_COUNT) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }
    for (const auto &uri : grantUris) {
        if (StartWith(uri.ToString(), FILE_DOCS_URI_PREFIX)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "remote uri count %{public}zu bundleName is %{public}s", grantUris.size(), bundleName.c_str());
            return static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED);
        }
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId)
{
    size_t offset = 0;
    size_t length = grantUris.size();
    size_t count = PasteData::URI_BATCH_SIZE;
    bool hasGranted = false;
    int32_t permissionCode = 0;
    int32_t ret = 0;
    auto appInfo = GetAppInfo(targetTokenId);
    int32_t userId = appInfo.userId;
    auto [hasData, data] = clips_.Find(userId);
    uint32_t srcTokenId = (hasData && data) ? data->GetTokenId() : 0;
    if (isRemoteData && CheckRemoteFileDocsUriLimit(grantUris, appInfo.bundleName) !=
        static_cast<int32_t>(PasteboardError::E_OK)) {
        return ret;
    }
    while (length > offset) {
        if (length - offset < PasteData::URI_BATCH_SIZE) {
            count = length - offset;
        }
        auto sendValues = std::vector<Uri>(grantUris.begin() + offset, grantUris.begin() + offset + count);
        if (isRemoteData) {
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermissionPrivileged(
                sendValues, permFlag, appInfo.bundleName, appInfo.appIndex);
        } else {
            std::vector<std::string> uriStrVec;
            for (auto &uri : sendValues) {
                uriStrVec.emplace_back(uri.ToString());
            }
            permissionCode = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermission(
                uriStrVec, permFlag, targetTokenId, srcTokenId);
        }
        hasGranted = hasGranted || (permissionCode == 0);
        ret = permissionCode == 0 ? ret : permissionCode;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "permissionCode is %{public}d", permissionCode);
        offset += count;
    }
    if (hasGranted) {
        std::lock_guard<std::mutex> lock(readBundleMutex_);
        if (readBundles_.count(targetTokenId) == 0) {
            readBundles_.insert(targetTokenId);
        }
    }
    return ret;
}

int32_t PasteboardService::GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
    uint32_t targetTokenId, bool isRemoteData)
{
    std::vector<Uri> readUris = grantUris[PasteDataRecord::READ_PERMISSION];
    std::vector<Uri> writeUris = grantUris[PasteDataRecord::READ_WRITE_PERMISSION];
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(!readUris.empty() ||
        !writeUris.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no uri");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "readUris=%{public}zu, writeUris=%{public}zu, targetTokenId=%{public}u",
        readUris.size(), writeUris.size(), targetTokenId);
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(callingUid != ANCO_SERVICE_BROKER_UID,
        static_cast<int32_t>(PasteboardError::E_OK), PASTEBOARD_MODULE_SERVICE, "callingUid = ANCO_SERVICE_BROKER_UID");
    int32_t ret = 0;
    if (isRemoteData) {
        RemoveInvalidRemoteUri(readUris);
        RemoveInvalidRemoteUri(writeUris);
    }
    auto permFlag = PasteDataRecord::READ_PERMISSION;
    ret = GrantPermission(readUris, permFlag, isRemoteData, targetTokenId);
    if (!isRemoteData) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "NeedPersistance, permFlag is %{public}d", permFlag);
        permFlag = PasteDataRecord::READ_WRITE_PERMISSION;
    }
    auto result = GrantPermission(writeUris, permFlag, isRemoteData, targetTokenId);
    ret = result == 0 ? ret : result;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "leave, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
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
} // namespace MiscServices
} // namespace OHOS
