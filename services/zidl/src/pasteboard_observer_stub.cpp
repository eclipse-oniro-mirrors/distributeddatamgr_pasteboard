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

#include "pasteboard_observer_stub.h"

#include "pasteboard_hilog.h"
#include "pasteboard_error.h"
#include "pasteboard_serv_ipc_interface_code.h"

using namespace OHOS::Security::PasteboardServ;
namespace OHOS {
namespace MiscServices {
PasteboardObserverStub::PasteboardObserverStub()
{
    memberFuncMap_[static_cast<uint32_t>(PasteboardObserverInterfaceCode::ON_PASTE_BOARD_CHANGE)] =
        &PasteboardObserverStub::OnPasteboardChangedStub;
    memberFuncMap_[static_cast<uint32_t>(PasteboardObserverInterfaceCode::ON_PASTE_BOARD_EVENT)] =
        &PasteboardObserverStub::OnPasteboardEventStub;
}

int32_t PasteboardObserverStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start##code = %{public}u", code);
    std::u16string myDescriptor = PasteboardObserverStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (myDescriptor != remoteDescriptor) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "end##descriptor checked fail");
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    pid_t pid = IPCSkeleton::GetCallingPid();
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "CallingPid = %{public}d, CallingUid = %{public}d, code = %{public}u", pid, uid, code);
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    int ret = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "end##ret = %{public}d", ret);
    return ret;
}

int32_t PasteboardObserverStub::OnPasteboardChangedStub(MessageParcel &data, MessageParcel &reply)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start.");
    OnPasteboardChanged();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "end.");
    return ERR_OK;
}

int32_t PasteboardObserverStub::OnPasteboardEventStub(MessageParcel &data, MessageParcel &reply)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "start.");
    PasteboardChangedEvent event;
    event.bundleName = data.ReadString();
    event.status = data.ReadInt32();
    if (!data.ReadInt32(event.status)) {
        return static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR);
    }
    if (!data.ReadInt32(event.userId)) {
        return static_cast<int32_t>(PasteboardError::DESERIALIZATION_ERROR);
    }
    OnPasteboardEvent(event);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "end.");
    return ERR_OK;
}

PasteboardObserverStub::~PasteboardObserverStub()
{
    memberFuncMap_.clear();
}
} // namespace MiscServices
} // namespace OHOS
