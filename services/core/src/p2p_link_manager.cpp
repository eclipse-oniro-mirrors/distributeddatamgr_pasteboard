/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "p2p_link_manager.h"

#include <thread>

#include "clip/clip_plugin.h"
#include "common/pasteboard_common_utils.h"
#include "device/dm_adapter.h"
#include "distributed_file_daemon_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
namespace {
constexpr int32_t RESULT_OK = 0;
using namespace OHOS::Storage::DistributedFile;
}

void P2PLinkManager::Init(std::shared_ptr<FFRTTimer> timer,
    std::function<std::shared_ptr<ClipPlugin>()> pluginGetter)
{
    timer_ = std::move(timer);
    pluginGetter_ = std::move(pluginGetter);
}

std::string P2PLinkManager::IdleTimerName(const std::string &networkId)
{
    return "P2P_IDLE_" + networkId;
}

std::string P2PLinkManager::EstablishTimerName(const std::string &networkId)
{
    return "P2P_ESTABLISH_" + networkId;
}

std::shared_ptr<P2PLinkManager::LinkEntry> P2PLinkManager::GetOrCreateEntry(const std::string &networkId)
{
    std::shared_ptr<LinkEntry> entry;
    links_.Compute(networkId, [&entry](const auto &key, auto &value) {
        if (!value) {
            value = std::make_shared<LinkEntry>();
        }
        entry = value;
        return true;
    });
    return entry;
}

std::shared_ptr<BlockObject<int32_t>> P2PLinkManager::EnsureLink(
    const std::string &networkId, bool applyResourcesOnReuse)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    if (networkId.empty()) {
        return nullptr;
    }
    auto entry = GetOrCreateEntry(networkId);
    std::lock_guard<std::mutex> lock(entry->mutex);
    entry->refCount++;
    auto state = entry->state.load();
    if (state == LinkState::ESTABLISHED) {
        CancelIdleTimer(networkId);
        if (applyResourcesOnReuse) {
            ApplyResourcesIfNeeded(networkId, entry);
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "reuse established link, deviceId=%{public}.5s, ref=%{public}d",
            networkId.c_str(), entry->refCount);
        return nullptr;
    }
    if (state == LinkState::ESTABLISHING) {
        auto block = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
        entry->waiters.push_back(block);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "join in-flight establish, deviceId=%{public}.5s, ref=%{public}d",
            networkId.c_str(), entry->refCount);
        return block;
    }
    entry->state.store(LinkState::ESTABLISHING);
    if (!timer_) {
        entry->state.store(LinkState::IDLE);
        entry->refCount--;
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "timer is null, cannot establish, deviceId=%{public}.5s, ref=%{public}d",
            networkId.c_str(), entry->refCount);
        return nullptr;
    }
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
    auto establishBlock = entry->establishBlock;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start new establish, deviceId=%{public}.5s, ref=%{public}d",
        networkId.c_str(), entry->refCount);
    std::string taskName = EstablishTimerName(networkId);
    FFRTTask task = [this, networkId, entry] {
        std::thread thread([=]() { DoEstablish(networkId, entry, true); });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "DoEstablish");
        thread.detach();
    };
    timer_->SetTimer(taskName, task);
    return establishBlock;
#else
    return nullptr;
#endif
}

void P2PLinkManager::PreEstablish(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    if (networkId.empty()) {
        return;
    }
    auto entry = GetOrCreateEntry(networkId);
    std::lock_guard<std::mutex> lock(entry->mutex);
    auto state = entry->state.load();
    if (state == LinkState::ESTABLISHED) {
        StartIdleTimer(networkId);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "pre-establish: already established, refresh idle, deviceId=%{public}.5s",
            networkId.c_str());
        return;
    }
    if (state == LinkState::ESTABLISHING) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "pre-establish: already in progress, deviceId=%{public}.5s", networkId.c_str());
        return;
    }
    entry->state.store(LinkState::ESTABLISHING);
    entry->establishBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pre-establish: start new, deviceId=%{public}.5s",
        networkId.c_str());
    if (timer_) {
        std::string taskName = EstablishTimerName(networkId);
        FFRTTask task = [this, networkId, entry] {
            std::thread thread([=]() { DoEstablish(networkId, entry, false); });
            PasteBoardCommonUtils::SetThreadTaskName(thread, "PreDoEstablish");
            thread.detach();
        };
        timer_->SetTimer(taskName, task);
    }
#endif
}

void P2PLinkManager::DoEstablish(
    const std::string &networkId, std::shared_ptr<LinkEntry> entry, bool applyResources)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DoEstablish enter, deviceId=%{public}.5s, applyRes=%{public}d",
        networkId.c_str(), applyResources);
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist, ret=%{public}d", ret);
        NotifyWaiters(entry, SET_VALUE_FAILURE);
        CleanupEntryIfIdle(networkId, entry);
        return;
    }
    auto plugin = pluginGetter_ ? pluginGetter_() : nullptr;
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is not exist");
        NotifyWaiters(entry, SET_VALUE_FAILURE);
        CleanupEntryIfIdle(networkId, entry);
        return;
    }
    if (applyResources && !ApplyAdvancedResource(networkId, plugin, entry)) {
        return;
    }
    if (!ConnectDfsAndPublish(networkId, plugin, entry, applyResources)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(entry->mutex);
        entry->state.store(LinkState::ESTABLISHED);
        entry->resourcesApplied = applyResources;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "establish success, deviceId=%{public}.5s, ref=%{public}d",
            networkId.c_str(), entry->refCount);
    }
    NotifyWaiters(entry, SET_VALUE_SUCCESS);
    {
        std::lock_guard<std::mutex> lock(entry->mutex);
        if (entry->refCount <= 0) {
            StartIdleTimer(networkId);
        }
    }
#endif
}

bool P2PLinkManager::ApplyAdvancedResource(const std::string &networkId,
    std::shared_ptr<ClipPlugin> plugin, std::shared_ptr<LinkEntry> entry)
{
    auto status = plugin->ApplyAdvancedResource(networkId);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "apply resource failed, deviceId=%{public}.5s, status=%{public}d",
            networkId.c_str(), status);
        NotifyWaiters(entry, SET_VALUE_FAILURE);
        CleanupEntryIfIdle(networkId, entry);
        return false;
    }
    return true;
}

bool P2PLinkManager::ConnectDfsAndPublish(const std::string &networkId,
    std::shared_ptr<ClipPlugin> plugin, std::shared_ptr<LinkEntry> entry, bool applyResources)
{
    auto status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "publish CONNECT_SUCC failed, status=%{public}d", status);
    }
    status = DistributedFileDaemonManager::GetInstance().ConnectDfs(networkId);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status=%{public}d", status);
        plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
        NotifyWaiters(entry, SET_VALUE_FAILURE);
        CleanupEntryIfIdle(networkId, entry);
        return false;
    }
    return true;
}

void P2PLinkManager::ApplyResourcesIfNeeded(const std::string &networkId, std::shared_ptr<LinkEntry> entry)
{
    if (entry->resourcesApplied) {
        return;
    }
    auto plugin = pluginGetter_ ? pluginGetter_() : nullptr;
    if (plugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "plugin is not exist, cannot apply resources");
        return;
    }
    auto status = plugin->ApplyAdvancedResource(networkId);
    if (status != RESULT_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "apply resource on reuse failed, status=%{public}d", status);
        return;
    }
    entry->resourcesApplied = true;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "apply resources on reuse, deviceId=%{public}.5s", networkId.c_str());
}

void P2PLinkManager::NotifyWaiters(std::shared_ptr<LinkEntry> entry, int32_t value)
{
    std::vector<std::shared_ptr<BlockObject<int32_t>>> snapshot;
    std::shared_ptr<BlockObject<int32_t>> establishBlock = nullptr;
    {
        std::lock_guard<std::mutex> lock(entry->mutex);
        establishBlock = entry->establishBlock;
        entry->establishBlock = nullptr;
        snapshot = entry->waiters;
        entry->waiters.clear();
    }
    if (establishBlock) {
        establishBlock->SetValue(value);
    }
    for (const auto &block : snapshot) {
        if (block) {
            block->SetValue(value);
        }
    }
}

void P2PLinkManager::Release(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto [found, entry] = links_.Find(networkId);
    if (!found || !entry) {
        return;
    }
    std::lock_guard<std::mutex> lock(entry->mutex);
    entry->refCount--;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "release, deviceId=%{public}.5s, ref=%{public}d, state=%{public}d",
        networkId.c_str(), entry->refCount, static_cast<int32_t>(entry->state.load()));
    if (entry->refCount <= 0 && entry->state.load() == LinkState::ESTABLISHED) {
        StartIdleTimer(networkId);
    }
#endif
}

void P2PLinkManager::ForceClose(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto [found, entry] = links_.Find(networkId);
    if (!found || !entry) {
        return;
    }
    CancelIdleTimer(networkId);
    if (timer_) {
        timer_->CancelTimer(EstablishTimerName(networkId));
    }
    NotifyWaiters(entry, SET_VALUE_FAILURE);
    {
        std::lock_guard<std::mutex> lock(entry->mutex);
        entry->state.store(LinkState::IDLE);
        entry->refCount = 0;
        entry->resourcesApplied = false;
        entry->establishBlock = nullptr;
    }
    DoCloseLink(networkId);
    links_.Erase(networkId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "force close, deviceId=%{public}.5s", networkId.c_str());
#endif
}

void P2PLinkManager::CloseAll()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    links_.ForEach([this](const auto &networkId, auto &entry) {
        CancelIdleTimer(networkId);
        if (timer_) {
            timer_->CancelTimer(EstablishTimerName(networkId));
        }
        if (entry) {
            NotifyWaiters(entry, SET_VALUE_FAILURE);
            std::lock_guard<std::mutex> lock(entry->mutex);
            entry->state.store(LinkState::IDLE);
            entry->refCount = 0;
            entry->resourcesApplied = false;
            entry->establishBlock = nullptr;
        }
        DoCloseLink(networkId);
        return false;
    });
    links_.Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "close all links");
#endif
}

bool P2PLinkManager::IsEstablished(const std::string &networkId)
{
    auto [found, entry] = links_.Find(networkId);
    if (!found || !entry) {
        return false;
    }
    return entry->state.load() == LinkState::ESTABLISHED;
}

bool P2PLinkManager::HasLink(const std::string &networkId)
{
    return links_.Find(networkId).first;
}

void P2PLinkManager::RemoveLink(const std::string &networkId)
{
    ForceClose(networkId);
}

std::vector<std::string> P2PLinkManager::GetAllNetworkIds()
{
    std::vector<std::string> result;
    links_.ForEach([&result](const auto &key, auto &value) {
        result.push_back(key);
        return false;
    });
    return result;
}

void P2PLinkManager::DoCloseLink(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
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
    auto plugin = pluginGetter_ ? pluginGetter_() : nullptr;
    if (plugin) {
        auto status = plugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::IDLE);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state idle error, status:%{public}d", status);
        }
    }
#endif
}

void P2PLinkManager::StartIdleTimer(const std::string &networkId)
{
    if (!timer_) {
        return;
    }
    std::string taskName = IdleTimerName(networkId);
    timer_->CancelTimer(taskName);
    FFRTTask task = [this, networkId] {
        std::thread thread([=]() { OnIdleTimeout(networkId); });
        PasteBoardCommonUtils::SetThreadTaskName(thread, "P2pIdleTimeout");
        thread.detach();
    };
    timer_->SetTimer(taskName, task, IDLE_TIMEOUT);
}

void P2PLinkManager::CancelIdleTimer(const std::string &networkId)
{
    if (timer_) {
        timer_->CancelTimer(IdleTimerName(networkId));
    }
}

void P2PLinkManager::OnIdleTimeout(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto [found, entry] = links_.Find(networkId);
    if (!found || !entry) {
        return;
    }
    bool shouldClose = false;
    {
        std::lock_guard<std::mutex> lock(entry->mutex);
        if (entry->refCount <= 0 && entry->state.load() == LinkState::ESTABLISHED) {
            entry->state.store(LinkState::IDLE);
            entry->resourcesApplied = false;
            entry->establishBlock = nullptr;
            shouldClose = true;
        }
    }
    if (shouldClose) {
        links_.Erase(networkId);
        DoCloseLink(networkId);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "idle timeout, close link, deviceId=%{public}.5s",
            networkId.c_str());
    }
#endif
}

void P2PLinkManager::CleanupEntryIfIdle(const std::string &networkId, std::shared_ptr<LinkEntry> entry)
{
    std::lock_guard<std::mutex> lock(entry->mutex);
    entry->state.store(LinkState::IDLE);
    entry->resourcesApplied = false;
    entry->establishBlock = nullptr;
    if (entry->refCount <= 0) {
        links_.Erase(networkId);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "cleanup idle entry after establish failure, deviceId=%{public}.5s", networkId.c_str());
    } else {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "reset to idle after establish failure, ref pending, deviceId=%{public}.5s, ref=%{public}d",
            networkId.c_str(), entry->refCount);
    }
}

} // namespace MiscServices
} // namespace OHOS
