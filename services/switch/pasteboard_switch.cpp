/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "pasteboard_switch.h"

#include "datashare_delegate.h"
#include "dev_profile.h"
#include "parameters.h"
#include "pasteboard_event_ue.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace UeReporter;
const constexpr char *DISTRIBUTED_PASTEBOARD_SWITCH = "distributed_pasteboard_switch";
constexpr const char *SUPPORT_STATUS = "1";
constexpr int32_t ERROR_USERID = -1;
constexpr const char *UE_SWITCH_STATUS = "PASTEBOARD_SWITCH_STATUS";
constexpr const char *DISABLE_DISTRIBUTED_PASTEBOARD = "const.pasteboard.disable_crossdevice_clipboard";
PastedSwitch::PastedSwitch() : userId_(ERROR_USERID)
{
    switchObserver_ = new (std::nothrow) PastedSwitchObserver([this]() -> void {
        std::thread thread([userId = userId_, this]() {
            SetSwitch(userId);
        });
        pthread_setname_np(thread.native_handle(), "SetSwitch");
        thread.detach();
    });
}

void PastedSwitch::Init(int32_t userId)
{
    if (OHOS::system::GetBoolParameter(DISABLE_DISTRIBUTED_PASTEBOARD, false)) {
        DevProfile::GetInstance().PutDeviceStatus(false);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "not support distributed pasteboard.");
        return;
    }
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "userId invalid.");
        return;
    }
    this->userId_ = userId;
    DataShareDelegate::GetInstance().SetUserId(userId_);
    DataShareDelegate::GetInstance().RegisterObserver(DISTRIBUTED_PASTEBOARD_SWITCH, switchObserver_);
    SetSwitch(userId);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Init SetSwitch %{public}d", userId);
    ReportUeSwitchEvent();
}

void PastedSwitch::SetSwitch(int32_t userId)
{
    std::string value;
    DataShareDelegate::GetInstance().SetUserId(userId);
    DataShareDelegate::GetInstance().GetValue(DISTRIBUTED_PASTEBOARD_SWITCH, value);
    if (value.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "empty switch, set status enable, userId=%{public}d", userId);
        DevProfile::GetInstance().PutDeviceStatus(true);
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "switch status=%{public}s, userId=%{public}d", value.c_str(), userId);
    DevProfile::GetInstance().PutDeviceStatus(value == SUPPORT_STATUS);
}

void PastedSwitch::DeInit()
{
    DataShareDelegate::GetInstance().UnregisterObserver(DISTRIBUTED_PASTEBOARD_SWITCH, switchObserver_);
}

void PastedSwitch::ReportUeSwitchEvent()
{
    std::string value;
    DataShareDelegate::GetInstance().GetValue(DISTRIBUTED_PASTEBOARD_SWITCH, value);
    UE_SWITCH(UE_SWITCH_STATUS, UeReporter::UE_STATUS_TYPE,
        (value == SUPPORT_STATUS) ? UeReporter::SwitchStatus::SWITCH_OPEN : UeReporter::SwitchStatus::SWITCH_CLOSE);
}

void PastedSwitchObserver::OnChange()
{
    if (func_ != nullptr) {
        func_();
    }
}
} // namespace OHOS::MiscServices