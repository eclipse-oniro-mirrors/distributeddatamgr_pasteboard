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

#include "distributed_device_profile_client_mock.h"

namespace OHOS {
namespace DistributedDeviceProfile {
IMPLEMENT_SINGLE_INSTANCE(DistributedDeviceProfileClient);

int32_t DistributedDeviceProfileClient::GetCharacteristicProfile(const std::string &deviceId,
    const std::string &serviceName, const std::string &characteristicId, CharacteristicProfile &characteristicProfile)
{
    return PasteDistributedDeviceProfileClient::pasteDistributedDeviceProfileClient->GetCharacteristicProfile(
        deviceId, serviceName, characteristicId, characteristicProfile);
}

int32_t DistributedDeviceProfileClient::PutCharacteristicProfile(const CharacteristicProfile &characteristicProfile)
{
    return PasteDistributedDeviceProfileClient::pasteDistributedDeviceProfileClient->PutCharacteristicProfile(
        characteristicProfile);
}

} // namespace DistributedDeviceProfile
} // namespace OHOS