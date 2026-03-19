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

#ifndef PASTEBOARD_DEVICE_PROFILE_CLIENT_MOCK_H
#define PASTEBOARD_DEVICE_PROFILE_CLIENT_MOCK_H

#include <gmock/gmock.h>
#include <string>

#include "device_profile_client.h"

namespace OHOS {
namespace DistributedDeviceProfile {

class DeviceProfileClientMock : public DeviceProfileClient {
public:
    DeviceProfileClientMock();
    ~DeviceProfileClientMock();
    static DeviceProfileClientMock *GetMock();

    MOCK_METHOD(int32_t, PutCharacteristicProfile, (const CharacteristicProfile &characteristicProfile), (override));
    MOCK_METHOD(int32_t, GetCharacteristicProfile, (const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile), (override));
    MOCK_METHOD(int32_t, SubscribeDeviceProfile, (const SubscribeInfo &subscribeInfo), (override));
    MOCK_METHOD(int32_t, UnSubscribeDeviceProfile, (const SubscribeInfo &subscribeInfo), (override));
    MOCK_METHOD(void, SendSubscribeInfos, (), (override));
    MOCK_METHOD(void, ClearDeviceProfileService, (), (override));

private:
    static inline DeviceProfileClientMock *mock_ = nullptr;
};
} // namespace DistributedDeviceProfile
} // namespace OHOS

#endif // PASTEBOARD_DEVICE_PROFILE_CLIENT_MOCK_H
