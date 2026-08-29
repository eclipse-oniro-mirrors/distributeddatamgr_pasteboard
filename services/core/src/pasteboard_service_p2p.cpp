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

#include "distributed_file_daemon_manager.h"
#include "ipc_skeleton.h"
#include "common/pasteboard_common_utils.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
using namespace Storage::DistributedFile;
namespace {
constexpr int32_t SET_VALUE_SUCCESS = 1;
} // namespace

const std::string PasteboardService::P2P_ESTABLISH_STR = "P2pEstablish";
const std::string PasteboardService::P2P_PRESYNC_ID = "P2pPreSyncId_";

void PasteboardService::ClearP2PEstablishTaskInfo()
{
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pEstablishInfo_.networkId.clear();
    p2pEstablishInfo_.pasteBlock = nullptr;
}

void PasteboardService::OpenP2PLink(const std::string &networkId)
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

void PasteboardService::EstablishP2PLink(const std::string &networkId, const std::string &pasteId)
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
                PasteComplete(networkId, pasteId);
            });
            PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete01");
            thread.detach();
        };
        ffrtTimer_->SetTimer(pasteId, task, MIN_TRANMISSION_TIME);
    }
    OpenP2PLink(networkId);
#endif
}

std::shared_ptr<BlockObject<int32_t>> PasteboardService::CheckAndReuseP2PLink(
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
                PasteComplete(networkId, pasteId);
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

bool PasteboardService::IsContainUri(const Event &evt)
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

void PasteboardService::OnEstablishP2PLinkTask(const std::string &networkId,
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

std::shared_ptr<BlockObject<int32_t>> PasteboardService::EstablishP2PLinkTask(
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

void PasteboardService::CloseP2PLink(const std::string &networkId)
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

void PasteboardService::DeletePreSyncP2pFromP2pMap(const std::string &networkId)
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

void PasteboardService::DeletePreSyncP2pMap(const std::string &networkId)
{
    auto p2pIter = preSyncP2pMap_.find(networkId);
    if (p2pIter != preSyncP2pMap_.end()) {
        if (p2pIter->second) {
            p2pIter->second->SetValue(SET_VALUE_SUCCESS);
        }
        preSyncP2pMap_.erase(networkId);
    }
}

void PasteboardService::AddPreSyncP2pTimeoutTask(const std::string &networkId)
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    std::string taskName = P2P_PRESYNC_ID + networkId;
    ffrtTimer_->CancelTimer(taskName);
    FFRTTask p2pTask = [this, networkId] {
        std::thread thread([=]() {
            PasteComplete(networkId, P2P_PRESYNC_ID);
            std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
            DeletePreSyncP2pMap(networkId);
        });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "PasteComplete03");
        thread.detach();
    };
    ffrtTimer_->SetTimer(taskName, p2pTask, PRE_ESTABLISH_P2P_LINK_TIME);
}

bool PasteboardService::OpenP2PLinkForPreEstablish(const std::string &networkId, ClipPlugin *clipPlugin)
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

void PasteboardService::PreEstablishP2PLink(const std::string &networkId, ClipPlugin *clipPlugin)
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

void PasteboardService::PreEstablishP2PLinkCallback(const std::string &networkId, ClipPlugin *clipPlugin)
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

} // namespace MiscServices
} // namespace OHOS
