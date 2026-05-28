/*
 * Copyright (C) 2021-2024 Huawei Device Co., Ltd.
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
#include "pasteboard_distributed_manager.h"

#include <dlfcn.h>
#include <sys/mman.h>

#include "account_manager.h"
#include "device/dev_profile.h"
#include "distributed_file_daemon_manager.h"
#include "eventcenter/pasteboard_event.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "pasteboard_common.h"
#include "common/pasteboard_common_utils.h"
#include "pasteboard_delay_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_pattern.h"
#include "pasteboard_time.h"
#include "pasteboard_trace.h"
#include "pasteboard_event_dfx.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_web_controller.h"
#include "remote_file_share.h"
#include "res_sched_client.h"
#include "reporter.h"
#include "distributed_module_config.h"
#ifdef PB_SCREENLOCK_MGR_ENABLE
#include "screenlock_manager.h"
#endif
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif

namespace OHOS {
namespace MiscServices {
using namespace Storage::DistributedFile;
using namespace RadarReporter;
using namespace Security::AccessToken;
namespace {
constexpr int32_t DEVICE_COLLABORATION_UID = 5521;
constexpr int32_t SET_VALUE_SUCCESS = 1;
constexpr uint16_t MAX_TRANSFER_SIZE = 1300;
constexpr uint32_t EXPIRATION_INTERVAL = 2 * 60 * 1000;
constexpr int MIN_TRANMISSION_TIME = 30 * 1000;
constexpr int PRESYNC_MONITOR_TIME = 2 * 60 * 1000;
constexpr int PRE_ESTABLISH_P2P_LINK_TIME = 2 * 60 * 1000;
constexpr uint32_t SET_DISTRIBUTED_DATA_INTERVAL = 40 * 1000;
constexpr uint32_t GET_REMOTE_DATA_WAIT_TIME = 30000;
constexpr int64_t PRESYNC_MONITOR_INTERVAL_MILLISECONDS = 500;
constexpr int32_t INVALID_SUBSCRIBE_ID = -1;
constexpr const char *PLUGIN_NAME = "distributed_clip";
constexpr int32_t DATA_SEC_LEVEL3 = 3;
constexpr int32_t WIFI_DISABLED = 1;
constexpr int32_t CTRLV_EVENT_SIZE = 2;
} // namespace

const std::string PasteboardDistributedManager::REGISTER_PRESYNC_MONITOR = "RegisterPresyncMonitor";
const std::string PasteboardDistributedManager::UNREGISTER_PRESYNC_MONITOR = "UnregisterPresyncMonitor";
const std::string PasteboardDistributedManager::P2P_ESTABLISH_STR = "P2pEstablish";
const std::string PasteboardDistributedManager::P2P_PRESYNC_ID = "P2pPreSyncId_";

PasteboardDistributedManager::PasteboardDistributedManager(PasteboardService &service) : service_(service)
{
    p2pEstablishInfo_.pasteBlock = nullptr;
    moduleConfig_.Init();
}

bool PasteboardDistributedManager::RemoteDataTaskManager::IsRemoteDataPasting(const Event &event)
{
    auto key = event.deviceId + std::to_string(event.seqId);
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = dataTasks_.find(key);
    if (it == dataTasks_.end() || it->second == nullptr) {
        return false;
    }
    return it->second->pasting_;
}

bool PasteboardDistributedManager::RemoteDataTaskManager::HasRunningTask()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &[key, task] : dataTasks_) {
        if (task != nullptr && task->pasting_) {
            return true;
        }
    }
    return false;
}

PasteboardDistributedManager::RemoteDataTaskManager::DataTask PasteboardDistributedManager::RemoteDataTaskManager::GetRemoteDataTask(
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

void PasteboardDistributedManager::RemoteDataTaskManager::Notify(const Event &event, std::shared_ptr<PasteDateTime> data)
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

std::shared_ptr<PasteDateTime> PasteboardDistributedManager::RemoteDataTaskManager::WaitRemoteData(const Event &event)
{
    std::shared_ptr<PasteboardDistributedManager::RemoteDataTaskManager::TaskContext> task;
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

void PasteboardDistributedManager::RemoteDataTaskManager::ClearRemoteDataTask(const Event &event)
{
    auto key = event.deviceId + std::to_string(event.seqId);
    std::lock_guard<std::mutex> lock(mutex_);
    dataTasks_.erase(key);
}

void PasteboardDistributedManager::ClearP2PEstablishTaskInfo()
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pEstablishInfo_.networkId.clear();
    p2pEstablishInfo_.pasteBlock = nullptr;
}

void PasteboardDistributedManager::OpenP2PLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
#endif
    auto plugin = GetClipPlugin();
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
    int32_t status = plugin->ApplyAdvancedResource(networkId);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == RESULT_OK, PASTEBOARD_MODULE_SERVICE,
        "apply resource failed, deviceId=%{public}.5s, status=%{public}d", networkId.c_str(), status);

    status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == RESULT_OK, PASTEBOARD_MODULE_SERVICE,
        "publish CONNECT_SUCC failed, deviceId=%{public}.5s, status=%{public}d", networkId.c_str(), status);

#ifdef PB_DEVICE_MANAGER_ENABLE
    status = DistributedFileDaemonManager::GetInstance().ConnectDfs(networkId);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status:%{public}d", status);
        plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Erase(networkId);
        return;
    }
#endif
}

void PasteboardDistributedManager::EstablishP2PLink(const std::string &networkId, const std::string &pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Compute(networkId, [pasteId, callPid](const auto &key, auto &value) {
            value.Compute(pasteId, [callPid](const auto &key, auto &value) {
                value.callPid = callPid;
                value.isSuccess = false;
                return true;
            });
            return true;
        });
    }
    if (ffrtTimer_) {
        FFRTTask task = [this, networkId, pasteId] {
            std::thread thread([=]() {
                service_.PasteComplete(networkId, pasteId);
            });
            PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete01");
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    OpenP2PLink(networkId);
#endif
}

std::shared_ptr<BlockObject<int32_t>> PasteboardDistributedManager::CheckAndReuseP2PLink(
    const std::string &networkId, const std::string &pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [pasteId, callPid](const auto &key, auto &value) {
        value.Compute(pasteId, [callPid](const auto &key, auto &value) {
            value.callPid = callPid;
            value.isSuccess = false;
            return true;
        });
        return true;
    });
    if (ffrtTimer_) {
        FFRTTask task = [this, networkId, pasteId] {
            std::thread thread([=]() {
                service_.PasteComplete(networkId, pasteId);
            });
            PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete02");
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    auto p2pNetwork = p2pMap_.Find(networkId);
    bool isP2pSuccess = p2pNetwork.first && p2pNetwork.second.Find(P2P_PRESYNC_ID).first &&
        p2pNetwork.second.Find(P2P_PRESYNC_ID).second.isSuccess == true;
    if (isP2pSuccess) {
        if (ffrtTimer_) {
            std::string taskName = P2P_PRESYNC_ID + networkId;
            ffrtTimer_->CancelTimer(taskName);
        }
        p2pMap_.ComputeIfPresent(networkId, [this](const auto &key, auto &value) {
            value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
                return false;
            });
            return true;
        });
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "No Need P2pEstablish");
        std::shared_ptr<BlockObject<int32_t>> result = nullptr;
        auto p2pIter = preSyncP2pMap_.find(networkId);
        if (p2pIter != preSyncP2pMap_.end()) {
            result = p2pIter->second;
        }
        preSyncP2pMap_.erase(networkId);
        return result;
    }
    return nullptr;
#else
    return nullptr;
#endif
}

bool PasteboardDistributedManager::IsContainUri(const Event &evt)
{
    if (evt.notNeedLink && !evt.isDelay) {
        return false;
    }
    std::vector<std::string> keyVecs;
    keyVecs.push_back(MIMETYPE_TEXT_URI);
    keyVecs.push_back(MIMETYPE_TEXT_HTML);
    bool result = std::any_of(keyVecs.begin(), keyVecs.end(), [dataType = evt.dataType](const std::string &key) {
        return std::find(dataType.begin(), dataType.end(), key) != dataType.end();
    });
    return result;
}

void PasteboardDistributedManager::OnEstablishP2PLinkTask(const std::string &networkId,
    std::shared_ptr<BlockObject<int32_t>> pasteBlock)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteBlock != nullptr, PASTEBOARD_MODULE_SERVICE, "block is nullptr");
    OpenP2PLink(networkId);
    pasteBlock->SetValue(SET_VALUE_SUCCESS);
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    auto findResult = p2pMap_.Find(networkId);
    if (!findResult.first || findResult.second.Empty()) {
        CloseP2PLink(networkId);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "P2pEstablish Finish");
}

std::shared_ptr<BlockObject<int32_t>> PasteboardDistributedManager::EstablishP2PLinkTask(
    const std::string &pasteId, const ClipPlugin::GlobalEvent &event)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    const std::string &networkId = event.deviceId;
    if (networkId.empty() || networkId == DMAdapter::GetInstance().GetLocalNetworkId()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "local device");
        return nullptr;
    }
    if (!IsContainUri(event)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "no MIMETYPE_TEXT_URI and no MIMETYPE_TEXT_HTML");
        return nullptr;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "EstablishP2PLinkTask enter");
    std::shared_ptr<BlockObject<int32_t>> result = CheckAndReuseP2PLink(networkId, pasteId);
    if (result) {
        return result;
    }
    if (!ffrtTimer_) {
        return nullptr;
    }
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
    if (!pasteBlock) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc BlockObject");
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pEstablishInfo_.networkId = networkId;
        p2pEstablishInfo_.pasteBlock = pasteBlock;
    }
    FFRTTask p2pTask = [networkId, pasteBlock, this] {
        std::thread thread([=]() {
            OnEstablishP2PLinkTask(networkId, pasteBlock);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "OnEstablishP2P");
        thread.detach();
    };
    std::string taskName = pasteId + P2P_ESTABLISH_STR;
    ffrtTimer_->SetTimer(taskName, p2pTask);
    return pasteBlock;
#else
    return nullptr;
#endif
}

void PasteboardDistributedManager::CloseP2PLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "CloseP2PLink enter");
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist");
        return;
    }
    std::thread thread([networkId]() {
        auto status = DistributedFileDaemonManager::GetInstance().DisconnectDfs(networkId);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "close p2p error, status:%{public}d", status);
        }
    });
    thread.detach();
    auto plugin = GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(plugin != nullptr, PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
    auto status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state idle error, status:%{public}d", status);
    }
#endif
}

int32_t PasteboardDistributedManager::GetRemoteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime)
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
        auto it = service_.clips_.Find(userId);
        if (it.first) {
            data = *it.second;
            ret = static_cast<int32_t>(PasteboardError::E_OK);
        }
        taskMgr_.ClearRemoteDataTask(event);
        return ret;
    }

    return GetRemotePasteData(userId, event, data, syncTime);
}

int32_t PasteboardDistributedManager::GetRemotePasteData(int32_t userId, const Event &event, PasteData &data, int32_t &syncTime)
{
    auto block = std::make_shared<BlockObject<std::shared_ptr<PasteDateTime>>>(GET_REMOTE_DATA_WAIT_TIME);
    std::thread thread([this, event, block, userId]() mutable {
        auto result = GetDistributedData(event, userId);
        auto [distRet, distEvt] = GetValidDistributeEvent(userId);
        std::shared_ptr<PasteDateTime> pasteDataTime = std::make_shared<PasteDateTime>();
        if (result.first != nullptr) {
            result.first->SetRemote(true);
            if (distEvt == event) {
                service_.clips_.InsertOrAssign(userId, result.first);
                service_.IncreaseChangeCount(userId);
                auto curTime =
                    static_cast<uint64_t>(PasteBoardTime::GetBootTimeMs());
                service_.copyTime_.InsertOrAssign(userId, curTime);
                service_.SetDataExpirationTimer(userId);
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

void PasteboardDistributedManager::GetDelayPasteData(int32_t userId, PasteData &data)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "get delay data start");
    service_.delayGetters_.ComputeIfPresent(userId, [this, &data, userId](auto, auto &delayGetter) {
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

int32_t PasteboardDistributedManager::GetDelayPasteRecord(int32_t userId, PasteData &data)
{
    auto [hasGetter, getter] = service_.entryGetters_.Find(userId);
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

int32_t PasteboardDistributedManager::ProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo,
    PasteDataEntry &entry)
{
    const auto &targetBundle = targetAppInfo.bundleName;
    const auto &appIndex = targetAppInfo.appIndex;
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

    std::map<uint32_t, std::vector<Uri>> grantUris = service_.CheckUriPermission(tmp, std::make_pair(targetBundle, appIndex));
    int32_t ret = service_.GrantUriPermission(grantUris, targetBundle, isRemoteData, appIndex);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "grant to %{public}s:%{public}d failed, ret=%{public}d", targetBundle.c_str(),
        appIndex, ret);

    return PostProcessDelayHtmlEntry(tmp, targetAppInfo, entry);
}

int32_t PasteboardDistributedManager::PostProcessDelayHtmlEntry(PasteData &data, const AppInfo &targetAppInfo,
    PasteDataEntry &entry)
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

int32_t PasteboardDistributedManager::GetFullDelayPasteData(int32_t userId, PasteData &data)
{
    auto [hasGetter, getter] = service_.entryGetters_.Find(userId);
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
    service_.clips_.ComputeIfPresent(userId, [&data](auto, auto &value) {
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

int32_t PasteboardDistributedManager::GetLocalEntryValue(int32_t userId, PasteData &data, PasteDataRecord &record,
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

    auto [hasGetter, getter] = service_.entryGetters_.Find(userId);
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

int32_t PasteboardDistributedManager::GetRemoteEntryValue(const AppInfo &appInfo, PasteData &data, PasteDataRecord &record,
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

int32_t PasteboardDistributedManager::ProcessRemoteDelayUri(const std::string &deviceId, const AppInfo &appInfo,
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
    std::map<uint32_t, std::vector<Uri>> grantUris = service_.CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(deviceId, data.GetPasteId());
        int32_t ret = service_.GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant remote uri failed, uri=%{private}s, ret=%{public}d",
            distributedUri.c_str(), ret);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDistributedManager::ProcessRemoteDelayHtml(const std::string &remoteDeviceId, const AppInfo &appInfo,
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

int32_t PasteboardDistributedManager::ProcessRemoteDelayHtmlInner(const std::string &remoteDeviceId, const AppInfo &appInfo,
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

    std::map<uint32_t, std::vector<Uri>> grantUris = service_.CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(remoteDeviceId, data.GetPasteId());
        int32_t ret = service_.GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);
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

bool PasteboardDistributedManager::SetDistributedData(int32_t user, PasteData &data)
{
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!networkId.empty(), false, PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
    Event event;
    event.user = user;
    event.seqId = ++service_.sequenceId_;
    auto expiration = PasteBoardTime::GetBootTimeMs() + EXPIRATION_INTERVAL;
    event.expiration = static_cast<uint64_t>(expiration);
    event.deviceId = networkId;
    event.account = AccountManager::GetInstance().GetCurrentAccount();
    event.status = ClipPlugin::EVT_NORMAL;
    event.dataType = data.GetMimeTypes();
    event.isDelay = data.IsDelayRecord();
    event.dataId = data.GetDataId();
    SetCurrentEvent(event);

    if (service_.IsConstraintEnabled(user) || IsDisallowDistributed()) {
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
    if (service_.CheckMdmShareOption(data)) {
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::LocalDevice, false, PASTEBOARD_MODULE_SERVICE,
            "data share option is local device, dataId:%{public}u", data.GetDataId());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, isDelay:%{public}d,"
        "expiration:%{public}" PRIu64, event.dataId, event.seqId, event.isDelay, event.expiration);
    return SetCurrentDistributedData(data, event);
}

bool PasteboardDistributedManager::SetCurrentDistributedData(PasteData &data, Event event)
{
    std::thread thread([this, data, event]() mutable {
        {
            std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
            setDistributedMemory_.latestEvent = event;
            setDistributedMemory_.latestData = std::make_shared<PasteData>(data);
            if (setDistributedMemory_.isRunning) {
                return;
            }
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
            PASTEBOARD_CHECK_AND_RETURN_LOGE(ret, PASTEBOARD_MODULE_SERVICE, "timeout,seqId:%{public}hu", event.seqId);
        }
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "SetDistributeDa");
    thread.detach();
    return true;
}

bool PasteboardDistributedManager::SetCurrentData()
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
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
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
            std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
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
        std::shared_lock<std::shared_mutex> read(PasteboardService::pasteDataMutex_);
        if (!currentData.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "distributed data encode failed, dataId:%{public}u, seqId:%{public}hu",
                currentEvent.dataId, currentEvent.seqId);
            return false;
        }
    }
    if (currentData.IsDelayRecord() && !needFull) {
        clipPlugin->RegisterDelayCallback(
            std::bind(&PasteboardDistributedManager::GetDistributedDelayData, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3),
            std::bind(&PasteboardDistributedManager::GetDistributedDelayEntry, this, std::placeholders::_1,
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

void PasteboardDistributedManager::GenerateDistributedUri(PasteData &data)
{
    std::vector<std::string> uris;
    std::vector<size_t> indexes;
    auto userId = service_.GetAppInfo(IPCSkeleton::GetCallingTokenID()).userId;
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
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
        if (!service_.IsBundleOwnUriPermission(bundleName, *uri) && !hasGrantUriPermission) {
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

int32_t PasteboardDistributedManager::SyncDelayedData()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = service_.GetAppInfo(tokenId);
    auto [hasData, data] = service_.clips_.Find(appInfo.userId);
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
        service_.clips_.ComputeIfPresent(userId, [=](auto, auto &value) {
            if (data->GetDataId() == value->GetDataId()) {
                value = std::move(data);
            }
            return true;
        });
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "SyncDelayedData");
    thread.detach();
    return ERR_OK;
}

std::pair<int32_t, ClipPlugin::GlobalEvent> PasteboardDistributedManager::GetValidDistributeEvent(int32_t user)
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

std::pair<std::shared_ptr<PasteData>, PasteDateResult> PasteboardDistributedManager::GetDistributedData(
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
    if (static_cast<int64_t>(rawData.size()) > service_.maxLocalCapacity_.load()) {
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

int32_t PasteboardDistributedManager::GetDistributedDelayData(const Event &evt, uint8_t version, std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64,
        evt.dataId, evt.seqId, evt.expiration);
    auto [hasData, data] = service_.clips_.Find(evt.user);
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

    auto authorityInfo = data->GetOriginAuthority();
    data->SetBundleInfo(authorityInfo.first, authorityInfo.second);
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authorityInfo);
        PasteboardWebController::GetInstance().SplitWebviewPasteData(*data, bundleIndex, evt.user);
        PasteboardWebController::GetInstance().SetWebviewPasteData(*data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(*data);
    }
    GenerateDistributedUri(*data);

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    std::shared_lock<std::shared_mutex> read(PasteboardService::pasteDataMutex_);
    bool encodeSucc = data->Encode(rawData, remoteVersionMin <= DistributedModuleConfig::Version::VERSION_FIVE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode data failed, dataId:%{public}u, seqId:%{public}hu", evt.dataId, evt.seqId);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "size=%{public}zu", rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDistributedManager::GetDistributedDelayEntry(const Event &evt, uint32_t recordId, const std::string &utdId,
    std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64
        ", recordId:%{public}u, type:%{public}s", evt.dataId, evt.seqId, evt.expiration, recordId, utdId.c_str());
    auto [hasData, data] = service_.clips_.Find(evt.user);
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
        ret = ProcessDistributedDelayUri(evt.user, *data, entry, rawData);
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

int32_t PasteboardDistributedManager::ProcessDistributedDelayUri(int32_t userId, PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");

    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
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

int32_t PasteboardDistributedManager::ProcessDistributedDelayHtml(PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    {
        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_)) {
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }

    PasteData tmp;
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
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

int32_t PasteboardDistributedManager::ProcessDistributedDelayEntry(PasteDataEntry &entry, std::vector<uint8_t> &rawData)
{
    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode entry failed, type=%{public}s", entry.GetUtdId().c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void PasteboardDistributedManager::DeletePreSyncP2pFromP2pMap(const std::string &networkId)
{
    std::string taskName = P2P_PRESYNC_ID + networkId;
    if (ffrtTimer_) {
        ffrtTimer_->CancelTimer(taskName);
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(networkId, [this](const auto &key, auto &value) {
        value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
            return false;
        });
        return true;
    });
    DeletePreSyncP2pMap(networkId);
}

void PasteboardDistributedManager::DeletePreSyncP2pMap(const std::string &networkId)
{
    auto p2pIter = preSyncP2pMap_.find(networkId);
    if (p2pIter != preSyncP2pMap_.end()) {
        if (p2pIter->second) {
            p2pIter->second->SetValue(SET_VALUE_SUCCESS);
        }
        preSyncP2pMap_.erase(networkId);
    }
}

void PasteboardDistributedManager::AddPreSyncP2pTimeoutTask(const std::string &networkId)
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    std::string taskName = P2P_PRESYNC_ID + networkId;
    ffrtTimer_->CancelTimer(taskName);
    FFRTTask p2pTask = [this, networkId] {
        std::thread thread([=]() {
            service_.PasteComplete(networkId, P2P_PRESYNC_ID);
            std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
            DeletePreSyncP2pMap(networkId);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete03");
        thread.detach();
    };
    ffrtTimer_->SetTimer(taskName, p2pTask, PRE_ESTABLISH_P2P_LINK_TIME);
}

void PasteboardDistributedManager::InitPlugin(std::shared_ptr<ClipPlugin> clipPlugin)
{
    if (!clipPlugin) {
        return;
    }
    clipPlugin->RegisterPreSyncCallback(std::bind(&PasteboardDistributedManager::PreEstablishP2PLinkCallback,
        this, std::placeholders::_1, std::placeholders::_2));
    clipPlugin->RegisterPreSyncMonitorCallback(std::bind(&PasteboardDistributedManager::PreSyncSwitchMonitorCallback, this));
    clipPlugin->SetMaxLocalCapacity(service_.maxLocalCapacity_.load() / SIZE_K / SIZE_K);
}

bool PasteboardDistributedManager::OpenP2PLinkForPreEstablish(const std::string &networkId, ClipPlugin *clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        DeletePreSyncP2pFromP2pMap(networkId);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist, ret:%{public}d", ret);
        return false;
    }
    auto status = DistributedFileDaemonManager::GetInstance().ConnectDfs(networkId);
    if (status != RESULT_OK) {
        DeletePreSyncP2pFromP2pMap(networkId);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status:%{public}d", status);
        return false;
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [](const auto &key, auto &value) {
        value.Compute(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
            value.isSuccess = true;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "preP2pLink isSuccess:%{public}d", value.isSuccess);
            return true;
        });
        return true;
    });
    if (clipPlugin) {
        status = clipPlugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state connect_succ error, status:%{public}d", status);
        }
    }
    AddPreSyncP2pTimeoutTask(networkId);
    return true;
#else
    return false;
#endif
}

void PasteboardDistributedManager::PreEstablishP2PLink(const std::string &networkId, ClipPlugin *clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink enter");
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = nullptr;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        if (p2pEstablishInfo_.pasteBlock && p2pEstablishInfo_.networkId == networkId) {
            return;
        }
        auto p2pNetwork = p2pMap_.Find(networkId);
        bool isP2pSuccess = p2pNetwork.first && p2pNetwork.second.Find(P2P_PRESYNC_ID).first &&
            p2pNetwork.second.Find(P2P_PRESYNC_ID).second.isSuccess == true;
        if (isP2pSuccess) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Pre P2pEstablish exist");
            AddPreSyncP2pTimeoutTask(networkId);
            return;
        }
        pasteBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
        if (!pasteBlock) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc BlockObject");
            return;
        }
        p2pMap_.Compute(networkId, [this](const auto &key, auto &value) {
            value.Compute(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
                value.callPid = 0;
                value.isSuccess = false;
                return true;
            });
            return true;
        });
        preSyncP2pMap_.emplace(networkId, pasteBlock);
    }
    if (OpenP2PLinkForPreEstablish(networkId, clipPlugin)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink Finish");
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink failed");
    }
    pasteBlock->SetValue(SET_VALUE_SUCCESS);
#endif
}

void PasteboardDistributedManager::PreEstablishP2PLinkCallback(const std::string &networkId, ClipPlugin *clipPlugin)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLinkCallback enter");
    if (networkId.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLinkCallback failed, networkId is null");
        return;
    }
    if (!clipPlugin) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        return;
    }
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
#ifdef PB_DEVICE_MANAGER_ENABLE
    FFRTTask p2pTask = [this, networkId, clipPlugin] {
        std::thread thread([=]() {
            PreEstablishP2PLink(networkId, clipPlugin);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PreEstablishP2P");
        thread.detach();
    };
    std::string taskName = "PreEstablishP2PLink_";
    taskName += networkId;
    ffrtTimer_->SetTimer(taskName, p2pTask);
#endif
}

void PasteboardDistributedManager::PreSyncRemotePasteboardData()
{
    auto clipPlugin = GetClipPlugin();
    if (!clipPlugin) {
        return;
    }
    if (!clipPlugin->NeedSyncTopEvent()) {
        return;
    }
    const int32_t DEFAULT_USER_ID = 0;
    clipPlugin->SendPreSyncEvent(DEFAULT_USER_ID);
}

void PasteboardDistributedManager::PreSyncSwitchMonitorCallback()
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    FFRTTask monitorTask = [this] {
        std::thread thread([=]() {
            RegisterPreSyncMonitor();
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PreSyncSwitchMo");
        thread.detach();
    };
    ffrtTimer_->SetTimer(REGISTER_PRESYNC_MONITOR, monitorTask);
}

void PasteboardDistributedManager::RegisterPreSyncMonitor()
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    if (!MMI::InputManager::GetInstance()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "MMI::InputManager is null");
        return;
    }
    FFRTTask monitorTask = [this] {
        std::thread thread([=]() {
            UnRegisterPreSyncMonitor();
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "RegisterPreSync");
        thread.detach();
    };
    if (subscribeActiveId_ != INVALID_SUBSCRIBE_ID) {
        ffrtTimer_->SetTimer(UNREGISTER_PRESYNC_MONITOR, monitorTask, PRESYNC_MONITOR_TIME);
        return;
    }
    std::shared_ptr<InputEventCallback> preSyncMonitor =
        std::make_shared<InputEventCallback>(InputEventCallback::INPUTTYPE_PRESYNC, &service_);
    if (!preSyncMonitor) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc InputEventCallback");
        return;
    }
    subscribeActiveId_ = MMI::InputManager::GetInstance()->SubscribeInputActive(
        std::static_pointer_cast<MMI::IInputEventConsumer>(preSyncMonitor), PRESYNC_MONITOR_INTERVAL_MILLISECONDS);
    if (subscribeActiveId_ < 0) {
        subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SubscribeInputActive failed");
        return;
    }
    ffrtTimer_->SetTimer(UNREGISTER_PRESYNC_MONITOR, monitorTask, PRESYNC_MONITOR_TIME);
}

void PasteboardDistributedManager::UnRegisterPreSyncMonitor()
{
    if (!MMI::InputManager::GetInstance()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "MMI::InputManager is null");
        return;
    }
    if (subscribeActiveId_ != INVALID_SUBSCRIBE_ID) {
        MMI::InputManager::GetInstance()->UnsubscribeInputActive(subscribeActiveId_);
        subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
    }
}

std::shared_ptr<ClipPlugin> PasteboardDistributedManager::GetClipPlugin()
{
    auto isOn = moduleConfig_.IsOn();
    if (isOn) {
        auto securityLevel = service_.securityLevel_.GetDeviceSecurityLevel();
#ifdef PB_DATACLASSIFICATION_ENABLE
        if (securityLevel < DATA_SEC_LEVEL3) {
            return nullptr;
        }
#endif
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

void PasteboardDistributedManager::OnConfigChange(bool isOn)
{
    std::thread thread([=]() {
        OnConfigChangeInner(isOn);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "OnConfigChange");
    thread.detach();
}

void PasteboardDistributedManager::OnConfigChangeInner(bool isOn)
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
        int32_t userId = service_.ResolveMainDisplayUserId();
        PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE,
            "main display user invalid");
        clipPlugin_->Close(userId);
        clipPlugin_ = nullptr;
        return;
    }
    service_.SetCriticalTimer();
    auto securityLevel = service_.securityLevel_.GetDeviceSecurityLevel();
#ifdef PB_DATACLASSIFICATION_ENABLE
    if (securityLevel < DATA_SEC_LEVEL3) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "device sec level is %{public}u less than 3.", securityLevel);
        return;
    }
#endif
    if (clipPlugin_ != nullptr) {
        return;
    }
    service_.SubscribeKeyboardEvent();
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    InitPlugin(clipPlugin_);
}

std::function<void(const OHOS::MiscServices::Event &)> PasteboardDistributedManager::RemotePasteboardChange()
{
    return [this](const OHOS::MiscServices::Event &event) {
        (void)event;
        std::lock_guard<std::mutex> lock(service_.observerMutex_);
        for (auto &observers : service_.observerRemoteChangedMap_) {
            for (const auto &observer : *(observers.second)) {
                observer->OnPasteboardChanged();
            }
        }
    };
}

bool PasteboardDistributedManager::HasRemoteUri(std::shared_ptr<PasteData> data)
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

bool PasteboardDistributedManager::IsDisallowDistributed()
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    if (uid == DEVICE_COLLABORATION_UID) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uid from device collaboration");
        return true;
    }
    return false;
}

bool PasteboardDistributedManager::IsNeedLink(PasteData &data)
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

void PasteboardDistributedManager::CleanDistributedData(int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->Clear(user);
}

bool PasteboardDistributedManager::IsValidCurrentEvent()
{
    auto expiration = PasteBoardTime::GetBootTimeMs();
    auto currentEvent = GetCurrentEvent();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(static_cast<uint64_t>(expiration) < currentEvent.expiration,
        false, PASTEBOARD_MODULE_SERVICE, "event is invalid");
    return true;
}

void PasteboardDistributedManager::CloseDistributedStore(int32_t user, bool isNeedClear)
{
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
    if (isNeedClear) {
        clipPlugin_->Clear(user);
    }
    clipPlugin_->Close(user);
}

void PasteboardDistributedManager::ChangeStoreStatus(int32_t userId)
{
    PasteboardService::currentUserId_.store(userId);
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->ChangeStoreStatus(userId);
}

void PasteboardDistributedManager::HandleWifiOffAndClearDistributedEvent(int32_t userId)
{
    bool isdeviceCollabSwitch = service_.switch_.GetDeviceCollabSwitch(userId);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(!isdeviceCollabSwitch, PASTEBOARD_MODULE_SERVICE,
        "wifi off but DeviceCollabSwitch is on");
    PASTEBOARD_CHECK_AND_RETURN_LOGD(IsValidCurrentEvent(), PASTEBOARD_MODULE_SERVICE, "wifi off but no valid event");
    CleanDistributedData(userId);
}

ClipPlugin::GlobalEvent PasteboardDistributedManager::GetCurrentEvent() const
{
    std::lock_guard<std::mutex> lock(currentEventMutex_);
    return currentEvent_;
}

void PasteboardDistributedManager::SetCurrentEvent(ClipPlugin::GlobalEvent event)
{
    std::lock_guard<std::mutex> lock(currentEventMutex_);
    currentEvent_ = std::move(event);
}
} // namespace MiscServices
} // namespace OHOS