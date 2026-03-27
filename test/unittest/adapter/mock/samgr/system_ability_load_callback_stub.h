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

#ifndef SYSTEM_ABILITY_LOAD_CALLBACK_STUB_H
#define SYSTEM_ABILITY_LOAD_CALLBACK_STUB_H

#include "iremote_stub.h"
#include "iremote_broker.h"

namespace OHOS {
class ISystemAbilityLoadCallback : public IRemoteBroker {
public:
    virtual ~ISystemAbilityLoadCallback() = default;

    virtual void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject)
    {
        (void)systemAbilityId;
        (void)remoteObject;
    }

    virtual void OnLoadSystemAbilityFail(int32_t systemAbilityId)
    {
        (void)systemAbilityId;
    }

    DECLARE_INTERFACE_DESCRIPTOR(u"ISystemAbilityLoadCallback");
};

class SystemAbilityLoadCallbackStub : public IRemoteStub<ISystemAbilityLoadCallback> {
public:
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        (void)code;
        (void)data;
        (void)reply;
        (void)option;
        return 0;
    }
};
} // namespace OHOS
#endif // SYSTEM_ABILITY_LOAD_CALLBACK_STUB_H
