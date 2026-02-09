/*
 * Copyright (C) 2022-2025 Huawei Device Co., Ltd.
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
#include "device/distributed_module_config.h"

#include <thread>
#include "device/dev_profile.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
constexpr uint32_t RETRY_TIMES = 30;
constexpr uint32_t RETRY_INTERVAL = 1000; // milliseconds
constexpr uint32_t RANDOM_MAX = 500;
constexpr uint32_t RANDOM_MIN = 5;
bool DistributedModuleConfig::IsOn()
{
    if (GetDeviceNum() != 0) {
        Notify();
    }
    return status_.load();
}

void DistributedModuleConfig::Watch(const Observer &observer)
{
    observer_ = std::move(observer);
}

void DistributedModuleConfig::Notify()
{
    auto status = GetEnabledStatus();
    if (status == static_cast<int32_t>(PasteboardError::DP_LOAD_SERVICE_ERROR)) {
        if (!retrying_.exchange(true)) {
            GetRetryTask();
        }
        return;
    }
    bool newStatus = (status == static_cast<int32_t>(PasteboardError::E_OK));
    if (newStatus != status_.load()) {
        status_.store(newStatus);
        if (observer_ != nullptr) {
            observer_(newStatus);
        }
    }
}

void DistributedModuleConfig::GetRetryTask()
{
    std::thread remover([this]() {
        retrying_.store(true);
        uint32_t retry = 0;
        auto status = static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR);
        while (retry < RETRY_TIMES) {
            ++retry;
            status = GetEnabledStatus();
            if (status == static_cast<int32_t>(PasteboardError::DP_LOAD_SERVICE_ERROR)) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                    "dp load err, retry:%{public}d, status_:%{public}d"
                    "newStatus:%{public}d",
                    retry, status_.load(), status);
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_INTERVAL));
                continue;
            }
            break;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "Retry end. count:%{public}d, status_:%{public}d"
            "newStatus:%{public}d",
            retry, status_.load(), status);
        bool newStatus = (status == static_cast<int32_t>(PasteboardError::E_OK));
        if (newStatus != status_.load()) {
            status_.store(newStatus);
            if (observer_ != nullptr) {
                observer_(newStatus);
            }
        }
        retrying_.store(false);
    });
    remover.detach();
}

size_t DistributedModuleConfig::GetDeviceNum()
{
    auto networkIds = DMAdapter::GetInstance().GetNetworkIds();
    return networkIds.size();
}

int32_t DistributedModuleConfig::GetEnabledStatus()
{
    auto localNetworkId = DMAdapter::GetInstance().GetLocalNetworkId();
    bool localEnable = false;
    auto status = DevProfile::GetInstance().GetDeviceStatus(localNetworkId, localEnable);
    if (status != static_cast<int32_t>(PasteboardError::E_OK) || !localEnable) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "GetLocalEnable false, ret:%{public}d, switch:%{public}d",
            status, localEnable);
        return static_cast<int32_t>(PasteboardError::LOCAL_SWITCH_NOT_TURNED_ON);
    }
    auto networkIds = DMAdapter::GetInstance().GetNetworkIds();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "device online nums: %{public}zu", networkIds.size());
    for (auto &id : networkIds) {
        bool remoteEnable = false;
        auto res = DevProfile::GetInstance().GetDeviceStatus(id, remoteEnable);
        if (res == static_cast<int32_t>(PasteboardError::E_OK) && remoteEnable) {
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "remoteEnabledStatus is false.");
    return static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR);
}

uint32_t DistributedModuleConfig::GetRemoteDeviceMinVersion()
{
    auto [minVersion, maxVersion] = GetRemoteDeviceVersion();
    return minVersion;
}

uint32_t DistributedModuleConfig::GetRemoteDeviceMaxVersion()
{
    auto [minVersion, maxVersion] = GetRemoteDeviceVersion();
    return maxVersion;
}

std::pair<uint32_t, uint32_t> DistributedModuleConfig::GetRemoteDeviceVersion()
{
    uint32_t minVersion = UINT_MAX;
    uint32_t maxVersion = 0;

    const auto &networkIds = DMAdapter::GetInstance().GetNetworkIds();
    for (const auto &networkId : networkIds) {
        bool remoteEnable = false;
        auto res = DevProfile::GetInstance().GetDeviceStatus(networkId, remoteEnable);
        if (res != static_cast<int32_t>(PasteboardError::E_OK) || !remoteEnable) {
            continue;
        }

        uint32_t deviceVersion = 0;
        if (!DevProfile::GetInstance().GetDeviceVersion(networkId, deviceVersion)) {
            continue;
        }

        minVersion = minVersion < deviceVersion ? minVersion : deviceVersion;
        maxVersion = maxVersion > deviceVersion ? maxVersion : deviceVersion;
    }
    return std::make_pair(minVersion, maxVersion);
}

void DistributedModuleConfig::Online(const std::string &device)
{
    srand(time(nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds((int32_t(rand() % (RANDOM_MAX - RANDOM_MIN)))));
    DevProfile::GetInstance().SubscribeProfileEvent(device);
    bool remoteEnable = false;
    DevProfile::GetInstance().GetDeviceStatus(device, remoteEnable);
    Notify();
}

void DistributedModuleConfig::Offline(const std::string &device)
{
    srand(time(nullptr));
    std::this_thread::sleep_for(std::chrono::milliseconds((int32_t(rand() % (RANDOM_MAX - RANDOM_MIN)))));
    DevProfile::GetInstance().UnSubscribeProfileEvent(device);
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(device);
    DevProfile::GetInstance().EraseEnabledStatus(udid);
    Notify();
}

void DistributedModuleConfig::OnReady(const std::string &device)
{
    (void)device;
}

void DistributedModuleConfig::Init()
{
    DMAdapter::GetInstance().Register(this);
    DevProfile::GetInstance().Watch([this](bool isEnable) -> void {
        Notify();
    });
}

void DistributedModuleConfig::DeInit()
{
    DMAdapter::GetInstance().Unregister(this);
}

} // namespace MiscServices
} // namespace OHOS