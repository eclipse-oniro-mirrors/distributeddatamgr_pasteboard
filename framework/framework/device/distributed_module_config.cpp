/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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
#include "distributed_module_config.h"

#include "dev_manager.h"
#include "dev_profile.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
using namespace DistributedHardware;
bool DistributedModuleConfig::status_ = false;
size_t DistributedModuleConfig::deviceNums_ = 0;
DistributedModuleConfig::Observer DistributedModuleConfig::observer_ = nullptr;
bool DistributedModuleConfig::IsOn()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "device online nums: %{public}zu", deviceNums_);
    return status_;
}

void DistributedModuleConfig::Watch(Observer observer)
{
    observer_ = std::move(observer);
}

void DistributedModuleConfig::Notify()
{
    bool newStatus = GetEnabledStatus();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "Notify, status:%{public}d, newStatus:%{public}d", status_, newStatus);
    if (newStatus != status_) {
        status_ = newStatus;
        if (observer_ != nullptr) {
            observer_(newStatus);
        }
    }
}

bool DistributedModuleConfig::GetEnabledStatus()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetEnabledStatus start.");
    if (!DevProfile::GetInstance().GetLocalEnable()) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetLocalEnable false.");
        return false;
    }

    auto deviceIds = DevManager::GetInstance().GetNetworkIds();
    deviceNums_ = deviceIds.size();
    std::string remoteEnabledStatus = "false";
    for (auto &id : deviceIds) {
        DevProfile::GetInstance().GetEnabledStatus(id, remoteEnabledStatus);
        if (remoteEnabledStatus == "true") {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "remoteEnabledStatus true.");
            return true;
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "remoteEnabledStatus = %{public}s.", remoteEnabledStatus.c_str());
    return false;
}
} // namespace MiscServices
} // namespace OHOS