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

#include "device/dev_profile.h"
#include <pthread.h>
#include <thread>

#include "device/device_profile_proxy.h"
#include "device/dm_adapter.h"
#include "ffrt/ffrt_utils.h"
#include "pasteboard_error.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_hilog.h"
namespace OHOS {
namespace MiscServices {
constexpr const char *UE_SWITCH_OPERATION = "PASTEBOARD_SWITCH_OPERATION";

DevProfile &DevProfile::GetInstance()
{
    static DevProfile instance;
    return instance;
}

void DevProfile::OnProfileUpdate(const std::string &udid, bool status)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "udid=%{public}.5s, status=%{public}d", udid.c_str(), status);
    DevProfile::GetInstance().UpdateEnabledStatus(udid, status);
    DevProfile::GetInstance().Notify(status);
}

void DevProfile::PostDelayReleaseProxy()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "post delay task start");
    constexpr uint32_t DELAY_TIME = 60 * 1000; // 60s
    static FFRTTimer ffrtTimer("release_dp_proxy");

    FFRTTask task = [this]() {
        std::thread thread([=]() {
            std::lock_guard lock(proxyMutex_);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "execute delay task");
            if (proxy_ == nullptr) {
                return;
            }

            if (subscribeUdidList_.empty()) {
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "deinit dp proxy");
                proxy_ = nullptr;
            }
        });
        pthread_setname_np(thread.native_handle(), "PostDelayReleas");
        thread.detach();
    };

    ffrtTimer.SetTimer("release_dp_proxy", task, DELAY_TIME);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "post delay task end");
}

void DevProfile::PutDeviceStatus(bool status)
{
    std::string networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(!udid.empty(), PASTEBOARD_MODULE_SERVICE,
        "get udid failed, netId=%{public}.5s", networkId.c_str());

    UpdateEnabledStatus(udid, status);
    UE_SWITCH(UE_SWITCH_OPERATION, UeReporter::UE_OPERATION_TYPE, status ?
        UeReporter::SwitchStatus::SWITCH_OPEN : UeReporter::SwitchStatus::SWITCH_CLOSE);

    std::lock_guard lock(proxyMutex_);
    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(adapter != nullptr, PASTEBOARD_MODULE_SERVICE, "adapter is null");
    int32_t ret = adapter->PutDeviceStatus(udid, status);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), PASTEBOARD_MODULE_SERVICE,
        "put dp status failed, ret=%{public}d", ret);

    Notify(status);
}

int32_t DevProfile::GetDeviceStatus(const std::string &networkId, bool &status)
{
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!udid.empty(),
        static_cast<int32_t>(PasteboardError::GET_LOCAL_DEVICE_ID_ERROR), PASTEBOARD_MODULE_SERVICE,
        "get udid failed, netId=%{public}.5s", networkId.c_str());

    bool cachedStatus = enabledStatusCache_.ComputeIfPresent(udid, [&status](const auto &key, auto &value) {
        status = value;
        return true;
    });
    if (cachedStatus) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    std::lock_guard lock(proxyMutex_);
    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(adapter != nullptr, static_cast<int32_t>(PasteboardError::DLOPEN_FAILED),
        PASTEBOARD_MODULE_SERVICE, "adapter is null");
    int32_t ret = adapter->GetDeviceStatus(udid, status);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get dp status failed, udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);

    UpdateEnabledStatus(udid, status);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

bool DevProfile::GetDeviceVersion(const std::string &networkId, uint32_t &versionId)
{
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!udid.empty(), false,
        PASTEBOARD_MODULE_SERVICE, "get udid failed, netId=%{public}.5s", networkId.c_str());

    std::lock_guard lock(proxyMutex_);
    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(adapter != nullptr, false, PASTEBOARD_MODULE_SERVICE, "adapter is null");
    bool ret = adapter->GetDeviceVersion(udid, versionId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, false,
        PASTEBOARD_MODULE_SERVICE, "get dp version failed, udid=%{public}.5s", udid.c_str());

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "netid=%{public}.5s, udid=%{public}.5s, version=%{public}u",
        networkId.c_str(), udid.c_str(), versionId);
    return true;
}

void DevProfile::SubscribeProfileEvent(const std::string &networkId)
{
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(!udid.empty(), PASTEBOARD_MODULE_SERVICE,
        "get udid failed, netId=%{public}.5s", networkId.c_str());

    std::lock_guard lock(proxyMutex_);
    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(adapter != nullptr, PASTEBOARD_MODULE_SERVICE, "adapter is null");

    int32_t ret = adapter->RegisterUpdateCallback(&DevProfile::OnProfileUpdate);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), PASTEBOARD_MODULE_SERVICE,
        "register dp update callback failed, ret=%{public}d", ret);

    ret = adapter->SubscribeProfileEvent(udid);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);

    subscribeUdidList_.emplace(udid);
}

void DevProfile::UnSubscribeProfileEvent(const std::string &networkId)
{
    std::string udid = DMAdapter::GetInstance().GetUdidByNetworkId(networkId);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(!udid.empty(), PASTEBOARD_MODULE_SERVICE,
        "get udid failed, netId=%{public}.5s", networkId.c_str());

    std::lock_guard lock(proxyMutex_);
    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(adapter != nullptr, PASTEBOARD_MODULE_SERVICE, "adapter is null");

    int32_t ret = adapter->UnSubscribeProfileEvent(udid);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);

    subscribeUdidList_.erase(udid);
}

void DevProfile::UnSubscribeAllProfileEvents()
{
    std::lock_guard lock(proxyMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(!subscribeUdidList_.empty(), PASTEBOARD_MODULE_SERVICE, "udid list empty");

    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(adapter != nullptr, PASTEBOARD_MODULE_SERVICE, "adapter is null");

    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    for (const std::string &udid : subscribeUdidList_) {
        ret = adapter->UnSubscribeProfileEvent(udid);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);
    }

    subscribeUdidList_.clear();
}

void DevProfile::SendSubscribeInfos()
{
    std::lock_guard lock(proxyMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(!subscribeUdidList_.empty(), PASTEBOARD_MODULE_SERVICE, "udid list empty");

    PostDelayReleaseProxy();
    if (proxy_ == nullptr) {
        proxy_ = std::make_shared<DeviceProfileProxy>();
    }
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(adapter != nullptr, PASTEBOARD_MODULE_SERVICE, "adapter is null");

    adapter->SendSubscribeInfos();
}

void DevProfile::ClearDeviceProfileService()
{
    std::lock_guard lock(proxyMutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(proxy_ != nullptr, PASTEBOARD_MODULE_SERVICE, "not need clear");

    enabledStatusCache_.Clear();
    PostDelayReleaseProxy();
    auto adapter = proxy_->GetAdapter();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(adapter != nullptr, PASTEBOARD_MODULE_SERVICE, "adapter is null");

    adapter->ClearDeviceProfileService();
}

void DevProfile::Watch(Observer observer)
{
    observer_ = std::move(observer);
}

void DevProfile::Notify(bool isEnable)
{
    if (observer_ != nullptr) {
        observer_(isEnable);
    }
}

void DevProfile::UpdateEnabledStatus(const std::string &udid, bool status)
{
    enabledStatusCache_.Compute(udid, [&status](const auto &key, auto &value) {
        value = status;
        return true;
    });
}

void DevProfile::EraseEnabledStatus(const std::string &udid)
{
    enabledStatusCache_.Erase(udid);
}
} // namespace MiscServices
} // namespace OHOS
