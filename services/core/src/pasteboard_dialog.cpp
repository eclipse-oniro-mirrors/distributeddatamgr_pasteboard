/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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

#include "pasteboard_dialog.h"

#include "pasteboard_ability_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace OHOS::AAFwk;

constexpr const char *PASTEBOARD_DIALOG_APP = "com.ohos.pasteboarddialog";
constexpr const char *PASTEBOARD_PROGRESS_ABILITY = "PasteboardProgressAbility";
constexpr const char *PASTEBOARD_TOAST_ABILITY = "ToastExtensionAbility";

int32_t PasteboardDialog::ShowProgress(const ProgressMessageInfo &message)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "showprogress begin");
    Want want;
    want.SetElementName(PASTEBOARD_DIALOG_APP, PASTEBOARD_PROGRESS_ABILITY);
    want.SetAction(PASTEBOARD_PROGRESS_ABILITY);
    want.SetParam("promptText", message.promptText);
    want.SetParam("remoteDeviceName", message.remoteDeviceName);
    want.SetParam("progressKey", message.progressKey);
    want.SetParam("isRemote", message.isRemote);
    want.SetParam("rectLeft", message.left);
    want.SetParam("rectTop", message.top);
    want.SetParam("rectWidth", message.width);
    want.SetParam("rectHeight", message.height);
    if (message.clientCallback != nullptr) {
        want.SetParam("ipcCallback", message.clientCallback);
    }
    if (message.callerToken != nullptr) {
        want.SetParam("tokenKey", message.callerToken);
    }

    int32_t result = PasteboardAbilityManager::StartAbility(want);
    if (result != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "start pasteboard progress failed, result:%{public}d", result);
        return static_cast<int32_t>(PasteboardError::PROGRESS_START_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start pasteboard progress success.");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardDialog::ShowToast(const ToastMessageInfo &message)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "[TOAST_DBG] ShowToast begin, appName:%{public}s", message.appName.c_str());
    Want want;
    want.SetElementName(PASTEBOARD_DIALOG_APP, PASTEBOARD_TOAST_ABILITY);
    want.SetAction(PASTEBOARD_TOAST_ABILITY);
    want.SetParam("appName", message.appName);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "[TOAST_DBG] ShowToast: Want built, app=%{public}s, ability=%{public}s",
        PASTEBOARD_DIALOG_APP, PASTEBOARD_TOAST_ABILITY);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "[TOAST_DBG] ShowToast: calling StartAbility");
    int32_t result = PasteboardAbilityManager::StartAbility(want);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "[TOAST_DBG] ShowToast: StartAbility returned result=%{public}d", result);
    if (result != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "[TOAST_DBG] ShowToast: FAILED, result:%{public}d", result);
        return static_cast<int32_t>(PasteboardError::PROGRESS_START_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "[TOAST_DBG] ShowToast: success");
    return static_cast<int32_t>(PasteboardError::E_OK);
}
} // namespace OHOS::MiscServices
