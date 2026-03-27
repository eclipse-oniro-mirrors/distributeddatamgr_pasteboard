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

#ifndef SYSTEM_ABILITY_MANAGER_MOCK_H
#define SYSTEM_ABILITY_MANAGER_MOCK_H

#include <gmock/gmock.h>

#include "iremote_object.h"
#include "if_system_ability_manager.h"

namespace OHOS {
class SystemAbilityManagerMock : public ISystemAbilityManager {
public:
    SystemAbilityManagerMock();
    ~SystemAbilityManagerMock();
    static SystemAbilityManagerMock *GetMock();

    MOCK_METHOD(sptr<IRemoteObject>, CheckSystemAbility, (int32_t systemAbilityId), (override));
    MOCK_METHOD(int32_t, LoadSystemAbility, (int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback),
        (override));

private:
    static inline SystemAbilityManagerMock *mock_ = nullptr;
};

class SystemAbilityManager : public ISystemAbilityManager {
public:
    sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId) override;
    int32_t LoadSystemAbility(int32_t systemAbilityId, const sptr<ISystemAbilityLoadCallback> &callback) override;
};
} // namespace OHOS
#endif // SYSTEM_ABILITY_MANAGER_MOCK_H
