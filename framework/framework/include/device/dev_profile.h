/*
 * Copyright (C) 2022-2025 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_DEV_PROFILE_H
#define PASTE_BOARD_DEV_PROFILE_H

#include "api/visibility.h"
#include "common/concurrent_map.h"
#include "ffrt.h"

#include <memory>
#include <unordered_set>

namespace OHOS {
namespace MiscServices {
class DeviceProfileProxy;

class API_EXPORT DevProfile {
public:
    using Observer = std::function<void(bool isEnable)>;
    static DevProfile &GetInstance();
    int32_t GetDeviceStatus(const std::string &udid, bool &status);
    void PutDeviceStatus(bool status);
    bool GetDeviceVersion(const std::string &udid, uint32_t &deviceVersion);
    void SubscribeProfileEvent(const std::string &udid);
    void UnSubscribeProfileEvent(const std::string &udid);
    void UnSubscribeAllProfileEvents();
    void SendSubscribeInfos();
    void ClearDeviceProfileService();
    void Watch(Observer observer);
    void UpdateEnabledStatus(const std::string &udid, bool status);
    void EraseEnabledStatus(const std::string &udid);

private:
    DevProfile() = default;
    virtual ~DevProfile() = default;
    static void OnProfileUpdate(const std::string &udid, bool status);
    void Notify(bool isEnable);
    void PostDelayReleaseProxy();

    Observer observer_ = nullptr;
    ConcurrentMap<std::string, bool> enabledStatusCache_;
    std::shared_ptr<DeviceProfileProxy> proxy_ = nullptr;
    ffrt::recursive_mutex proxyMutex_;
    std::unordered_set<std::string> subscribeUdidList_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_DEV_PROFILE_H
