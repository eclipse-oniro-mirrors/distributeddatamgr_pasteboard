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

#ifndef PASTEBOARD_DEVICE_PROFILE_ADAPTER_H
#define PASTEBOARD_DEVICE_PROFILE_ADAPTER_H

#include <cstdint>
#include <string>

#include "api/visibility.h"

namespace OHOS {
namespace MiscServices {

class IDeviceProfileAdapter {
public:
    typedef void (*OnProfileUpdateCallback)(const std::string &udid, bool status);

    virtual ~IDeviceProfileAdapter() = default;
    virtual int32_t RegisterUpdateCallback(const OnProfileUpdateCallback callback) = 0;
    virtual int32_t PutDeviceStatus(const std::string &udid, bool status) = 0;
    virtual int32_t GetDeviceStatus(const std::string &udid, bool &status) = 0;
    virtual bool GetDeviceVersion(const std::string &udid, uint32_t &versionId) = 0;
    virtual int32_t SubscribeProfileEvent(const std::string &udid) = 0;
    virtual int32_t UnSubscribeProfileEvent(const std::string &udid) = 0;
    virtual void SendSubscribeInfos() = 0;
    virtual void ClearDeviceProfileService() = 0;
};

extern "C" {
    typedef IDeviceProfileAdapter *(*GetDeviceProfileAdapterFunc)();
    typedef void (*DeinitDeviceProfileAdapterFunc)();

    API_EXPORT IDeviceProfileAdapter *GetDeviceProfileAdapter();
    API_EXPORT void DeinitDeviceProfileAdapter();
}

} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_DEVICE_PROFILE_ADAPTER_H
