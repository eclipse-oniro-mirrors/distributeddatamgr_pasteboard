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

#ifndef OHOS_DP_DISTRIBUTED_DEVICE_PROFILE_PROXY_H
#define OHOS_DP_DISTRIBUTED_DEVICE_PROFILE_PROXY_H

#include "i_distributed_device_profile.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace DistributedDeviceProfile {
class DistributedDeviceProfileProxy : public IRemoteProxy<IDistributedDeviceProfile> {
public:
    explicit DistributedDeviceProfileProxy(const sptr<IRemoteObject> &impl)
        : IRemoteProxy<IDistributedDeviceProfile>(impl)
    {
    }

    ~DistributedDeviceProfileProxy()
    {
    }

    int32_t PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile) override;
    int32_t GetCharacteristicProfile(const std::string &deviceId, const std::string &serviceName,
        const std::string &characteristicId, CharacteristicProfile &characteristicProfile) override;
    int32_t SubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) override;
    int32_t UnSubscribeDeviceProfile(const SubscribeInfo &subscribeInfo) override;
    int32_t SendSubscribeInfos(std::map<std::string, SubscribeInfo> listenerMap) override;
};
} // namespace DistributedDeviceProfile
} // namespace OHOS
#endif // OHOS_DP_DISTRIBUTED_DEVICE_PROFILE_PROXY_H
