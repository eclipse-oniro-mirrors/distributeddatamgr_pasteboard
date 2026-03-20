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

#ifndef OHOS_DP_PROFILE_CHANGE_LISTENER_STUB_H
#define OHOS_DP_PROFILE_CHANGE_LISTENER_STUB_H

#include "iremote_stub.h"

namespace OHOS {
namespace DistributedDeviceProfile {

class TrustDeviceProfile {
public:
    TrustDeviceProfile() = default;
};

class DeviceProfile {
public:
    DeviceProfile() = default;
};

class ServiceProfile {
public:
    ServiceProfile() = default;
};

class CharacteristicProfile {
public:
    CharacteristicProfile() = default;
};

class IProfileChangeListener : public IRemoteBroker {
public:
    IProfileChangeListener() = default;
    virtual ~IProfileChangeListener() = default;

    virtual int32_t OnTrustDeviceProfileAdd(const TrustDeviceProfile &profile) = 0;
    virtual int32_t OnTrustDeviceProfileDelete(const TrustDeviceProfile &profile) = 0;
    virtual int32_t OnTrustDeviceProfileUpdate(const TrustDeviceProfile &oldProfile,
        const TrustDeviceProfile &newProfile) = 0;
    virtual int32_t OnDeviceProfileAdd(const DeviceProfile &profile) = 0;
    virtual int32_t OnDeviceProfileDelete(const DeviceProfile &profile) = 0;
    virtual int32_t OnDeviceProfileUpdate(const DeviceProfile &oldProfile, const DeviceProfile &newProfile) = 0;
    virtual int32_t OnServiceProfileAdd(const ServiceProfile &profile) = 0;
    virtual int32_t OnServiceProfileDelete(const ServiceProfile &profile) = 0;
    virtual int32_t OnServiceProfileUpdate(const ServiceProfile &oldProfile, const ServiceProfile &newProfile) = 0;
    virtual int32_t OnCharacteristicProfileAdd(const CharacteristicProfile &profile) = 0;
    virtual int32_t OnCharacteristicProfileDelete(const CharacteristicProfile &profile) = 0;
    virtual int32_t OnCharacteristicProfileUpdate(const CharacteristicProfile &oldProfile,
        const CharacteristicProfile &newProfile) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DeviceProfile.IProfileChangeListener");
};

class ProfileChangeListenerStub : public IRemoteStub<IProfileChangeListener> {
public:
    ProfileChangeListenerStub() = default;
    virtual ~ProfileChangeListenerStub() = default;

    int32_t OnTrustDeviceProfileAdd(const TrustDeviceProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnTrustDeviceProfileDelete(const TrustDeviceProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnTrustDeviceProfileUpdate(const TrustDeviceProfile &oldProfile,
        const TrustDeviceProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return 0;
    }

    int32_t OnDeviceProfileAdd(const DeviceProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnDeviceProfileDelete(const DeviceProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnDeviceProfileUpdate(const DeviceProfile &oldProfile, const DeviceProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return 0;
    }

    int32_t OnServiceProfileAdd(const ServiceProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnServiceProfileDelete(const ServiceProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnServiceProfileUpdate(const ServiceProfile &oldProfile, const ServiceProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return 0;
    }

    int32_t OnCharacteristicProfileAdd(const CharacteristicProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnCharacteristicProfileDelete(const CharacteristicProfile &profile) override
    {
        (void)profile;
        return 0;
    }

    int32_t OnCharacteristicProfileUpdate(const CharacteristicProfile &oldProfile,
        const CharacteristicProfile &newProfile) override
    {
        (void)oldProfile;
        (void)newProfile;
        return 0;
    }
};

} // namespace DistributedDeviceProfile
} // namespace OHOS

#endif // OHOS_DP_PROFILE_CHANGE_LISTENER_STUB_H
