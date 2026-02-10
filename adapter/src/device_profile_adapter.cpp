/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "device_profile_adapter.h"

#include <thread>

#include "cJSON.h"
#include "device_profile_client.h"
#include "distributed_device_profile_errors.h"
#include "i_static_capability_collector.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "profile_change_listener_stub.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::DistributedDeviceProfile;

constexpr const int32_t ERR_OK = 0;

constexpr const char *STATUS_ENABLE = "1";
constexpr const char *STATUS_DISABLE = "0";
constexpr const char *SERVICE_ID = "pasteboardService";
constexpr const char *STATIC_CHARACTER_ID = "static_capability";
constexpr const char *VERSION_ID = "PasteboardVersionId";
constexpr const char *CHARACTERISTIC_VALUE = "characteristicValue";
constexpr const char *SWITCH_ID = "SwitchStatus_Key_Distributed_Pasteboard";
constexpr const char *CHARACTER_ID = "SwitchStatus";

static IDeviceProfileAdapter::OnProfileUpdateCallback g_onProfileUpdateCallback = nullptr;

static inline std::string Bool2Str(bool value)
{
    return value ? STATUS_ENABLE : STATUS_DISABLE;
}

static inline bool Str2Bool(const std::string &value)
{
    return value == STATUS_ENABLE;
}

class SubscribeDPChangeListener : public ProfileChangeListenerStub {
public:
    SubscribeDPChangeListener() = default;
    ~SubscribeDPChangeListener() = default;

    int32_t OnTrustDeviceProfileAdd(const TrustDeviceProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnTrustDeviceProfileDelete(const TrustDeviceProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnTrustDeviceProfileUpdate(const TrustDeviceProfile &oldProfile,
        const TrustDeviceProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return ERR_OK;
    }

    int32_t OnDeviceProfileAdd(const DeviceProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnDeviceProfileDelete(const DeviceProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnDeviceProfileUpdate(const DeviceProfile &oldProfile, const DeviceProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return ERR_OK;
    }

    int32_t OnServiceProfileAdd(const ServiceProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnServiceProfileDelete(const ServiceProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnServiceProfileUpdate(const ServiceProfile &oldProfile, const ServiceProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return ERR_OK;
    }

    int32_t OnCharacteristicProfileAdd(const CharacteristicProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnCharacteristicProfileDelete(const CharacteristicProfile &profile) override
    {
        (void)profile;
        return ERR_OK;
    }

    int32_t OnCharacteristicProfileUpdate(const CharacteristicProfile &oldProfile,
        const CharacteristicProfile &newProfile) override
    {
        std::string udid = newProfile.GetDeviceId();
        std::string status = newProfile.GetCharacteristicValue();
        if (g_onProfileUpdateCallback != nullptr) {
            std::thread thread(g_onProfileUpdateCallback, udid, status == STATUS_ENABLE);
            pthread_setname_np(thread.native_handle(), "OnProfileUpdate");
            thread.detach();
        }
        return ERR_OK;
    }
};

class DeviceProfileAdapter : public IDeviceProfileAdapter {
public:
    int32_t RegisterUpdateCallback(const OnProfileUpdateCallback callback) override;
    int32_t PutDeviceStatus(const std::string &udid, bool status) override;
    int32_t GetDeviceStatus(const std::string &udid, bool &status) override;
    bool GetDeviceVersion(const std::string &udid, uint32_t &versionId) override;
    int32_t SubscribeProfileEvent(const std::string &udid) override;
    int32_t UnSubscribeProfileEvent(const std::string &udid) override;
    void SendSubscribeInfos() override;
    void ClearDeviceProfileService() override;

private:
    std::mutex mutex_;
    std::unordered_map<std::string, SubscribeInfo> subscribeInfoCache_;
};

int32_t DeviceProfileAdapter::RegisterUpdateCallback(const OnProfileUpdateCallback callback)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(callback != nullptr,
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_COMMON, "callback is null");
    g_onProfileUpdateCallback = callback;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "success");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t DeviceProfileAdapter::PutDeviceStatus(const std::string &udid, bool status)
{
    std::string enabledStatus = Bool2Str(status);
    CharacteristicProfile profile;
    profile.SetDeviceId(udid);
    profile.SetServiceName(SWITCH_ID);
    profile.SetCharacteristicKey(CHARACTER_ID);
    profile.SetCharacteristicValue(enabledStatus);

    int32_t ret = DeviceProfileClient::GetInstance().PutCharacteristicProfile(profile);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == DistributedDeviceProfile::DP_SUCCESS ||
        ret == DistributedDeviceProfile::DP_CACHE_EXIST, ret, PASTEBOARD_MODULE_COMMON,
        "failed, udid=%{public}.5s, status=%{public}d, ret=%{public}d", udid.c_str(), status, ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "success, udid=%{public}.5s, status=%{public}d", udid.c_str(), status);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t DeviceProfileAdapter::GetDeviceStatus(const std::string &udid, bool &status)
{
    CharacteristicProfile profile;
    int32_t ret = DeviceProfileClient::GetInstance().GetCharacteristicProfile(udid,
        SWITCH_ID, CHARACTER_ID, profile);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == DistributedDeviceProfile::DP_SUCCESS, ret, PASTEBOARD_MODULE_COMMON,
        "failed, udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);

    std::string enabledStatus = profile.GetCharacteristicValue();
    status = Str2Bool(enabledStatus);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "success, udid=%{public}.5s, status=%{public}d", udid.c_str(), status);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

bool DeviceProfileAdapter::GetDeviceVersion(const std::string &udid, uint32_t &versionId)
{
    CharacteristicProfile profile;
    int32_t ret = DeviceProfileClient::GetInstance().GetCharacteristicProfile(udid,
        SERVICE_ID, STATIC_CHARACTER_ID, profile);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == DistributedDeviceProfile::DP_SUCCESS, false, PASTEBOARD_MODULE_COMMON,
        "get profile failed, udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);

    std::string jsonStr = profile.GetCharacteristicValue();
    cJSON *jsonObj = cJSON_Parse(jsonStr.c_str());
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(jsonObj != nullptr, false, PASTEBOARD_MODULE_COMMON,
        "parse profile failed, udid=%{public}.5s, profile=%{public}s", udid.c_str(), jsonStr.c_str());

    cJSON *version = cJSON_GetObjectItemCaseSensitive(jsonObj, VERSION_ID);
    if (version == nullptr || !cJSON_IsNumber(version) || (version->valuedouble < 0)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "version not found, udid=%{public}.5s, profile=%{public}s",
            udid.c_str(), jsonStr.c_str());
        cJSON_Delete(jsonObj);
        return false;
    }

    versionId = static_cast<uint32_t>(version->valuedouble);
    cJSON_Delete(jsonObj);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "udid=%{public}.5s, version=%{public}u", udid.c_str(), versionId);
    return true;
}

int32_t DeviceProfileAdapter::SubscribeProfileEvent(const std::string &udid)
{
    std::lock_guard lock(mutex_);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(subscribeInfoCache_.find(udid) == subscribeInfoCache_.end(),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_COMMON,
        "already exists, udid=%{public}.5s", udid.c_str());

    SubscribeInfo subscribeInfo;
    subscribeInfo.SetSaId(PASTEBOARD_SERVICE_ID);
    subscribeInfo.SetSubscribeKey(udid, SWITCH_ID, CHARACTER_ID, CHARACTERISTIC_VALUE);
    subscribeInfo.AddProfileChangeType(ProfileChangeType::CHAR_PROFILE_ADD);
    subscribeInfo.AddProfileChangeType(ProfileChangeType::CHAR_PROFILE_UPDATE);
    subscribeInfo.AddProfileChangeType(ProfileChangeType::CHAR_PROFILE_DELETE);
    sptr<IProfileChangeListener> subscribeDPChangeListener = new (std::nothrow) SubscribeDPChangeListener;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(subscribeDPChangeListener != nullptr,
        static_cast<int32_t>(PasteboardError::MALLOC_FAILED), PASTEBOARD_MODULE_COMMON,
        "malloc failed, udid=%{public}.5s", udid.c_str());

    subscribeInfo.SetListener(subscribeDPChangeListener);
    subscribeInfoCache_[udid] = subscribeInfo;

    int32_t ret = DeviceProfileClient::GetInstance().SubscribeDeviceProfile(subscribeInfo);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == DistributedDeviceProfile::DP_SUCCESS, ret, PASTEBOARD_MODULE_COMMON,
        "failed, udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "success, udid=%{public}.5s", udid.c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t DeviceProfileAdapter::UnSubscribeProfileEvent(const std::string &udid)
{
    std::lock_guard lock(mutex_);
    auto it = subscribeInfoCache_.find(udid);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(it != subscribeInfoCache_.end(),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_COMMON,
        "not find, udid=%{public}.5s", udid.c_str());

    int32_t ret = DeviceProfileClient::GetInstance().UnSubscribeDeviceProfile(it->second);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == DistributedDeviceProfile::DP_SUCCESS, ret, PASTEBOARD_MODULE_COMMON,
        "failed, udid=%{public}.5s, ret=%{public}d", udid.c_str(), ret);

    subscribeInfoCache_.erase(it);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "success, udid=%{public}.5s", udid.c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

void DeviceProfileAdapter::SendSubscribeInfos()
{
    std::lock_guard lock(mutex_);
    PASTEBOARD_CHECK_AND_RETURN_LOGI(!subscribeInfoCache_.empty(), PASTEBOARD_MODULE_COMMON,
        "no subscribe info");

    DeviceProfileClient::GetInstance().SendSubscribeInfos();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "success, size=%{public}zu", subscribeInfoCache_.size());
}

void DeviceProfileAdapter::ClearDeviceProfileService()
{
    DeviceProfileClient::GetInstance().ClearDeviceProfileService();
}

IDeviceProfileAdapter *GetDeviceProfileAdapter()
{
    static DeviceProfileAdapter instance;
    return &instance;
}

void DeinitDeviceProfileAdapter()
{
    DeviceProfileClient::GetInstance().ClearDeviceProfileService();
    g_onProfileUpdateCallback = nullptr;
}

class PasteboardStaticCapability : public IStaticCapabilityCollector {
public:
    static PasteboardStaticCapability &GetInstance()
    {
        static PasteboardStaticCapability instance;
        return instance;
    }

    bool IsSupportCapability() override
    {
        return true;
    }
};

extern "C" {
API_EXPORT IStaticCapabilityCollector *GetStaticCapabilityCollector()
{
    return &PasteboardStaticCapability::GetInstance();
}
} // extern "C"

} // namespace MiscServices
} // namespace OHOS
