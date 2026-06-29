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

#include "pasteboard_delay_data_handler.h"
#include "pasteboard_hilog.h"
#include "pasteboard_delay_manager.h"
#include "common/pasteboard_common_utils.h"
#include "pasteboard_web_controller.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace MiscServices {

PasteboardDelayDataHandler::DelayGetterDeathRecipient::DelayGetterDeathRecipient(
    int32_t userId, PasteboardDelayDataHandler& handler)
    : userId_(userId), handler_(handler)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Construct Delay Getter Death Recipient");
}

void PasteboardDelayDataHandler::DelayGetterDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start");
    (void)remote;
    handler_.NotifyDelayGetterDied(userId_);
}

PasteboardDelayDataHandler::EntryGetterDeathRecipient::EntryGetterDeathRecipient(
    int32_t userId, PasteboardDelayDataHandler& handler)
    : userId_(userId), handler_(handler)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Construct Entry Getter Death Recipient");
}

void PasteboardDelayDataHandler::EntryGetterDeathRecipient::OnRemoteDied(const wptr<IRemoteObject>& remote)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start");
    (void)remote;
    handler_.NotifyEntryGetterDied(userId_);
}

PasteboardDelayDataHandler::PasteboardDelayDataHandler(PasteboardService& service)
    : service_(service)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDelayDataHandler constructed.");
}

PasteboardDelayDataHandler::~PasteboardDelayDataHandler()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardDelayDataHandler destructed.");
}

void PasteboardDelayDataHandler::NotifyDelayGetterDied(int32_t userId)
{
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "error userId: %{public}d", userId);
        return;
    }
    delayGetters_.Erase(userId);
}

void PasteboardDelayDataHandler::NotifyEntryGetterDied(int32_t userId)
{
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "error userId: %{public}d", userId);
        return;
    }
    entryGetters_.Erase(userId);
}

void PasteboardDelayDataHandler::GetDelayPasteData(int32_t userId, PasteData& data)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "get delay data start");
    delayGetters_.ComputeIfPresent(userId, [this, &data, userId](auto, auto& delayGetter) {
        PasteData delayData;
        if (delayGetter.first != nullptr) {
            delayGetter.first->GetPasteData("", delayData);
        }
        if (delayGetter.second != nullptr && delayGetter.first != nullptr) {
            delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
        }
        delayData.SetDelayData(false);
        delayData.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
        delayData.SetOriginAuthority(data.GetOriginAuthority());
        delayData.SetTime(data.GetTime());
        delayData.SetTokenId(data.GetTokenId());
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(delayData, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(delayData, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(delayData);
        data = delayData;
        return false;
    });
}

int32_t PasteboardDelayDataHandler::GetDelayPasteRecord(int32_t userId, PasteData& data)
{
    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    auto delayEntryInfos = DelayManager::GetPrimaryDelayEntryInfo(data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!delayEntryInfos.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no delay entry");
    DelayManager::GetLocalEntryValue(delayEntryInfos, getter.first, data);
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDelayDataHandler::GetLocalEntryValue(int32_t userId, PasteData& data, PasteDataRecord& record,
    PasteDataEntry& value)
{
    std::string utdId = value.GetUtdId();
    auto entry = record.GetEntry(utdId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, static_cast<int32_t>(PasteboardError::INVALID_MIMETYPE),
        PASTEBOARD_MODULE_SERVICE, "entry is null, recordId=%{public}u, type=%{public}s", record.GetRecordId(),
        utdId.c_str());

    std::string mimeType = entry->GetMimeType();
    value.SetMimeType(mimeType);
    if (entry->HasContent(utdId)) {
        value.SetValue(entry->GetValue());
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    int32_t ret = getter.first->GetRecordValueByType(record.GetRecordId(), value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);

    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        if (data.rawDataSize_ + value.rawDataSize_ < service_.maxLocalCapacity_.load()) {
            record.AddEntry(utdId, std::make_shared<PasteDataEntry>(value));
            data.rawDataSize_ += value.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, value.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, value.rawDataSize_);
        }
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDelayDataHandler::GetRemoteEntryValue(const AppInfo& appInfo, PasteData& data,
    PasteDataRecord& record, PasteDataEntry& entry)
{
    auto clipPlugin = service_.GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(clipPlugin != nullptr, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL),
        PASTEBOARD_MODULE_SERVICE, "plugin is null");

    auto [distRet, distEvt] = service_.GetValidDistributeEvent(appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(distRet == static_cast<int32_t>(PasteboardError::E_OK) ||
        distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA), distRet,
        PASTEBOARD_MODULE_SERVICE, "get distribute event failed, ret=%{public}d", distRet);

    std::vector<uint8_t> rawData;
    std::string utdId = entry.GetUtdId();
    int32_t ret = clipPlugin->GetPasteDataEntry(distEvt, record.GetRecordId(), utdId, rawData);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE, "get remote raw data failed");

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessRemoteDelayHtml(distEvt.deviceId, appInfo, rawData, data, record, entry);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "process remote delay html failed");
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    PasteDataEntry tmpEntry;
    tmpEntry.Decode(rawData);
    entry.SetValue(tmpEntry.GetValue());
    entry.rawDataSize_ = static_cast<int64_t>(rawData.size());
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < service_.maxLocalCapacity_.load()) {
            record.AddEntry(utdId, std::make_shared<PasteDataEntry>(entry));
            data.rawDataSize_ += entry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        }
    }

    if (mimeType != MIMETYPE_TEXT_URI) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    return ProcessRemoteDelayUri(distEvt.deviceId, appInfo, data, record, entry);
}

int32_t PasteboardDelayDataHandler::ProcessRemoteDelayUri(const std::string& deviceId, const AppInfo& appInfo,
    PasteData& data, PasteDataRecord& record, PasteDataEntry& entry)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");
    std::string distributedUri = uri->ToString();
    record.SetConvertUri(distributedUri);
    record.isConvertUriFromRemote = true;
    record.SetGrantUriPermission(true);

    int64_t uriFileSize = entry.GetFileSize();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri=%{private}s, fileSize=%{public}" PRId64,
        distributedUri.c_str(), uriFileSize);
    if (uriFileSize > 0) {
        int64_t dataFileSize = data.GetFileSize();
        int64_t fileSize = (uriFileSize > INT64_MAX - dataFileSize) ? INT64_MAX : uriFileSize + dataFileSize;
        data.SetFileSize(fileSize);
    }
    std::map<uint32_t, std::vector<Uri>> grantUris = service_.CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        service_.p2pManager_->EstablishP2PLink(deviceId, data.GetPasteId());
        int32_t ret = service_.GrantUriPermission(grantUris, appInfo.tokenId, data.IsRemote());
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant remote uri failed, uri=%{private}s, ret=%{public}d",
            distributedUri.c_str(), ret);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDelayDataHandler::ProcessRemoteDelayHtml(const std::string& remoteDeviceId, const AppInfo& appInfo,
    const std::vector<uint8_t>& rawData, PasteData& data, PasteDataRecord& record, PasteDataEntry& entry)
{
    PasteData tmpData;
    tmpData.Decode(rawData);
    auto htmlRecord = tmpData.GetRecordById(1);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(htmlRecord != nullptr,
        static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED), PASTEBOARD_MODULE_SERVICE, "record is null");
    auto htmlEntry = htmlRecord->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(htmlEntry != nullptr,
        static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED), PASTEBOARD_MODULE_SERVICE, "htmlEntry is null");
    entry.SetValue(htmlEntry->GetValue());
    entry.rawDataSize_ = static_cast<int64_t>(rawData.size());
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < service_.maxLocalCapacity_.load()) {
            record.AddEntry(entry.GetUtdId(), std::make_shared<PasteDataEntry>(entry));
            data.rawDataSize_ += entry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        }

        PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(htmlRecord->GetFrom() != 0, static_cast<int32_t>(PasteboardError::E_OK),
            PASTEBOARD_MODULE_SERVICE, "no uri");

        data.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
        uint32_t htmlRecordId = record.GetRecordId();
        record.SetFrom(htmlRecordId);
        for (auto& recordItem : tmpData.AllRecords()) {
            if (recordItem == nullptr) {
                continue;
            }
            if (!recordItem->GetConvertUri().empty()) {
                recordItem->isConvertUriFromRemote = true;
            }
            if (recordItem->GetFrom() > 0 && recordItem->GetRecordId() != recordItem->GetFrom()) {
                recordItem->SetFrom(htmlRecordId);
                data.AddRecord(*recordItem);
            }
        }
    }
    return ProcessRemoteDelayHtmlInner(remoteDeviceId, appInfo, tmpData, data, entry);
}

int32_t PasteboardDelayDataHandler::ProcessRemoteDelayHtmlInner(const std::string& remoteDeviceId,
    const AppInfo& appInfo, PasteData& tmpData, PasteData& data, PasteDataEntry& entry)
{
    int64_t htmlFileSize = tmpData.GetFileSize();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "htmlFileSize=%{public}" PRId64, htmlFileSize);
    if (htmlFileSize > 0) {
        int64_t dataFileSize = data.GetFileSize();
        int64_t fileSize = (htmlFileSize > INT64_MAX - dataFileSize) ? INT64_MAX : htmlFileSize + dataFileSize;
        data.SetFileSize(fileSize);
    }

    bool isInvalid = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!isInvalid, static_cast<int32_t>(PasteboardError::INVALID_URI_ERROR),
        PASTEBOARD_MODULE_SERVICE, "uri invalid");

    std::map<uint32_t, std::vector<Uri>> grantUris = service_.CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        service_.p2pManager_->EstablishP2PLink(remoteDeviceId, data.GetPasteId());
        int32_t ret = service_.GrantUriPermission(grantUris, appInfo.tokenId, data.IsRemote());
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant to %{public}s failed, ret=%{public}d", appInfo.bundleName.c_str(), ret);
    }

    tmpData.SetOriginAuthority(data.GetOriginAuthority());
    tmpData.SetTokenId(data.GetTokenId());
    tmpData.SetRemote(data.IsRemote());
    PasteboardService::SetLocalPasteFlag(tmpData.IsRemote(), appInfo.tokenId, tmpData);
    int32_t ret = PostProcessDelayHtmlEntry(tmpData, appInfo, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "post process remote html failed, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDelayDataHandler::GetFullDelayPasteData(int32_t userId, PasteData& data)
{
    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    auto delayEntryInfos = DelayManager::GetAllDelayEntryInfo(data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!delayEntryInfos.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no delay entry");
    DelayManager::GetLocalEntryValue(delayEntryInfos, getter.first, data);
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    service_.clips_.ComputeIfPresent(service_.GetCompositeKey(userId), [&data](auto, auto& value) {
#else
    service_.clips_.ComputeIfPresent(service_.GetCompositeKey(userId), [&data](auto, auto& value) {
#endif
        if (data.GetDataId() != value->GetDataId()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "set data fail, data is out time, pre dataId is %{public}d, cur dataId is %{public}d",
                data.GetDataId(), value->GetDataId());
            return true;
        }
        value = std::make_shared<PasteData>(data);
        return true;
    });
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDelayDataHandler::SyncDelayedData()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = service_.GetAppInfo(tokenId);
#ifdef PB_COCKPIT_PLATFORM_ENABLE
    auto [hasData, data] = service_.clips_.Find(service_.GetCompositeKey(appInfo.userId));
#else
    auto [hasData, data] = service_.clips_.Find(service_.GetCompositeKey(appInfo.userId));
#endif
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(tokenId == data->GetTokenId(),
        static_cast<int32_t>(PasteboardError::INVALID_TOKEN_ID), PASTEBOARD_MODULE_SERVICE,
        "tokenId=%{public}u mismatch, local=%{public}u", tokenId, data->GetTokenId());

    int32_t ret = GetFullDelayPasteData(appInfo.userId, *data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get full delay failed, ret=%{public}d", ret);

    std::thread thread([=, userId = appInfo.userId, data = data] {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(data != nullptr, PASTEBOARD_MODULE_SERVICE, "sync delayed data is null");
        data->RemoveEmptyEntry();
#ifdef PB_COCKPIT_PLATFORM_ENABLE
        service_.clips_.ComputeIfPresent(service_.GetCompositeKey(userId), [=](auto, auto& value) {
#else
        service_.clips_.ComputeIfPresent(service_.GetCompositeKey(userId), [=](auto, auto& value) {
#endif
            if (data->GetDataId() == value->GetDataId()) {
                value = std::move(data);
            }
            return true;
        });
    });
    thread.detach();
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDelayDataHandler::ProcessDelayHtmlEntry(PasteData& data, const AppInfo& targetAppInfo,
    PasteDataEntry& entry)
{
    const auto& targetBundle = targetAppInfo.bundleName;
    const auto& appIndex = targetAppInfo.appIndex;
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (!PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, targetAppInfo.userId)) {
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }

    PasteData tmp;
    bool isRemoteData = data.IsRemote();
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    tmp.SetRemote(isRemoteData);
    PasteboardService::SetLocalPasteFlag(tmp.IsRemote(), targetAppInfo.tokenId, tmp);
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, targetAppInfo.userId);
    PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
    PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);

    std::map<uint32_t, std::vector<Uri>> grantUris = service_.CheckUriPermission(
        tmp, std::make_pair(targetBundle, appIndex));
    int32_t ret = service_.GrantUriPermission(grantUris, targetAppInfo.tokenId, isRemoteData);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "grant to %{public}s:%{public}d failed, ret=%{public}d", targetBundle.c_str(),
        appIndex, ret);

    return PostProcessDelayHtmlEntry(tmp, targetAppInfo, entry);
}

int32_t PasteboardDelayDataHandler::PostProcessDelayHtmlEntry(PasteData& data, const AppInfo& targetAppInfo,
    PasteDataEntry& entry)
{
    PasteboardWebController::GetInstance().RetainUri(data);
    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    PasteboardWebController::GetInstance().RebuildWebviewPasteData(data, targetAppInfo.bundleName,
        targetAppInfo.appIndex);

    std::shared_ptr<std::string> html = data.GetPrimaryHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::REBUILD_HTML_FAILED),
        PASTEBOARD_MODULE_SERVICE, "rebuild html failed");

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(*html);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::HTML_CONTENT] = *html;
        entry.SetValue(newObject);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

} // namespace MiscServices
} // namespace OHOS