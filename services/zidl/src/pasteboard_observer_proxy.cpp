/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "pasteboard_observer_proxy.h"
#include "pasteboard_hilog.h"
#include "pasteboard_error.h"
#include "pasteboard_serv_ipc_interface_code.h"

using namespace OHOS::Security::PasteboardServ;
namespace OHOS {
namespace MiscServices {
PasteboardObserverProxy::PasteboardObserverProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IPasteboardChangedObserver>(object)
{
}

void PasteboardObserverProxy::OnPasteboardChanged()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option = { MessageOption::TF_ASYNC };
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "write descriptor failed!");
        return;
    }

    int ret = Remote()->SendRequest(
        static_cast<int>(PasteboardObserverInterfaceCode::ON_PASTE_BOARD_CHANGE), data, reply, option);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SendRequest is failed, error code: %{public}d", ret);
    }
    return;
}

void PasteboardObserverProxy::OnPasteboardEvent(const PasteboardChangedEvent &event)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option = { MessageOption::TF_ASYNC };
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start.");
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "write descriptor failed!");
        return;
    }
    PASTEBOARD_CHECK_AND_RETURN_LOGE(data.WriteString(event.bundleName), PASTEBOARD_MODULE_SERVICE,
        "Write bundleName failed");
    PASTEBOARD_CHECK_AND_RETURN_LOGE(data.WriteInt32(event.status), PASTEBOARD_MODULE_SERVICE, "Write status failed");
    PASTEBOARD_CHECK_AND_RETURN_LOGE(data.WriteInt32(event.userId), PASTEBOARD_MODULE_SERVICE, "Write userId failed");
    int ret = Remote()->SendRequest(
        static_cast<int>(PasteboardObserverInterfaceCode::ON_PASTE_BOARD_EVENT), data, reply, option);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SendRequest is failed, error code: %{public}d", ret);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "end.");
    return;
}
} // namespace MiscServices
} // namespace OHOS
