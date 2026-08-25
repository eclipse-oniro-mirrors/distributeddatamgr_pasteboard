/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "pasteboard_hilog.h"
#include "pasteboard_hml_manager.h"
#include "system_ability_definition.h"

namespace OHOS::MiscServices {

constexpr int32_t WIFI_SUCCESS = 0;
constexpr int32_t WIFI_FAILED = -1;
constexpr int32_t RETRY_INTERVAL_SEC = 10;
constexpr int32_t MAX_RETRY_COUNT = 3;
constexpr int32_t LOADSA_TIMEOUT_MS = 4000;
constexpr int32_t WIFI_OPT_SUCCESS = 0;
constexpr uint32_t WIFI_MGR_GET_DEVICE_SERVICE = 0;
constexpr uint32_t WIFI_SVR_CMD_IS_WLAN_SUPPORTED = 4192;

int32_t PasteboardHmlManager::GetSystemAbility(sptr<IRemoteObject> &remoteObject)
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(samgr != nullptr, WIFI_FAILED,
        PASTEBOARD_MODULE_SERVICE, "Get samgr failed.");

    remoteObject = samgr->CheckSystemAbility(WIFI_DEVICE_SYS_ABILITY_ID);
    if (remoteObject != nullptr) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Get wifi SA succeed.");
        return WIFI_SUCCESS;
    }

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "remoteObject is null.");
    remoteObject = samgr->LoadSystemAbility(WIFI_DEVICE_SYS_ABILITY_ID, LOADSA_TIMEOUT_MS);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(remoteObject != nullptr, WIFI_FAILED,
        PASTEBOARD_MODULE_SERVICE, "Load wifi SA failed.");
    return WIFI_SUCCESS;
}

int32_t PasteboardHmlManager::IsWlansupported(bool &isSupported)
{
    sptr<IRemoteObject> mgrProxy = nullptr;
    int32_t ret = PasteboardHmlManager::GetSystemAbility(mgrProxy);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, WIFI_FAILED,
        PASTEBOARD_MODULE_SERVICE, "Load wifi SA failed.");

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(u"ohos.wifi.IWifiDeviceMgr");
    data.WriteInt32(0);

    MessageOption option;
    int error = mgrProxy->SendRequest(WIFI_MGR_GET_DEVICE_SERVICE, data, reply, option);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(error == ERR_NONE, WIFI_FAILED,
        PASTEBOARD_MODULE_SERVICE, "SendRequest to get device service failed, error:%{public}d", error);

    sptr<IRemoteObject> deviceProxy = reply.ReadRemoteObject();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(deviceProxy != nullptr, WIFI_FAILED,
        PASTEBOARD_MODULE_SERVICE, "get device proxy from reply failed.");

    MessageParcel data2;
    MessageParcel reply2;
    data2.WriteInterfaceToken(u"ohos.wifi.IWifiDeviceService");
    data2.WriteInt32(0);

    error = deviceProxy->SendRequest(WIFI_SVR_CMD_IS_WLAN_SUPPORTED, data2, reply2, option);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(error == ERR_NONE, WIFI_FAILED,
        PASTEBOARD_MODULE_SERVICE, "SendRequest to check wlan supported failed, error:%{public}d", error);

    int32_t retCode = reply2.ReadInt32();
    if (retCode == WIFI_OPT_SUCCESS) {
        isSupported = reply2.ReadBool();
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "IsWlansupported failed, ret:%{public}d", retCode);
    }
    return retCode;
}

bool PasteboardHmlManager::IsHmlSupported()
{
    bool isWlanSupported = false;
    int32_t ret = 0;

    for (int32_t i = 0; i < MAX_RETRY_COUNT; ++i) {
        ret = IsWlansupported(isWlanSupported);
        if (ret == WIFI_SUCCESS) {
            break;
        }
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "IsWlanSupported failed, retry %{public}d", i);
        if (i < MAX_RETRY_COUNT - 1) {
            sleep(RETRY_INTERVAL_SEC);
        }
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == WIFI_SUCCESS, false,
        PASTEBOARD_MODULE_SERVICE, "IsWlanSupported failed");

    return isWlanSupported;
}
} // namespace OHOS::MiscServices