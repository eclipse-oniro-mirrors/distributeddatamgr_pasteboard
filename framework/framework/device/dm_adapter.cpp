/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include <pthread.h>
#include <thread>

#include "device/dm_adapter.h"

#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
constexpr size_t DMAdapter::MAX_ID_LEN;
constexpr const char *PKG_NAME = "pasteboard_service";

#ifdef PB_DEVICE_MANAGER_ENABLE
DmStateObserver::DmStateObserver(const std::function<void(const DmDeviceInfo &)> online,
    const std::function<void(const DmDeviceInfo &)> onReady, const std::function<void(const DmDeviceInfo &)> offline)
    : online_(std::move(online)), onReady_(std::move(onReady)), offline_(std::move(offline))
{
}

void DmStateObserver::OnDeviceOnline(const DmDeviceInfo &deviceInfo)
{
    std::thread thread([=] {
        if (online_ == nullptr || deviceInfo.authForm != IDENTICAL_ACCOUNT) {
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "device on:%{public}.6s", deviceInfo.networkId);
        online_(deviceInfo);
    });
    pthread_setname_np(thread.native_handle(), "OnDeviceOnline");
    thread.detach();
}

void DmStateObserver::OnDeviceOffline(const DmDeviceInfo &deviceInfo)
{
    std::thread thread([=] {
        if (offline_ == nullptr) {
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "device off:%{public}.6s", deviceInfo.networkId);
        offline_(deviceInfo);
    });
    pthread_setname_np(thread.native_handle(), "OnDeviceOffline");
    thread.detach();
}

void DmStateObserver::OnDeviceChanged(const DmDeviceInfo &deviceInfo)
{
    if (DeviceManager::GetInstance().IsSameAccount(deviceInfo.networkId)) {
        std::thread thread([=] {
            // authForm not valid use networkId
            PASTEBOARD_CHECK_AND_RETURN_LOGE(online_ != nullptr, PASTEBOARD_MODULE_SERVICE, "online_ is null");
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "device config changed:%{public}.6s", deviceInfo.networkId);
            online_(deviceInfo);
        });
        pthread_setname_np(thread.native_handle(), "OnDeviceChanged");
        thread.detach();
    }
}

void DmStateObserver::OnDeviceReady(const DmDeviceInfo &deviceInfo)
{
    std::thread thread([=] {
        if (onReady_ == nullptr || deviceInfo.authForm != IDENTICAL_ACCOUNT) {
            return;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "device onReady:%{public}.6s", deviceInfo.networkId);
        onReady_(deviceInfo);
    });
    pthread_setname_np(thread.native_handle(), "OnDeviceReady");
    thread.detach();
}

void DmDeath::OnRemoteDied()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "device manager died");
    DMAdapter::GetInstance().Initialize();
}
#endif

DMAdapter::DMAdapter() {}

DMAdapter::~DMAdapter()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    devices_.clear();
#endif
}

DMAdapter &DMAdapter::GetInstance()
{
    static DMAdapter instance;
    return instance;
}

bool DMAdapter::Initialize()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto observer = GetDmStateObserver();
    if (dmDeathObserver_ == nullptr) {
        dmDeathObserver_ = std::make_shared<DmDeath>();
    }
    DeviceManager::GetInstance().InitDeviceManager(PKG_NAME, dmDeathObserver_);
    DeviceManager::GetInstance().RegisterDevStateCallback(PKG_NAME, "", observer);
    SetDevices();
#endif
    return false;
}

#ifdef PB_DEVICE_MANAGER_ENABLE
std::shared_ptr<DmStateObserver> DMAdapter::GetDmStateObserver()
{
    std::lock_guard lock(observerMutex_);
    if (dmStateObserver_ != nullptr) {
        return dmStateObserver_;
    }
    dmStateObserver_ = std::make_shared<DmStateObserver>(
        [this](const DmDeviceInfo &deviceInfo) {
            observers_.ForEachCopies([&deviceInfo](auto &key, auto &value) {
                DMAdapter::GetInstance().SetDevices();
                value->Online(deviceInfo.networkId);
                return false;
            });
        },
        [this](const DmDeviceInfo &deviceInfo) {
            observers_.ForEachCopies([&deviceInfo](auto &key, auto &value) {
                value->OnReady(deviceInfo.networkId);
                return false;
            });
        },
        [this](const DmDeviceInfo &deviceInfo) {
            observers_.ForEachCopies([&deviceInfo](auto &key, auto &value) {
                DMAdapter::GetInstance().SetDevices();
                value->Offline(deviceInfo.networkId);
                return false;
            });
        });
    return dmStateObserver_;
}
#endif

void DMAdapter::DeInitialize()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DeviceManager::GetInstance().UnRegisterDevStateCallback(PKG_NAME);
    DeviceManager::GetInstance().UnInitDeviceManager(PKG_NAME);
#endif
}

const std::string &DMAdapter::GetLocalDeviceUdid()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!localDeviceUdid_.empty()) {
        return localDeviceUdid_;
    }

    DmDeviceInfo info;
    int32_t ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, info);
    if (ret != 0) {
        return invalidDeviceUdid_;
    }
    DeviceManager::GetInstance().GetUdidByNetworkId(PKG_NAME, info.networkId, localDeviceUdid_);
    if (!localDeviceUdid_.empty()) {
        return localDeviceUdid_;
    }
#endif
    return invalidDeviceUdid_;
}

std::string DMAdapter::GetDeviceName(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto devices = GetDevices();
    for (auto &device : devices) {
        if (device.networkId == networkId) {
            return device.deviceName;
        }
    }
#endif
    return DEVICE_INVALID_NAME;
}

const std::string DMAdapter::GetLocalNetworkId()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo info;
    int32_t ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, info);
    auto networkId = std::string(info.networkId);
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ret: %{public}d, networkId:%{public}.5s", ret, networkId.c_str());
    if (ret == 0 && !networkId.empty()) {
        return networkId;
    }
#endif
    return invalidNetworkId_;
}

#ifdef PB_DEVICE_MANAGER_ENABLE
int32_t DMAdapter::GetRemoteDeviceInfo(const std::string &networkId, DmDeviceInfo &remoteDevice)
{
    auto devices = GetDevices();
    for (auto &device : devices) {
        if (device.networkId == networkId) {
            remoteDevice = device;
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
    }
    return static_cast<int32_t>(PasteboardError::NO_TRUST_DEVICE_ERROR);
}
#endif

std::string DMAdapter::GetUdidByNetworkId(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    std::string udid;
    int32_t ret = DeviceManager::GetInstance().GetUdidByNetworkId(PKG_NAME, networkId, udid);
    if (ret == 0 && !udid.empty()) {
        return udid;
    }
#endif
    return invalidUdid_;
}

void DMAdapter::Register(DMObserver *observer)
{
    observers_.Insert(observer, observer);
}

void DMAdapter::Unregister(DMObserver *observer)
{
    observers_.Erase(observer);
}

std::vector<std::string> DMAdapter::GetNetworkIds()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto devices = GetDevices();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "devicesNums = %{public}zu.", devices.size());
    if (devices.empty()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "no device online!");
        return {};
    }
    std::vector<std::string> networkIds;
    for (auto &item : devices) {
        networkIds.emplace_back(item.networkId);
    }
    return networkIds;
#else
    return {};
#endif
}

int32_t DMAdapter::GetLocalDeviceType()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    if (deviceType_.load() != DmDeviceType::DEVICE_TYPE_UNKNOWN) {
        return deviceType_.load();
    }
    int32_t deviceType = DmDeviceType::DEVICE_TYPE_UNKNOWN;
    int32_t ret = DeviceManager::GetInstance().GetLocalDeviceType(PKG_NAME, deviceType);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get type failed, ret is %{public}d!", ret);
    } else {
        deviceType_.store(deviceType);
    }
    return deviceType;
#else
    return static_cast<int32_t>(PasteboardError::NO_DATA_ERROR);
#endif
}

bool DMAdapter::IsSameAccount(const std::string &networkId)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    auto devices = GetDevices();
    for (auto &device : devices) {
        if (device.networkId == networkId) {
            return device.authForm == IDENTICAL_ACCOUNT;
        }
    }
#endif
    return false;
}

void DMAdapter::SetDevices()
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    std::vector<DmDeviceInfo> devices;
    int32_t ret = DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", devices);
    if (ret != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Get device list failed, errCode: %{public}d", ret);
        return;
    }
    std::vector<DmDeviceInfo> networkIds;
    for (auto &item : devices) {
        if (DeviceManager::GetInstance().IsSameAccount(item.networkId)) {
            networkIds.emplace_back(item);
        }
    }
    std::unique_lock<std::shared_mutex> lock(dmMutex_);
    devices_ = std::move(networkIds);
#endif
}

#ifdef PB_DEVICE_MANAGER_ENABLE
std::vector<DmDeviceInfo> DMAdapter::GetDevices()
{
    std::shared_lock<std::shared_mutex> lock(dmMutex_);
    return devices_;
}
#endif
} // namespace OHOS::MiscServices