/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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
#include "dev_profile.h"

#include <cstring>
#include <thread>

#include "cJSON.h"
#include "dev_manager.h"
#include "device_manager.h"
#include "distributed_device_profile_client.h"
#include "distributed_module_config.h"
#include "para_handle.h"
#include "pasteboard_hilog.h"
#include "service_characteristic_profile.h"
#include "subscribe_info.h"
namespace OHOS {
namespace MiscServices {
using namespace OHOS::DeviceProfile;
using namespace OHOS::DistributedHardware;
constexpr const int32_t HANDLE_OK = 0;
constexpr const uint32_t NOT_SUPPORT = 0;
constexpr const uint32_t SUPPORT = 1;
constexpr const uint32_t FIRST_VERSION = 4;
constexpr const char *SERVICE_ID = "pasteboardService";
constexpr const char *CHARACTER_ID = "supportDistributedPasteboard";
constexpr const char *VERSION_ID = "PasteboardVersionId";

void DevProfile::PasteboardProfileEventCallback::OnSyncCompleted(const SyncResult &syncResults)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "OnSyncCompleted.");
}

void DevProfile::PasteboardProfileEventCallback::OnProfileChanged(const ProfileChangeNotification &changeNotification)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "OnProfileChanged start.");
    DistributedModuleConfig::Notify();
}

DevProfile::DevProfile()
{
}

DevProfile &DevProfile::GetInstance()
{
    static DevProfile instance;
    return instance;
}

void DevProfile::Init()
{
    ParaHandle::GetInstance().WatchEnabledStatus(ParameterChange);
}

void DevProfile::OnReady()
{
    auto status = ParaHandle::GetInstance().GetEnabledStatus();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "status = %{public}s.", status.c_str());
    PutEnabledStatus(status);
}

void DevProfile::ParameterChange(const char *key, const char *value, void *context)
{
    auto enabledKey = ParaHandle::DISTRIBUTED_PASTEBOARD_ENABLED_KEY;
    if (strncmp(key, enabledKey, strlen(enabledKey)) != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "key is error.");
        return;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "ParameterChange, key = %{public}s, value = %{public}s.", key, value);
    DevProfile::GetInstance().PutEnabledStatus(value);
    DistributedModuleConfig::Notify();
}

void DevProfile::PutEnabledStatus(const std::string &enabledStatus)
{
    ServiceCharacteristicProfile profile;
    profile.SetServiceId(SERVICE_ID);
    profile.SetServiceType(SERVICE_ID);
    cJSON *jsonObject = cJSON_CreateObject();
    cJSON_AddNumberToObject(jsonObject, CHARACTER_ID, NOT_SUPPORT);
    localEnable_ = false;
    if (enabledStatus == "true") {
        cJSON_ReplaceItemInObject(jsonObject, CHARACTER_ID, cJSON_CreateNumber(SUPPORT));
        localEnable_ = true;
    }
    cJSON_AddNumberToObject(jsonObject, VERSION_ID, FIRST_VERSION);
    profile.SetCharacteristicProfileJson(cJSON_PrintUnformatted(jsonObject));
    int32_t errNo = DistributedDeviceProfileClient::GetInstance().PutDeviceProfile(profile);
    if (errNo != HANDLE_OK) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "PutDeviceProfile failed, %{public}d", errNo);
        return;
    }
    SyncEnabledStatus();
}

void DevProfile::GetEnabledStatus(const std::string &deviceId, std::string &enabledStatus)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetEnabledStatus start.");
    ServiceCharacteristicProfile profile;
    int32_t ret = DistributedDeviceProfileClient::GetInstance().GetDeviceProfile(deviceId, SERVICE_ID, profile);
    if (ret != HANDLE_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "GetDeviceProfile failed, %{public}.5s.", deviceId.c_str());
        return;
    }
    const auto &jsonData = profile.GetCharacteristicProfileJson();
    cJSON *jsonObject = cJSON_Parse(jsonData.c_str());
    if (jsonObject == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "json parse failed.");
        return;
    }

    enabledStatus = "false";
    if (cJSON_GetNumberValue(cJSON_GetObjectItem(jsonObject, CHARACTER_ID)) == SUPPORT) {
        enabledStatus = "true";
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetEnabledStatus success %{public}s.", enabledStatus.c_str());
}

void DevProfile::GetRemoteDeviceVersion(const std::string &deviceId, uint32_t &versionId)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetRemoteDeviceVersion start.");
    ServiceCharacteristicProfile profile;
    int32_t ret = DistributedDeviceProfileClient::GetInstance().GetDeviceProfile(deviceId, SERVICE_ID, profile);
    if (ret != HANDLE_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "GetDeviceProfile failed, %{public}.5s.", deviceId.c_str());
        return;
    }
    const auto &jsonData = profile.GetCharacteristicProfileJson();
    cJSON *jsonObject = cJSON_Parse(jsonData.c_str());
    if (jsonObject == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "json parse failed.");
        return;
    }
    if (cJSON_GetNumberValue(cJSON_GetObjectItem(jsonObject, VERSION_ID)) == FIRST_VERSION) {
        versionId = FIRST_VERSION;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetRemoteDeviceVersion success, versionId = %{public}d.", versionId);
}

void DevProfile::SubscribeProfileEvent(const std::string &deviceId)
{
    if (deviceId.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "deviceId is empty.");
        return;
    }
    std::lock_guard<std::mutex> mutexLock(callbackMutex_);
    if (callback_.find(deviceId) != callback_.end()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "deviceId = %{public}.5s already exists.", deviceId.c_str());
        return;
    }
    auto profileCallback = std::make_shared<PasteboardProfileEventCallback>();
    callback_[deviceId] = profileCallback;
    std::list<std::string> serviceIds = { SERVICE_ID };
    ExtraInfo extraInfo;
    extraInfo["deviceId"] = deviceId;
    extraInfo["serviceIds"] = serviceIds;

    SubscribeInfo subscribeInfo;
    subscribeInfo.profileEvent = ProfileEvent::EVENT_PROFILE_CHANGED;
    subscribeInfo.extraInfo = std::move(extraInfo);
    int32_t errCode =
        DistributedDeviceProfileClient::GetInstance().SubscribeProfileEvent(subscribeInfo, profileCallback);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SubscribeProfileEvent result, errCode = %{public}d.", errCode);
}

void DevProfile::UnSubscribeProfileEvent(const std::string &deviceId)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "start, deviceId = %{public}.5s", deviceId.c_str());
    ProfileEvent profileEvent = EVENT_PROFILE_CHANGED;
    std::lock_guard<std::mutex> mutexLock(callbackMutex_);
    auto it = callback_.find(deviceId);
    if (it == callback_.end()) {
        return;
    }
    int32_t errCode = DistributedDeviceProfileClient::GetInstance().UnsubscribeProfileEvent(profileEvent, it->second);
    callback_.erase(it);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "UnsubscribeProfileEvent result, errCode = %{public}d.", errCode);
}

void DevProfile::SyncEnabledStatus()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SyncEnabledStatus start.");
    SyncOptions syncOptions;
    auto deviceIds = DevManager::GetInstance().GetNetworkIds();
    if (deviceIds.empty()) {
        return;
    }
    for (auto &id : deviceIds) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "id = %{public}.5s.", id.c_str());
        syncOptions.AddDevice(id);
    }
    syncOptions.SetSyncMode(SyncMode::PUSH_PULL);
    auto syncCallback = std::make_shared<PasteboardProfileEventCallback>();
    int32_t errCode = DistributedDeviceProfileClient::GetInstance().SyncDeviceProfile(syncOptions, syncCallback);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SyncEnabledStatus, ret = %{public}d.", errCode);
}

void DevProfile::UnsubscribeAllProfileEvents()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "UnsubscribeAllProfileEvents start.");
    std::lock_guard<std::mutex> mutexLock(callbackMutex_);
    for (auto it = callback_.begin(); it != callback_.end(); ++it) {
        ProfileEvent profileEvent = EVENT_PROFILE_CHANGED;
        int32_t errCode =
            DistributedDeviceProfileClient::GetInstance().UnsubscribeProfileEvent(profileEvent, it->second);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "errCode = %{public}d.", errCode);
        it = callback_.erase(it);
    }
}

bool DevProfile::GetLocalEnable()
{
    return localEnable_;
}

} // namespace MiscServices
} // namespace OHOS
