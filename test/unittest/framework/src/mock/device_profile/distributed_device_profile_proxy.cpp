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

#include "distributed_device_profile_proxy.h"
#include "distributed_device_profile_errors.h"

namespace OHOS {
namespace DistributedDeviceProfile {
int32_t DistributedDeviceProfileProxy::PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile)
{
    (void)characteristicProfile;
    return DP_SUCCESS;
}

int32_t DistributedDeviceProfileProxy::GetCharacteristicProfile(const std::string &deviceId,
    const std::string &serviceName, const std::string &characteristicId, CharacteristicProfile &characteristicProfile)
{
    (void)deviceId;
    (void)serviceName;
    (void)characteristicId;
    (void)characteristicProfile;
    return DP_SUCCESS;
}

int32_t DistributedDeviceProfileProxy::SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo)
{
    (void)subscribeInfo;
    return DP_SUCCESS;
}

int32_t DistributedDeviceProfileProxy::UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo)
{
    (void)subscribeInfo;
    return DP_SUCCESS;
}

int32_t DistributedDeviceProfileProxy::SendSubscribeInfos(std::map<std::string, SubscribeInfo> listenerMap)
{
    (void)listenerMap;
    return DP_SUCCESS;
}
} // namespace DistributedDeviceProfile
} // namespace OHOS
