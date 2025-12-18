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

#include "pasteboard_ability_manager.h"

#include "ability_manager_interface.h"
#include "iservice_registry.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "system_ability_definition.h"

namespace OHOS::MiscServices {
using namespace OHOS::AAFwk;

const std::u16string ABILITY_MGR_DESCRIPTOR = u"ohos.aafwk.AbilityManager";
constexpr int DEFAULT_INVAL_VALUE = -1;

sptr<IRemoteObject> PasteboardAbilityManager::GetAbilityManagerService()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(systemAbilityManager != nullptr, nullptr, PASTEBOARD_MODULE_SERVICE,
        "get samgr failed");

    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(ABILITY_MGR_SERVICE_ID);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(remoteObject != nullptr, nullptr, PASTEBOARD_MODULE_SERVICE,
        "get ability manager failed");

    return remoteObject;
}

int32_t PasteboardAbilityManager::CheckUIExtensionIsFocused(uint32_t tokenId, bool &isFocused)
{
    sptr<IRemoteObject> remote = GetAbilityManagerService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(remote != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "get ability manager failed");

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool ret = data.WriteInterfaceToken(ABILITY_MGR_DESCRIPTOR);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_DATA, PASTEBOARD_MODULE_SERVICE,
        "write interface token failed");
    ret = data.WriteUint32(tokenId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_DATA, PASTEBOARD_MODULE_SERVICE,
        "write token id failed");

    uint32_t cmd = static_cast<uint32_t>(AbilityManagerInterfaceCode::CHECK_UI_EXTENSION_IS_FOCUSED);
    int32_t result = remote->SendRequest(cmd, data, reply, option);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result == NO_ERROR, result, PASTEBOARD_MODULE_SERVICE,
        "send request failed, ret=%{public}d", result);

    ret = reply.ReadBool(isFocused);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_REPLY, PASTEBOARD_MODULE_SERVICE,
        "read result failed");
    return NO_ERROR;
}

int32_t PasteboardAbilityManager::StartAbility(const OHOS::AAFwk::Want &want)
{
    sptr<IRemoteObject> remote = GetAbilityManagerService();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(remote != nullptr,
        static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR), PASTEBOARD_MODULE_SERVICE,
        "get ability manager failed");

    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool ret = data.WriteInterfaceToken(ABILITY_MGR_DESCRIPTOR);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_DATA, PASTEBOARD_MODULE_SERVICE,
        "write interface token failed");
    ret = data.WriteParcelable(&want);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_DATA, PASTEBOARD_MODULE_SERVICE,
        "write want failed");
    ret = data.WriteInt32(DEFAULT_INVAL_VALUE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_DATA, PASTEBOARD_MODULE_SERVICE,
        "write value1 failed");
    ret = data.WriteInt32(DEFAULT_INVAL_VALUE);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_DATA, PASTEBOARD_MODULE_SERVICE,
        "write value2 failed");

    uint32_t cmd = static_cast<uint32_t>(AbilityManagerInterfaceCode::START_ABILITY);
    int32_t result = remote->SendRequest(cmd, data, reply, option);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result == NO_ERROR, result, PASTEBOARD_MODULE_SERVICE,
        "send request failed, ret=%{public}d", result);

    ret = reply.ReadInt32(result);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, ERR_INVALID_REPLY, PASTEBOARD_MODULE_SERVICE,
        "read result failed");
    return result;
}
} // namespace OHOS::MiscServices
