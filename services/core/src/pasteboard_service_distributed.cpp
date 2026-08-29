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
#include "file_mount_manager.h"
#include "ipc_skeleton.h"
#include "os_account_manager.h"
#include "pasteboard_common.h"
#include "pasteboard_delay_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_hilog.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_mime_utils.h"
#include "pasteboard_time.h"
#include "pasteboard_web_controller.h"
#include "remote_file_share.h"
#include "reporter.h"

namespace OHOS {
namespace MiscServices {
using namespace Storage::DistributedFile;
using namespace RadarReporter;
using namespace OHOS::AppFileService::ModuleRemoteFileShare;
namespace {
constexpr int32_t DEVICE_COLLABORATION_UID = 5521;
const std::string CONSTRAINT = "constraint.distributed.transmission.outgoing";
} // namespace

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

void PasteboardService::HandleWifiOffAndClearDistributedEvent(int32_t userId)
{
    bool isdeviceCollabSwitch = switch_.GetDeviceCollabSwitch(userId);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(!isdeviceCollabSwitch, PASTEBOARD_MODULE_SERVICE,
        "wifi off but DeviceCollabSwitch is on");
    PASTEBOARD_CHECK_AND_RETURN_LOGD(IsValidCurrentEvent(), PASTEBOARD_MODULE_SERVICE, "wifi off but no valid event");
    CleanDistributedData(userId);
}

std::pair<std::shared_ptr<PasteData>, PasteDateResult> PasteboardService::GetDistributedData(
    const Event &event, int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    PasteDateResult pasteDateResult;
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR);
        return std::make_pair(nullptr, pasteDateResult);
    }
    std::vector<uint8_t> rawData;
    auto result = clipPlugin->GetPasteData(event, rawData);
    if (result.first != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get data failed");
        Reporter::GetInstance().PasteboardFault().Report({ user, "GET_REMOTE_DATA_FAILED" });
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = result.first;
        return std::make_pair(nullptr, pasteDateResult);
    }
    if (static_cast<int64_t>(rawData.size()) > maxLocalCapacity_.load()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote dataSize exceeded, dataSize=%{public}zu", rawData.size());
        pasteDateResult.syncTime = 0;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED);
        return std::make_pair(nullptr, pasteDateResult);
    }
    SetCurrentEvent(std::move(event));
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->Decode(rawData);
    pasteData->SetOriginAuthority(std::make_pair(pasteData->GetBundleName(), pasteData->GetAppIndex()));
    pasteData->rawDataSize_ = static_cast<int64_t>(rawData.size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set remote data, dataSize=%{public}" PRId64, pasteData->rawDataSize_);
    for (size_t i = 0; i < pasteData->GetRecordCount(); i++) {
        auto item = pasteData->GetRecordAt(i);
        if (item == nullptr || item->GetConvertUri().empty()) {
            continue;
        }
        if (item->GetOriginUri() == nullptr) {
            item->SetConvertUri("");
            continue;
        }
        item->isConvertUriFromRemote = true;
    }
    pasteDateResult.syncTime = result.second;
    pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::E_OK);
    return std::make_pair(pasteData, pasteDateResult);
}

bool PasteboardService::IsConstraintEnabled(int32_t user)
{
    bool isConstraintEnabled = false;
    ErrCode err = AccountSA::OsAccountManager::CheckOsAccountConstraintEnabled(user, CONSTRAINT, isConstraintEnabled);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        err == ERR_OK, false, PASTEBOARD_MODULE_SERVICE, "CheckOsAccountConstraintEnabled failed, %{public}d", err);
    return isConstraintEnabled;
}

bool PasteboardService::IsDisallowDistributed()
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    if (uid == DEVICE_COLLABORATION_UID) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uid from device collaboration");
        return true;
    }
    return false;
}

bool PasteboardService::IsNeedLink(PasteData &data)
{
    for (const auto &record : data.AllRecords()) {
        if (record == nullptr) {
            continue;
        }
        auto uri = record->GetConvertUri();
        if (uri.empty()) {
            continue;
        }
        if (uri.find("networkid=") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool PasteboardService::SetDistributedData(int32_t user, PasteData &data)
{
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!networkId.empty(), false, PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
    Event event;
    event.user = user;
    event.seqId = ++sequenceId_;
    auto expiration = PasteBoardTime::GetBootTimeMs() + EXPIRATION_INTERVAL;
    event.expiration = static_cast<uint64_t>(expiration);
    event.deviceId = networkId;
    event.account = AccountManager::GetInstance().GetCurrentAccount();
    event.status = ClipPlugin::EVT_NORMAL;
    event.dataType = data.GetMimeTypes();
    event.isDelay = data.IsDelayRecord();
    event.dataId = data.GetDataId();
    SetCurrentEvent(event);

    if (IsConstraintEnabled(user) || IsDisallowDistributed()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "not allowed to send, user:%{public}d", user);
        return false;
    }
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", data.GetDataId());
        return false;
    }
    ShareOption shareOpt = data.GetShareOption();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::InApp, false, PASTEBOARD_MODULE_SERVICE,
        "data share option is in app, dataId:%{public}u", data.GetDataId());
    if (CheckMdmShareOption(data)) {
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::LocalDevice, false, PASTEBOARD_MODULE_SERVICE,
            "data share option is local device, dataId:%{public}u", data.GetDataId());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, isDelay:%{public}d,"
        "expiration:%{public}" PRIu64, event.dataId, event.seqId, event.isDelay, event.expiration);
    return SetCurrentDistributedData(data, event);
}

bool PasteboardService::SetCurrentDistributedData(PasteData &data, Event event)
{
    std::thread thread([this, data, event]() mutable {
        {
            std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
            setDistributedMemory_.latestEvent = event;
            setDistributedMemory_.latestData = std::make_shared<PasteData>(data);
            PASTEBOARD_CHECK_AND_RETURN_LOGD(!setDistributedMemory_.isRunning, PASTEBOARD_MODULE_SERVICE, "running");
            setDistributedMemory_.isRunning = true;
        }
        bool isNeedCheck = false;
        while (true) {
            auto block = std::make_shared<BlockObject<bool>>(SET_DISTRIBUTED_DATA_INTERVAL, false);
            {
                std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
                if ((setDistributedMemory_.currentEvent.seqId == setDistributedMemory_.latestEvent.seqId
                        && isNeedCheck) || setDistributedMemory_.latestData == nullptr) {
                    setDistributedMemory_.latestData = nullptr;
                    setDistributedMemory_.isRunning = false;
                    break;
                }
            }
            if (!isNeedCheck) {
                isNeedCheck = true;
            }
            std::thread innerThread([this, block]() mutable {
                auto result = SetCurrentData();
                block->SetValue(true);
            });
            PasteBoardCommonUtils::SetThreadTaskName(innerThread, "SetCurrentData");
            innerThread.detach();
            bool ret = block->GetValue();
            if (!ret) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SetCurrentData timeout,seqId:%{public}hu", event.seqId);
            }
        }
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "SetDistributeDa");
    thread.detach();
    return true;
}

bool PasteboardService::SetCurrentData()
{
    PasteData currentData;
    Event currentEvent;
    {
        std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
        if (setDistributedMemory_.latestData == nullptr) {
            return false;
        }
        setDistributedMemory_.currentEvent = setDistributedMemory_.latestEvent;
        currentEvent = setDistributedMemory_.currentEvent;
        currentData = *setDistributedMemory_.latestData;
    }
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_LOAD_DISTRIBUTED_PLUGIN, DFX_SUCCESS);
        PASTEBOARD_HILOGE(
            PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", currentData.GetDataId());
        return false;
    }
    RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_LOAD_DISTRIBUTED_PLUGIN, DFX_SUCCESS);
    bool needFull = currentData.IsDelayRecord() &&
        moduleConfig_.GetRemoteDeviceMinVersion() == DistributedModuleConfig::Version::VERSION_FOUR;
    if (needFull) {
        GetFullDelayPasteData(currentEvent.user, currentData);
        currentEvent.isDelay = false;
        {
            std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
            std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(currentData.GetOriginAuthority());
            PasteboardWebController::GetInstance().SplitWebviewPasteData(
                currentData, bundleIndex, currentData.userId_);
            PasteboardWebController::GetInstance().SetWebviewPasteData(currentData, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(currentData);
        }
    }
    GenerateDistributedUri(currentData);
    currentEvent.notNeedLink = !IsNeedLink(currentData);
    std::vector<uint8_t> rawData;
    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    {
        std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
        if (!currentData.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "distributed data encode failed, dataId:%{public}u, seqId:%{public}hu",
                currentEvent.dataId, currentEvent.seqId);
            return false;
        }
    }
    if (currentData.IsDelayRecord() && !needFull) {
        clipPlugin->RegisterDelayCallback(
            std::bind(&PasteboardService::GetDistributedDelayData, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3),
            std::bind(&PasteboardService::GetDistributedDelayEntry, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }
    std::vector<uint8_t> rawMimeTypes;
    if (rawData.size() > MAX_TRANSFER_SIZE) {
        auto mimeTypes = currentData.GetMimeTypes();
        rawMimeTypes = EncodeMimeTypes(mimeTypes);
    }
    clipPlugin->SetPasteData(currentEvent, rawData, remoteVersionMin, rawMimeTypes);
    return true;
}

int32_t PasteboardService::GetDistributedDelayEntry(const Event &evt, uint32_t recordId, const std::string &utdId,
    std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64
        ", recordId:%{public}u, type:%{public}s", evt.dataId, evt.seqId, evt.expiration, recordId, utdId.c_str());
    auto [hasData, data] = clips_.Find(evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    auto record = data->GetRecordById(recordId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID),
        PASTEBOARD_MODULE_SERVICE, "recordId=%{public}u invalid, max=%{public}zu", recordId, data->GetRecordCount());

    PasteDataEntry entry;
    entry.SetUtdId(utdId);
    int32_t ret = GetLocalEntryValue(evt.user, *data, *record, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, seqId=%{public}hu, dataId=%{public}u, recordId=%{public}u"
        ", type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_URI) {
        ret = ProcessDistributedDelayUri(evt.user, *data, entry, recordId, rawData);
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessDistributedDelayHtml(*data, entry, rawData);
    } else {
        ret = ProcessDistributedDelayEntry(entry, rawData);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "process distributed entry failed, seqId=%{public}hu, dataId=%{public}u, "
        "recordId=%{public}u, type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "type=%{public}s, size=%{public}zu", utdId.c_str(), rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDistributedDelayUri(int32_t userId, PasteData &data, PasteDataEntry &entry,
    uint32_t recordId, std::vector<uint8_t> &rawData)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");

    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        auto item = data.GetRecordById(recordId);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(item != nullptr, static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID),
            PASTEBOARD_MODULE_SERVICE, "record[%{public}u]invalid, max=%{public}zu", recordId, data.GetRecordCount());
        bool hasUriPerm = item->HasGrantUriPermission();
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasUriPerm, static_cast<int32_t>(PasteboardError::INVALID_URI_ERROR),
            PASTEBOARD_MODULE_SERVICE, "no permission, uri=%{private}s", uri->ToString().c_str());
    }
    std::string localUri = uri->ToString();
    std::vector<std::string> localUris = { localUri };
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    int32_t ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(localUris, userId, dfsUris);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE,
        "generate distributed uri failed, uri=%{private}s", localUri.c_str());

    auto dfsUri = dfsUris.find(localUri);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(dfsUri != dfsUris.end(), static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "dfsUris is null");
    std::string distributedUri = dfsUri->second.uriStr;
    size_t fileSize = dfsUri->second.fileSize;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri: %{private}s -> %{private}s, fileSize=%{public}zu",
        localUri.c_str(), distributedUri.c_str(), fileSize);

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(distributedUri);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::FILE_URI_PARAM] = distributedUri;
        entry.SetValue(newObject);
        entry.SetFileSize(static_cast<int64_t>(fileSize));
    }

    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode uri failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDistributedDelayHtml(PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_)) {
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }

    PasteData tmp;

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        auto newEntry = std::make_shared<PasteDataEntry>(entry.GetUtdId(), entry.GetMimeType(), EntryValue(newObject));
        auto record = std::make_shared<PasteDataRecord>();
        record->AddEntryByMimeType(MIMETYPE_TEXT_HTML, newEntry);
        tmp.AddRecord(record);
    } else {
        std::shared_ptr<std::string> html = entry.ConvertToHtml();
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr,
            static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
            PASTEBOARD_MODULE_SERVICE, "convert to html failed");
        tmp.AddHtmlRecord(*html);
    }
    tmp.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    if (PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, data.userId_)) {
        PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);
        GenerateDistributedUri(tmp);
    }

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    bool encodeSucc = tmp.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode html failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::ProcessDistributedDelayEntry(PasteDataEntry &entry, std::vector<uint8_t> &rawData)
{
    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode entry failed, type=%{public}s", entry.GetUtdId().c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetDistributedDelayData(const Event &evt, uint8_t version, std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64,
        evt.dataId, evt.seqId, evt.expiration);
    auto [hasData, data] = clips_.Find(evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    if (version == 0) {
        ret = GetFullDelayPasteData(evt.user, *data);
    } else if (version == 1) {
        ret = GetDelayPasteRecord(evt.user, *data);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get delay data failed, version=%{public}hhu", version);

    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        auto authorityInfo = data->GetOriginAuthority();
        data->SetBundleInfo(authorityInfo.first, authorityInfo.second);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authorityInfo);
        PasteboardWebController::GetInstance().SplitWebviewPasteData(*data, bundleIndex, evt.user);
        PasteboardWebController::GetInstance().SetWebviewPasteData(*data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(*data);
    }
    GenerateDistributedUri(*data);

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
    bool encodeSucc = data->Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode data failed, dataId:%{public}u, seqId:%{public}hu", evt.dataId, evt.seqId);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "size=%{public}zu", rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardService::GetLocalEntryValue(int32_t userId, PasteData &data, PasteDataRecord &record,
    PasteDataEntry &value)
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
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + value.rawDataSize_ < maxLocalCapacity_.load()) {
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

int32_t PasteboardService::GetFullDelayPasteData(int32_t userId, PasteData &data)
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
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    clips_.ComputeIfPresent(userId, [&data](auto, auto &value) {
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

void PasteboardService::GenerateDistributedUri(PasteData &data)
{
    std::vector<std::string> uris;
    std::vector<size_t> indexes;
    auto userId = GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
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

std::shared_ptr<ClipPlugin> PasteboardService::GetClipPlugin()
{
    auto isOn = moduleConfig_.IsOn();
    if (isOn) {
        auto isSupported = securityLevel_.IsSupportedDistributed(false);
        if (!isSupported) {
            return nullptr;
        }
    }
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    if (!isOn || clipPlugin_ != nullptr) {
        return clipPlugin_;
    }
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    InitPlugin(clipPlugin_);
    return clipPlugin_;
}

void PasteboardService::CleanDistributedData(int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->Clear(user);
}

bool PasteboardService::IsValidCurrentEvent()
{
    auto expiration = PasteBoardTime::GetBootTimeMs();
    auto currentEvent = GetCurrentEvent();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(static_cast<uint64_t>(expiration) < currentEvent.expiration,
        false, PASTEBOARD_MODULE_SERVICE, "event is invalid");
    return true;
}

void PasteboardService::CloseDistributedStore(int32_t user, bool isNeedClear)
{
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
    if (isNeedClear) {
        clipPlugin_->Clear(user);
    }
    clipPlugin_->Close(user);
}

void PasteboardService::OnConfigChange(bool isOn)
{
    std::thread thread([=]() {
        OnConfigChangeInner(isOn);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "OnConfigChange");
    thread.detach();
}

void PasteboardService::OnConfigChangeInner(bool isOn)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConfigChange isOn: %{public}d.", isOn);
    if (!isOn) {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.ForEach([this](const auto &deviceId, auto &value) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "configChange is off, need close p2p link.");
            CloseP2PLink(deviceId);
            return false;
        });
        p2pMap_.Clear();
    }
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    if (!isOn) {
        PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        int32_t userId = ResolveMainDisplayUserId();
        PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE,
            "main display user invalid");
        clipPlugin_->Close(userId);
        clipPlugin_ = nullptr;
        return;
    }
    SetCriticalTimer();
    auto isSupported = securityLevel_.IsSupportedDistributed(true);
    if (!isSupported) {
        return;
    }
    if (clipPlugin_ != nullptr) {
        return;
    }
    SubscribeKeyboardEvent();
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    InitPlugin(clipPlugin_);
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
} // namespace MiscServices
} // namespace OHOS
