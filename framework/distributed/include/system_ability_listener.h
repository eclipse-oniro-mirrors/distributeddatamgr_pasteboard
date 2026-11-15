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

#ifndef DISTRIBUTED_PASTEBOARD_FRAMEWORK_SYSTEM_ABILITY_LISTENER_H
#define DISTRIBUTED_PASTEBOARD_FRAMEWORK_SYSTEM_ABILITY_LISTENER_H

#include <functional>
#include <mutex>
#include <unordered_map>

#include "system_ability_status_change_stub.h"

namespace OHOS {
namespace MiscServices {
class SystemAbilityListener {
public:
    enum Status {
        ON_ADD = 1,
        ON_REMOVE = 2,
    };

    struct StatusChangeObserver {
        int32_t systemAbilityId;
        std::function<void(int32_t, Status)> onStatusChanged;
    };

    int32_t AddObserver(StatusChangeObserver &observer, int32_t &observerId);
    int32_t RemoveObserver(int32_t observerId);

    static SystemAbilityListener &GetInstance()
    {
        static SystemAbilityListener instance;
        return instance;
    }

private:
    SystemAbilityListener();
    virtual ~SystemAbilityListener();
    void OnStatusChanged(int32_t systemAbilityId, Status status);

    class StatusChangeCallback;
    sptr<ISystemAbilityStatusChange> statusChangeCallback_ = nullptr;
    int32_t observerId_ = 0;
    std::recursive_mutex mutex_;
    std::unordered_map<int32_t, StatusChangeObserver> observerMap_;
    std::unordered_map<int32_t, uint32_t> refCount_;
};
} // namespace MiscServices
} // namespace OHOS
#endif  // DISTRIBUTED_PASTEBOARD_FRAMEWORK_SYSTEM_ABILITY_LISTENER_H

