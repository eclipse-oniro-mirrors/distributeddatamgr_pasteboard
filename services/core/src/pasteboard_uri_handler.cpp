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

#include "pasteboard_uri_handler.h"

#include "pasteboard_hilog.h"
#include "pasteboard_error.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_common.h"
#include "pasteboard_user_context.h"
#include "ipc_skeleton.h"
#include "uri_permission_manager_client.h"
#include "file_mount_manager.h"
#include "common/pasteboard_common_utils.h"

namespace OHOS {
namespace MiscServices {

PasteboardUriHandler::PasteboardUriHandler(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardUriHandler constructed.");
}

PasteboardUriHandler::~PasteboardUriHandler()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardUriHandler destructed.");
}

bool PasteboardUriHandler::IsBundleOwnUriPermission(const std::string &bundleName, Uri &uri)
{
    return (bundleName.compare(uri.GetAuthority()) == 0);
}

void PasteboardUriHandler::RemoveInvalidRemoteUri(std::vector<Uri> &grantUris)
{
    auto newEnd = std::remove_if(grantUris.begin(), grantUris.end(),
        [](const Uri& uri) {
            std::string puri = uri.ToString();
            return puri.find("networkid=") == std::string::npos;
        });
    grantUris.erase(newEnd, grantUris.end());
}

bool PasteboardUriHandler::HasRemoteUri(std::shared_ptr<PasteData> data)
{
    for (const auto &record : data->AllRecords()) {
        if (record == nullptr) {
            continue;
        }
        auto recordTypes = record->GetMimeTypes();
        if (recordTypes.find(MIMETYPE_TEXT_URI) == recordTypes.end()) {
            continue;
        }
        auto convertUri = record->GetConvertUri();
        if (!convertUri.empty() && convertUri.find(PasteboardImgExtractor::FILE_SCHEME_PREFIX) == 0 &&
            convertUri.find("networkid=") != std::string::npos) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "record has convert uri");
            return true;
        }
        auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_URI);
        if (entry == nullptr) {
            continue;
        }
        if (!entry->HasContentByMimeType(MIMETYPE_TEXT_URI)) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "uri is delay, has no content");
            return true;
        }
        auto uri = entry->ConvertToUri();
        if (uri == nullptr) {
            continue;
        }
        auto uriStr = uri->ToString();
        if (!uriStr.empty() && uriStr.find(PasteboardImgExtractor::FILE_SCHEME_PREFIX) == 0 &&
            uriStr.find("networkid=") != std::string::npos) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "has remote uri");
            return true;
        }
    }
    return false;
}

std::map<uint32_t, std::vector<Uri>> PasteboardUriHandler::CheckUriPermission(PasteData &data,
    const std::pair<std::string, int32_t> &targetBundleAndIndex)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "enter");
    std::vector<Uri> readUris;
    std::vector<Uri> writeUris;
    std::map<uint32_t, std::vector<Uri>> result;
    std::shared_lock<std::shared_mutex> read(service_.pasteDataMutex_);
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

int32_t PasteboardUriHandler::GrantPermission(const std::vector<Uri> &grantUris, uint32_t permFlag, bool isRemoteData,
    uint32_t targetTokenId)
{
    size_t offset = 0;
    size_t length = grantUris.size();
    size_t count = PasteData::URI_BATCH_SIZE;
    bool hasGranted = false;
    int32_t permissionCode = 0;
    int32_t ret = 0;
    auto appInfo = service_.GetAppInfo(targetTokenId);
    int32_t userId = appInfo.userId;
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = service_.clips_.Find(service_.GetCompositeKey(userId));
#else
    auto [hasData, data] = service_.clips_.Find(service_.GetCompositeKey(userId));
#endif
    uint32_t srcTokenId = (hasData && data) ? data->GetTokenId() : 0;
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
        std::lock_guard<std::mutex> lock(service_.readBundleMutex_);
        if (service_.readBundles_.count(targetTokenId) == 0) {
            service_.readBundles_.insert(targetTokenId);
        }
    }
    return ret;
}

int32_t PasteboardUriHandler::GrantUriPermission(std::map<uint32_t, std::vector<Uri>> &grantUris,
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

int32_t PasteboardUriHandler::CheckAndGrantRemoteUri(PasteData &data, const AppInfo &appInfo,
    const std::string &pasteId, std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    int64_t fileSize = data.GetFileSize();
    bool isRemoteData = data.IsRemote();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pasteId=%{public}s, isRemote=%{public}s, fileSize=%{public}" PRId64,
        pasteId.c_str(), isRemoteData ? "true" : "false", fileSize);
    service_.GetPasteDataDot(data, appInfo.bundleName, appInfo.userId);
    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (isRemoteData) {
        data.SetPasteId(pasteId);
        if (pasteBlock) {
            if (!grantUris.empty()) {
                pasteBlock->GetValue();
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "wait P2PEstablish finish");
            } else {
                service_.PasteComplete(data.deviceId_, pasteId);
            }
        }
    }
    service_.ClearP2PEstablishTaskInfo();
    return GrantUriPermission(grantUris, appInfo.tokenId, isRemoteData);
}

void PasteboardUriHandler::GenerateDistributedUri(PasteData &data)
{
    std::vector<std::string> uris;
    std::vector<size_t> indexes;
    auto userId = service_.GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::unique_lock<std::shared_mutex> write(service_.pasteDataMutex_);
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
        if (ret != 0 || dfsUris.empty()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "Get remoteUri failed, ret:%{public}d, userId:%{public}d, uri size:%{public}zu.",
                ret, userId, uris.size());
        }
        for (size_t i = 0; i < indexes.size(); i++) {
            auto item = data.GetRecordAt(indexes[i]);
            if (item == nullptr) {
                continue;
            }
            if (item->GetOriginUri() == nullptr) {
                if (!item->GetConvertUri().empty()) {
                    item->SetConvertUri(" ");
                }
                continue;
            }
            auto it = dfsUris.find(item->GetOriginUri()->ToString());
            if (it != dfsUris.end()) {
                item->SetConvertUri(it->second.uriStr);
                fileSize += it->second.fileSize;
            } else {
                item->SetConvertUri(" ");
            }
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "file size: %{public}zu", fileSize);
    data.SetFileSize(static_cast<int64_t>(fileSize));
}

void PasteboardUriHandler::ClearUriOnUninstall(int32_t userId, int32_t tokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(tokenId >= 0, PASTEBOARD_MODULE_SERVICE, "tokenId is invalid");
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "userId is invalid");
    #ifdef PB_COCKPIT_PLATFORM_ENABLE
    service_.clips_.ComputeIfPresent(service_.GetCompositeKey(userId), [this, tokenId, userId](auto, auto &pasteData) {
#else
    service_.clips_.ComputeIfPresent(service_.GetCompositeKey(userId), [this, tokenId, userId](auto, auto &pasteData) {
#endif
        if (pasteData == nullptr) {
            return true;
        }
        if (pasteData->GetTokenId() != static_cast<uint32_t>(tokenId)) {
            return true;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "clear uri, tokenId=%{public}d, userId=%{public}d",
            tokenId, userId);
        ClearUriOnUninstall(pasteData);
        service_.delayGetters_.ComputeIfPresent(userId, [](auto, auto &delayGetter) {
            if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
                delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
            }
            return false;
        });
        service_.entryGetters_.ComputeIfPresent(userId, [](auto, auto &entryGetter) {
            if (entryGetter.first != nullptr && entryGetter.second != nullptr) {
                entryGetter.first->AsObject()->RemoveDeathRecipient(entryGetter.second);
            }
            return false;
        });
        return true;
    });
}

void PasteboardUriHandler::ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteData != nullptr, PASTEBOARD_MODULE_SERVICE, "pasteData is null");
    std::thread thread([pasteData, this]() {
        {
            std::unique_lock<std::shared_mutex> threadWriteLock(service_.pasteDataMutex_);
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

        std::lock_guard<decltype(service_.mutex)> lockGuard(service_.mutex);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(service_.clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        service_.clipPlugin_->Clear(pasteData->userId_);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "ClearUriUninsta");
    thread.detach();
}

} // namespace MiscServices
} // namespace OHOS