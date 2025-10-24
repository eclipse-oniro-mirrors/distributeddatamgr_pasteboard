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

#ifndef PASTE_BOARD_DISTRIBUTE_MODULE_CONFIG_H
#define PASTE_BOARD_DISTRIBUTE_MODULE_CONFIG_H

#include "device/dm_adapter.h"
#include <atomic>

namespace OHOS {
namespace MiscServices {
class API_EXPORT DistributedModuleConfig : protected DMAdapter::DMObserver {
public:
    DistributedModuleConfig() : status_(false), retrying_(false) {}
    using Observer = std::function<void(bool isOn)>;
    bool IsOn();
    void Watch(const Observer &observer);
    void Init();
    void DeInit();
    uint32_t GetRemoteDeviceMinVersion();
    uint32_t GetRemoteDeviceMaxVersion();
    static constexpr uint32_t FIRST_VERSION = 4;
    static constexpr uint32_t SECOND_VERSION = 5;

protected:
    void Online(const std::string &device) override;
    void Offline(const std::string &device) override;
    void OnReady(const std::string &device) override;

private:
    std::pair<uint32_t, uint32_t> GetRemoteDeviceVersion();
    int32_t GetEnabledStatus();
    void Notify();
    void GetRetryTask();
    size_t GetDeviceNum();
    Observer observer_ = nullptr;
    std::atomic<bool> status_;
    std::atomic<bool> retrying_;
    static constexpr const char *SUPPORT_STATUS = "1";
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_DISTRIBUTE_MODULE_CONFIG_H
