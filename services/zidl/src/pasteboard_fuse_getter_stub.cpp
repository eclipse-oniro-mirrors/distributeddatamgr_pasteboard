/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "pasteboard_fuse_getter_stub.h"

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "pasteboard_fuse_read_callback_proxy.h"
#include "pasteboard_hilog.h"
#include "pasteboard_serv_ipc_interface_code.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::Security::PasteboardServ;

static bool IsNativeCallingToken()
{
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    if (tokenId == 0) {
        return false;
    }
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (tokenType != Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE) {
        return false;
    }
    constexpr uid_t CLIPBOARD_UID = 3816;
    return IPCSkeleton::GetCallingUid() == CLIPBOARD_UID;
}

PasteboardFuseGetterStub::PasteboardFuseGetterStub()
{
    memberFuncMap_[static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ)] =
        &PasteboardFuseGetterStub::HandleRead;
    memberFuncMap_[static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ_ASYNC)] =
        &PasteboardFuseGetterStub::HandleReadAsync;
    memberFuncMap_[static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_STAT)] =
        &PasteboardFuseGetterStub::HandleStat;
    memberFuncMap_[static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_CANCEL)] =
        &PasteboardFuseGetterStub::HandleCancel;
}

int PasteboardFuseGetterStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    PASTEBOARD_HILOGI(
        PASTEBOARD_MODULE_CLIENT, "code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!IsNativeCallingToken()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "reject non-native caller, code:%{public}u", code);
        return ERR_INVALID_VALUE;
    }
    std::u16string localDescriptor = PasteboardFuseGetterStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "remote descriptor is not equal to local descriptor");
        return ERR_INVALID_VALUE;
    }
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t PasteboardFuseGetterStub::HandleRead(MessageParcel &data, MessageParcel &reply)
{
    uint32_t recordId = data.ReadUint32();
    std::string id = data.ReadString();
    int64_t offset = data.ReadInt64();
    int32_t length = data.ReadInt32();
    if (length < 0 || length > 5 * 1024 * 1024 || offset < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "HandleRead invalid params, length:%{public}d, offset:%{public}lld",
            length, static_cast<long long>(offset));
        return ERR_INVALID_VALUE;
    }
    std::vector<uint8_t> fileData;
    auto result = OnRead(recordId, id, offset, length, fileData);
    if (!reply.WriteInt32(result)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to write result:%{public}d", result);
        return ERR_INVALID_VALUE;
    }
    if (!reply.WriteInt32(static_cast<int32_t>(fileData.size()))) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to write data size");
        return ERR_INVALID_VALUE;
    }
    if (!fileData.empty()) {
        if (!reply.WriteUInt8Vector(fileData)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to write file data");
            return ERR_INVALID_VALUE;
        }
    }
    return ERR_OK;
}

int32_t PasteboardFuseGetterStub::HandleReadAsync(MessageParcel &data, MessageParcel &reply)
{
    uint32_t recordId = data.ReadUint32();
    std::string id = data.ReadString();
    int64_t offset = data.ReadInt64();
    int32_t length = data.ReadInt32();
    if (length < 0 || length > 5 * 1024 * 1024 || offset < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "HandleReadAsync invalid params");
        return ERR_INVALID_VALUE;
    }
    auto callbackRemote = data.ReadRemoteObject();
    if (callbackRemote == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "HandleReadAsync: callback remote is null");
        return ERR_INVALID_VALUE;
    }
    std::vector<uint8_t> fileData;
    int32_t result = OnRead(recordId, id, offset, length, fileData);
    auto callbackProxy = new (std::nothrow) PasteboardFuseReadCallbackProxy(callbackRemote);
    if (callbackProxy != nullptr) {
        callbackProxy->OnResult(result, fileData);
        delete callbackProxy;
    }
    return ERR_OK;
}

int32_t PasteboardFuseGetterStub::HandleStat(MessageParcel &data, MessageParcel &reply)
{
    uint32_t recordId = data.ReadUint32();
    std::string id = data.ReadString();
    FuseFileStat stat;
    auto result = OnStat(recordId, id, stat);
    if (!reply.WriteInt32(result)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to write result:%{public}d", result);
        return ERR_INVALID_VALUE;
    }
    reply.WriteString(stat.id);
    reply.WriteString(stat.name);
    reply.WriteInt64(stat.size);
    reply.WriteInt64(stat.modifyTime);
    reply.WriteInt32(stat.type);
    return ERR_OK;
}

int32_t PasteboardFuseGetterStub::HandleCancel(MessageParcel &data, MessageParcel &reply)
{
    uint32_t recordId = data.ReadUint32();
    std::vector<std::string> idList;
    if (!data.ReadStringVector(&idList)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to read idList");
        return ERR_INVALID_VALUE;
    }
    auto result = OnCancel(recordId, idList);
    if (!reply.WriteInt32(result)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Failed to write result:%{public}d", result);
        return ERR_INVALID_VALUE;
    }
    return ERR_OK;
}
} // namespace MiscServices
} // namespace OHOS
