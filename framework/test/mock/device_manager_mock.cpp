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

#include "device_manager_mock.h"

namespace OHOS {
namespace DistributedHardware {
int32_t DeviceManagerImpl::GetLocalDeviceInfo(const std::string &pkgName, DmDeviceInfo &deviceInfo)
{
    return PasteDeviceManager::pasteDeviceManager->GetLocalDeviceInfo(pkgName, deviceInfo);
}
int32_t DeviceManagerImpl::GetDeviceInfo(
    const std::string &pkgName, const std::string networkId, DmDeviceInfo &deviceInfo)
{
    return PasteDeviceManager::pasteDeviceManager->GetDeviceInfo(pkgName, networkId, deviceInfo);
}
int32_t DeviceManagerImpl::GetUdidByNetworkId(
    const std::string &pkgName, const std::string &networkId, std::string &udid)
{
    return PasteDeviceManager::pasteDeviceManager->GetUdidByNetworkId(pkgName, networkId, udid);
}
bool DeviceManagerImpl::IsSameAccount(const std::string &networkId)
{
    return PasteDeviceManager::pasteDeviceManager->IsSameAccount(networkId);
}
int32_t DeviceManagerImpl::GetLocalDeviceType(const std::string &pkgName, int32_t &deviceType)
{
    return PasteDeviceManager::pasteDeviceManager->GetLocalDeviceType(pkgName, deviceType);
}
int32_t DeviceManagerImpl::GetTrustedDeviceList(
    const std::string &pkgName, const std::string &extra, std::vector<DmDeviceInfo> &deviceList)
{
    return PasteDeviceManager::pasteDeviceManager->GetTrustedDeviceList(pkgName, extra, deviceList);
}
} // namespace DistributedHardware
} // namespace OHOS