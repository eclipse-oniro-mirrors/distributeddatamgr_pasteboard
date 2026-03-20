/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_DP_I_DISTRIBUTED_DEVICE_PROFILE_H
#define OHOS_DP_I_DISTRIBUTED_DEVICE_PROFILE_H

#include <cstdint>
#include <map>

#include "iremote_broker.h"

namespace OHOS {
namespace DistributedDeviceProfile {
class CharacteristicProfile {
public:
    CharacteristicProfile() = default;

    CharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicKey, const std::string &characteristicValue)
        : deviceId_(deviceId), serviceName_(serviceName), characteristicKey_(characteristicKey),
        characteristicValue_(characteristicValue)
    {
    }

    void SetDeviceId(const std::string &deviceId)
    {
        deviceId_ = deviceId;
    }

    void SetServiceName(const std::string &serviceName)
    {
        serviceName_ = serviceName;
    }

    void SetCharacteristicKey(const std::string &characteristicKey)
    {
        characteristicKey_ = characteristicKey;
    }

    void SetCharacteristicValue(const std::string &characteristicValue)
    {
        characteristicValue_ = characteristicValue;
    }

    std::string GetDeviceId() const
    {
        return deviceId_;
    }

    std::string GetServiceName() const
    {
        return serviceName_;
    }

    std::string GetCharacteristicKey() const
    {
        return characteristicKey_;
    }

    std::string GetCharacteristicValue() const
    {
        return characteristicValue_;
    }

private:
    std::string deviceId_;
    std::string serviceName_;
    std::string characteristicKey_;
    std::string characteristicValue_;
};

enum class ProfileChangeType {
    TRUST_PROFILE_ADD = 0,
    TRUST_PROFILE_CHANGE,
    TRUST_PROFILE_DELETE,
    SERVICE_PROFILE_ADD,
    SERVICE_PROFILE_CHANGE,
    SERVICE_PROFILE_DELETE,
    CHAR_PROFILE_ADD,
    CHAR_PROFILE_CHANGE,
    CHAR_PROFILE_DELETE,
    DEVICE_PROFILE_ADD,
    DEVICE_PROFILE_CHANGE,
    DEVICE_PROFILE_DELETE,
};

class SubscribeInfo {
public:
    SubscribeInfo() = default;

    SubscribeInfo(int32_t saId, const std::string &subscribeKey) : saId_(saId), subscribeKey_(subscribeKey)
    {
    }

    int32_t GetSaId() const
    {
        return saId_;
    }

    std::string GetSubscribeKey() const
    {
        return subscribeKey_;
    }

    void SetSaId(int32_t saId)
    {
        saId_ = saId;
    }

    void SetSubscribeKeyKey(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, const std::string &characteristicValue)
    {
        subscribeKey_ = deviceId + SEPARATOR + serviceName + SEPARATOR + characteristicId + SEPARATOR + characteristicValue;
    }

    void AddProfileChangeType(ProfileChangeType type)
    {
        profileChangeTypes_.push_back(type);
    }

    void SetListener(const sptr<IRemoteObject> &listener)
    {
        listener_ = listener;
    }

    sptr<IRemoteObject> GetListener() const
    {
        return listener_;
    }

private:
    int32_t saId_ = -1;
    std::string subscribeKey_;
    std::vector<ProfileChangeType> profileChangeTypes_;
    sptr<IRemoteObject> listener_;
};

class IDistributedDeviceProfile : public IRemoteBroker {
public:
    IDistributedDeviceProfile() = default;
    virtual ~IDistributedDeviceProfile() = default;

    virtual int32_t PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile) = 0;
    virtual int32_t GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile) = 0;
    virtual int32_t SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) = 0;
    virtual int32_t UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) = 0;
    virtual int32_t SendSubscribeInfos(std::map<std::string, SubscribeInfo> listenerMap) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DeviceProfile.IDistributedDeviceProfile");
};
} // namespace DistributedDeviceProfile
} // namespace OHOS
#endif // OHOS_DP_I_DISTRIBUTED_DEVICE_PROFILE_H
