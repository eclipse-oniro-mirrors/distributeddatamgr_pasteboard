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

#ifndef INTERFACES_INNERKITS_SAMGR_INCLUDE_IF_SYSTEM_ABILITY_MANAGER_H
#define INTERFACES_INNERKITS_SAMGR_INCLUDE_IF_SYSTEM_ABILITY_MANAGER_H

#include "iremote_object.h"
#include "system_ability_load_callback_stub.h"

namespace OHOS {
class ISystemAbilityManager : public IRemoteBroker {
public:
    virtual sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) = 0;
    virtual int32_t LoadSystemAbility(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback) = 0;

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};
} // namespace OHOS

#endif // INTERFACES_INNERKITS_SAMGR_INCLUDE_IF_SYSTEM_ABILITY_MANAGER_H
