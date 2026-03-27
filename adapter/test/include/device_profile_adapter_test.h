/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DEVICE_PROFILE_ADAPTER_TEST_H
#define OHOS_DEVICE_PROFILE_ADAPTER_TEST_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

#include "device_profile_client.h"

namespace OHOS {
using namespace OHOS::DistributedDeviceProfile;

class IDeviceProfileClientInterface {
public:
    virtual ~IDeviceProfileClientInterface() = default;
    virtual int32_t PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile) = 0;
    virtual int32_t GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile) = 0;
    virtual int32_t SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) = 0;
    virtual int32_t UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) = 0;
    virtual void SendSubscribeInfos() = 0;
    virtual void ClearDeviceProfileService() = 0;
};

class DeviceProfileClientInterfaceMock : public IDeviceProfileClientInterface {
public:
    DeviceProfileClientInterfaceMock() = default;
    ~DeviceProfileClientInterfaceMock() override = default;
    static DeviceProfileClientInterfaceMock *GetMock()
    {
        return mock_;
    }
    static void SetMock(DeviceProfileClientInterfaceMock *mock)
    {
        mock_ = mock;
    }

    MOCK_METHOD(int32_t, PutCharacteristicProfile, (const CharacteristicProfile &characteristicProfile), (override));
    MOCK_METHOD(int32_t, GetCharacteristicProfile, (const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile), (override));
    MOCK_METHOD(int32_t, SubscribeDeviceProfile, (const SubscribeInfo &subscribeInfo), (override));
    MOCK_METHOD(int32_t, UnSubscribeDeviceProfile, (const SubscribeInfo &subscribeInfo), (override));
    MOCK_METHOD(void, SendSubscribeInfos, (), (override));
    MOCK_METHOD(void, ClearDeviceProfileService, (), (override));

private:
    static inline DeviceProfileClientInterfaceMock *mock_ = nullptr;
    static bool mockFlag_;
};

} // namespace OHOS

#endif // OHOS_DEVICE_PROFILE_ADAPTER_TEST_H