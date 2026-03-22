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

#ifndef OHOS_DP_DEVICE_PROFILE_CLIENT_MOCK_H
#define OHOS_DP_DEVICE_PROFILE_CLIENT_MOCK_H

#include <map>
#include <mutex>
#include <string>

#include "i_distributed_device_profile.h"
#include "iremote_object.h"

namespace OHOS {
namespace DistributedDeviceProfile {

class DeviceProfileClientMock {
public:
    static DeviceProfileClientMock &GetInstance()
    {
        static DeviceProfileClientMock instance;
        return instance;
    }

    int32_t PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile)
    {
        putCharacteristicProfileCalled_ = true;
        lastCharacteristicProfile_ = characteristicProfile;
        return putCharacteristicProfileReturn_;
    }

    int32_t GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile)
    {
        getCharacteristicProfileCalled_ = true;
        lastDeviceId_ = deviceId;
        lastServiceName_ = serviceName;
        lastCharacteristicId_ = characteristicId;
        characteristicProfile = lastReturnedProfile_;
        return getCharacteristicProfileReturn_;
    }

    int32_t SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo)
    {
        subscribeDeviceProfileCalled_ = true;
        lastSubscribeInfo_ = subscribeInfo;
        lastListener_ = subscribeInfo.GetListener();
        return subscribeDeviceProfileReturn_;
    }

    int32_t UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo)
    {
        unSubscribeDeviceProfileCalled_ = true;
        lastSubscribeInfo_ = subscribeInfo;
        return unSubscribeDeviceProfileReturn_;
    }

    void SendSubscribeInfos()
    {
        sendSubscribeInfosCalled_ = true;
    }

    void ClearDeviceProfileService()
    {
        clearDeviceProfileServiceCalled_ = true;
    }

    void SetPutCharacteristicProfileReturn(int32_t ret)
    {
        putCharacteristicProfileReturn_ = ret;
    }

    void SetGetCharacteristicProfileReturn(int32_t ret)
    {
        getCharacteristicProfileReturn_ = ret;
    }

    void SetSubscribeDeviceProfileReturn(int32_t ret)
    {
        subscribeDeviceProfileReturn_ = ret;
    }

    void SetUnSubscribeDeviceProfileReturn(int32_t ret)
    {
        unSubscribeDeviceProfileReturn_ = ret;
    }

    void SetReturnedProfile(const CharacteristicProfile &profile)
    {
        lastReturnedProfile_ = profile;
    }

    bool PutCharacteristicProfileCalled() const
    {
        return putCharacteristicProfileCalled_;
    }

    bool GetCharacteristicProfileCalled() const
    {
        return getCharacteristicProfileCalled_;
    }

    bool SubscribeDeviceProfileCalled() const
    {
        return subscribeDeviceProfileCalled_;
    }

    bool UnSubscribeDeviceProfileCalled() const
    {
        return unSubscribeDeviceProfileCalled_;
    }

    bool SendSubscribeInfosCalled() const
    {
        return sendSubscribeInfosCalled_;
    }

    bool ClearDeviceProfileServiceCalled() const
    {
        return clearDeviceProfileServiceCalled_;
    }

    sptr<IRemoteObject> GetLastListener() const
    {
        return lastListener_;
    }

    void Reset()
    {
        putCharacteristicProfileCalled_ = false;
        getCharacteristicProfileCalled_ = false;
        subscribeDeviceProfileCalled_ = false;
        unSubscribeDeviceProfileCalled_ = false;
        sendSubscribeInfosCalled_ = false;
        clearDeviceProfileServiceCalled_ = false;
        putCharacteristicProfileReturn_ = DP_SUCCESS;
        getCharacteristicProfileReturn_ = DP_SUCCESS;
        subscribeDeviceProfileReturn_ = DP_SUCCESS;
        unSubscribeDeviceProfileReturn_ = DP_SUCCESS;
    }

private:
    bool putCharacteristicProfileCalled_ = false;
    bool getCharacteristicProfileCalled_ = false;
    bool subscribeDeviceProfileCalled_ = false;
    bool unSubscribeDeviceProfileCalled_ = false;
    bool sendSubscribeInfosCalled_ = false;
    bool clearDeviceProfileServiceCalled_ = false;

    int32_t putCharacteristicProfileReturn_ = DP_SUCCESS;
    int32_t getCharacteristicProfileReturn_ = DP_SUCCESS;
    int32_t subscribeDeviceProfileReturn_ = DP_SUCCESS;
    int32_t unSubscribeDeviceProfileReturn_ = DP_SUCCESS;

    CharacteristicProfile lastCharacteristicProfile_;
    CharacteristicProfile lastReturnedProfile_;
    std::string lastDeviceId_;
    std::string lastServiceName_;
    std::string lastCharacteristicId_;
    SubscribeInfo lastSubscribeInfo_;
    sptr<IRemoteObject> lastListener_;
};

} // namespace DistributedDeviceProfile
} // namespace OHOS

#endif // OHOS_DP_DEVICE_PROFILE_CLIENT_MOCK_H
