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

#include "pasteboard_p2p_manager.h"

#include "common/pasteboard_common_utils.h"
#include "device/dm_adapter.h"
#include "distributed_file_daemon_manager.h"
#include "ffrt/ffrt_utils.h"
#include "ipc_skeleton.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

namespace {
constexpr int32_t RESULT_OK = 0;
constexpr int32_t SET_VALUE_SUCCESS = 0;
constexpr int32_t MIN_TRANMISSION_TIME = 10000;
constexpr int32_t PRE_ESTABLISH_P2P_LINK_TIME = 15000;
constexpr int32_t PRESYNC_MONITOR_TIME = 10000;
constexpr int32_t PRESYNC_MONITOR_INTERVAL_MILLISECONDS = 1000;
constexpr int32_t INVALID_SUBSCRIBE_ID = -1;
const std::string P2P_PRESYNC_ID = "P2PPresyncId";
const std::string P2P_ESTABLISH_STR = "P2PEstablish";
const std::string REGISTER_PRESYNC_MONITOR = "RegisterPreSyncMonitor";
const std::string UNREGISTER_PRESYNC_MONITOR = "UnregisterPreSyncMonitor";
}

PasteboardP2PManager::PasteboardP2PManager(PasteboardService& service)
    : service_(service), subscribeActiveId_(INVALID_SUBSCRIBE_ID)
{
    ffrtTimer_ = FFRTPool::GetTimer("pasteboard_p2p_manager");
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardP2PManager constructed.");
}

PasteboardP2PManager::~PasteboardP2PManager()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PasteboardP2PManager destructed.");
}

void PasteboardP2PManager::ClearP2PEstablishTaskInfo()
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pEstablishInfo_.networkid.clear();
    p2pEstablishInfo_.pasteBlock = nullptr;
}

void PasteboardP2PManager::OpenP2PLink(const std::string& networkId)
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
    auto plugin = service_.distributedManager_->GetClipPlugin();
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

void PasteboardP2PManager::EstablishP2PLink(const std::string& networkId, const std::string& pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.Compute(networkId, [pasteId, callPid](const auto& key, auto& value) {
            value.Compute(pasteId, [callPid](const auto& key, auto& value) {
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

std::shared_ptr<BlockObject<int32_t>> PasteboardP2PManager::CheckAndReuseP2PLink(
    const std::string& networkId, const std::string& pasteId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto callPid = IPCSkeleton::GetCallingPid();
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [pasteId, callPid](const auto& key, auto& value) {
        value.Compute(pasteId, [callPid](const auto& key, auto& value) {
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
        p2pMap_.ComputeIfPresent(networkId, [this](const auto& key, auto& value) {
            value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto& key, auto& value) {
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

bool PasteboardP2PManager::IsContainUri(const Event& evt)
{
    if (evt.notNeedLink && !evt.isDelay) {
        return false;
    }
    std::vector<std::string> keyVecs;
    keyVecs.push_back(MIMETYPE_TEXT_URI);
    keyVecs.push_back(MIMETYPE_TEXT_HTML);
    bool result = std::any_of(keyVecs.begin(), keyVecs.end(), [dataType = evt.dataType](const std::string& key) {
        return std::find(dataType.begin(), dataType.end(), key) != dataType.end();
    });
    return result;
}

void PasteboardP2PManager::OnEstablishP2PLinkTask(const std::string& networkId,
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

std::shared_ptr<BlockObject<int32_t>> PasteboardP2PManager::EstablishP2PLinkTask(
    const std::string& pasteId, const ClipPlugin::GlobalEvent& event)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    const std::string& networkId = event.deviceId;
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
        p2pEstablishInfo_.networkid = networkId;
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

void PasteboardP2PManager::CloseP2PLink(const std::string& networkId)
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
    auto plugin = service_.distributedManager_->GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(plugin != nullptr, PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
    auto status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state idle error, status:%{public}d", status);
    }
#endif
}

void PasteboardP2PManager::DeletePreSyncP2pFromP2pMap(const std::string& networkId)
{
    std::string taskName = P2P_PRESYNC_ID + networkId;
    if (ffrtTimer_) {
        ffrtTimer_->CancelTimer(taskName);
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(networkId, [this](const auto& key, auto& value) {
        value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto& key, auto& value) {
            return false;
        });
        return true;
    });
    DeletePreSyncP2pMap(networkId);
}

void PasteboardP2PManager::DeletePreSyncP2pMap(const std::string& networkId)
{
    auto p2pIter = preSyncP2pMap_.find(networkId);
    if (p2pIter != preSyncP2pMap_.end()) {
        if (p2pIter->second) {
            p2pIter->second->SetValue(SET_VALUE_SUCCESS);
        }
        preSyncP2pMap_.erase(networkId);
    }
}

void PasteboardP2PManager::AddPreSyncP2pTimeoutTask(const std::string& networkId)
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

bool PasteboardP2PManager::OpenP2PLinkForPreEstablish(const std::string& networkId, ClipPlugin* clipPlugin)
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
    p2pMap_.Compute(networkId, [](const auto& key, auto& value) {
        value.Compute(P2P_PRESYNC_ID, [](const auto& key, auto& value) {
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

void PasteboardP2PManager::PreEstablishP2PLink(const std::string& networkId, ClipPlugin* clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink enter");
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = nullptr;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        if (p2pEstablishInfo_.pasteBlock && p2pEstablishInfo_.networkid == networkId) {
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
        p2pMap_.Compute(networkId, [this](const auto& key, auto& value) {
            value.Compute(P2P_PRESYNC_ID, [](const auto& key, auto& value) {
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

void PasteboardP2PManager::PreEstablishP2PLinkCallback(const std::string& networkId, ClipPlugin* clipPlugin)
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

void PasteboardP2PManager::PreSyncRemotePasteboardData()
{
    auto clipPlugin = service_.distributedManager_->GetClipPlugin();
    if (!clipPlugin) {
        return;
    }
    if (!clipPlugin->NeedSyncTopEvent()) {
        return;
    }
    const int32_t DEFAULT_USER_ID = 0;
    clipPlugin->SendPreSyncEvent(DEFAULT_USER_ID);
}

void PasteboardP2PManager::PreSyncSwitchMonitorCallback()
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

void PasteboardP2PManager::RegisterPreSyncMonitor()
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

void PasteboardP2PManager::UnRegisterPreSyncMonitor()
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

bool PasteboardP2PManager::IsNeedLink(PasteData& data)
{
    for (const auto& record : data.AllRecords()) {
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

int32_t PasteboardP2PManager::OnPasteComplete(const std::string& deviceId, const std::string& pasteId)
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(deviceId, [pasteId, deviceId, this](const auto& key, auto& value) {
        value.ComputeIfPresent(pasteId, [deviceId](const auto& key, auto& value) {
            return false;
        });
        if (value.Empty()) {
            CloseP2PLink(deviceId);
            return false;
        }
        return true;
    });
    return 0;
}

void PasteboardP2PManager::ClearAllP2PLinks()
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ForEach([this](const auto& deviceId, auto& value) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "configChange is off, need close p2p link.");
        CloseP2PLink(deviceId);
        return false;
    });
    p2pMap_.Clear();
}

void PasteboardP2PManager::RemoveP2PLinkByNetworkId(const std::string& networkId)
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.EraseIf([networkId, this](auto& key, auto& value) {
        if (key == networkId) {
            CloseP2PLink(networkId);
            return true;
        }
        return false;
    });
}

void PasteboardP2PManager::RemoveP2PLinksByPid(pid_t pid, std::vector<std::string>& networkIds)
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.EraseIf([pid, &networkIds, this](auto& networkId, auto& pidMap) {
        pidMap.EraseIf([pid, this, networkId](const auto& key, const auto& value) {
            if (value.callPid == pid) {
                service_.PasteStart(networkId);
                return true;
            }
            return false;
        });
        if (pidMap.Empty()) {
            networkIds.emplace_back(networkId);
            return true;
        }
        return false;
    });
}

} // namespace MiscServices
} // namespace OHOS