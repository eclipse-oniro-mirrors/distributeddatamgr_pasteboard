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

#ifndef PASTEBOARD_DEVICE_MANAGER_MOCK_H
#define PASTEBOARD_DEVICE_MANAGER_MOCK_H

#include <gmock/gmock.h>
#include <string>

#include "device_manager_impl.h"

namespace OHOS {
namespace DistributedHardware {

class PasteDeviceManager {
public:
    virtual ~PasteDeviceManager() = default;

public:
    virtual int32_t GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo) = 0;
    virtual int32_t GetDeviceInfo(
        const std::string &pkgName, const std::string &networkId, DmDeviceInfo &deviceInfo) = 0;
    virtual int32_t GetUdidByNetworkId(const std::string &pkgName, const std::string &networkId, std::string &udid) = 0;
    virtual bool IsSameAccount(const std::string &networkId) = 0;
    virtual int32_t GetLocalDeviceType(const std::string &pkgName, int32_t &deviceType) = 0;
    virtual int32_t GetTrustedDeviceList(
        const std::string &pkgName, const std::string &extra, std::vector<DmDeviceInfo> &deviceList) = 0;

public:
    static inline std::shared_ptr<PasteDeviceManager> pasteDeviceManager = nullptr;
};

class DeviceManagerMock : public PasteDeviceManager {
public:
    MOCK_METHOD(int32_t, GetLocalDeviceInfo, (const std::string &, DmDeviceInfo &));
    MOCK_METHOD(int32_t, GetDeviceInfo, (const std::string &, const std::string &, DmDeviceInfo &));
    MOCK_METHOD(int32_t, GetUdidByNetworkId, (const std::string &, const std::string &, std::string &));
    MOCK_METHOD(bool, IsSameAccount, (const std::string &));
    MOCK_METHOD(int32_t, GetLocalDeviceType, (const std::string &, int32_t &));
    MOCK_METHOD(int32_t, GetTrustedDeviceList, (const std::string &, const std::string &, std::vector<DmDeviceInfo> &));
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // PASTEBOARD_DEVICE_MANAGER_MOCK_H