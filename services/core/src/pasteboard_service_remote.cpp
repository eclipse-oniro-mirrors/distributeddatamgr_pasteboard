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

#include "account_manager.h"
#include "common/pasteboard_common_utils.h"
#include "ipc_skeleton.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_mime_utils.h"
#include "pasteboard_time.h"
#include "pasteboard_web_controller.h"

namespace OHOS {
namespace MiscServices {

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
        auto it = clips_.Find(userId);
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
        auto result = GetDistributedData(event, userId);
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        std::shared_ptr<PasteDateTime> pasteDataTime = std::make_shared<PasteDateTime>();
        if (result.first != nullptr) {
            result.first->SetRemote(true);
            if (distEvt == event) {
                clips_.InsertOrAssign(userId, result.first);
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

bool PasteboardService::HasRemoteUri(std::shared_ptr<PasteData> data)
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
    auto [hasData, data] = clips_.Find(appInfo.userId);
    if (!hasData || data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "local data is null");
        return false;
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(data->IsRemote(), false,
        PASTEBOARD_MODULE_SERVICE, "not contains remote data.");
    bool hasRemoteUri = HasRemoteUri(data);
    return hasRemoteUri;
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
    auto it = clips_.Find(userId);
    if (!it.first) {
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        return distRet == static_cast<int32_t>(PasteboardError::E_OK);
    }
    return it.second->IsRemote();
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

int32_t PasteboardService::GetRemoteEntryValue(const AppInfo &appInfo, PasteData &data, PasteDataRecord &record,
    PasteDataEntry &entry)
{
    auto clipPlugin = GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(clipPlugin != nullptr, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL),
        PASTEBOARD_MODULE_SERVICE, "plugin is null");

    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
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
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < maxLocalCapacity_.load()) {
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

int32_t PasteboardService::ProcessRemoteDelayUri(const std::string &deviceId, const AppInfo &appInfo,
    PasteData &data, PasteDataRecord &record, PasteDataEntry &entry)
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
    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(deviceId, data.GetPasteId());
        int32_t ret = GrantUriPermission(grantUris, appInfo.tokenId, data.IsRemote());
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant remote uri failed, uri=%{private}s, ret=%{public}d",
            distributedUri.c_str(), ret);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessRemoteDelayHtml(const std::string &remoteDeviceId, const AppInfo &appInfo,
    const std::vector<uint8_t> &rawData, PasteData &data, PasteDataRecord &record, PasteDataEntry &entry)
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
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < maxLocalCapacity_.load()) {
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
        for (auto &recordItem : tmpData.AllRecords()) {
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

int32_t PasteboardService::ProcessRemoteDelayHtmlInner(const std::string &remoteDeviceId, const AppInfo &appInfo,
    PasteData &tmpData, PasteData &data, PasteDataEntry &entry)
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

    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(remoteDeviceId, data.GetPasteId());
        int32_t ret = GrantUriPermission(grantUris, appInfo.tokenId, data.IsRemote());
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant to %{public}s failed, ret=%{public}d", appInfo.bundleName.c_str(), ret);
    }

    tmpData.SetOriginAuthority(data.GetOriginAuthority());
    tmpData.SetTokenId(data.GetTokenId());
    tmpData.SetRemote(data.IsRemote());
    SetLocalPasteFlag(tmpData.IsRemote(), appInfo.tokenId, tmpData);
    int32_t ret = PostProcessDelayHtmlEntry(tmpData, appInfo, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "post process remote html failed, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}
} // namespace MiscServices
} // namespace OHOS
