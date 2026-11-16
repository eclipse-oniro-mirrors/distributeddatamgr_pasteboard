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

#include "pasteboard_fuse_getter_proxy.h"

#include "pasteboard_hilog.h"
#include "pasteboard_serv_ipc_interface_code.h"

namespace OHOS {
namespace MiscServices {
using namespace OHOS::Security::PasteboardServ;
PasteboardFuseGetterProxy::PasteboardFuseGetterProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IPasteboardFuseGetter>(object) {}

int32_t PasteboardFuseGetterProxy::OnRead(
    uint32_t recordId, const std::string &id, int64_t offset, int32_t length, std::vector<uint8_t> &data)
{
    MessageParcel request;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC, FUSE_IPC_TIMEOUT_MS);
    if (!request.WriteInterfaceToken(GetDescriptor())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "write interface token failed");
        return ERR_INVALID_VALUE;
    }
    if (!request.WriteUint32(recordId) || !request.WriteString(id) || !request.WriteInt64(offset) ||
        !request.WriteInt32(length)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "write request params failed");
        return ERR_INVALID_VALUE;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "remote is null");
        return ERR_INVALID_VALUE;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_READ), request, reply, option);
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "SendRequest failed, result:%{public}d", result);
        return result;
    }
    result = reply.ReadInt32();
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "OnRead remote failed, result:%{public}d", result);
        return result;
    }
    int32_t dataSize = reply.ReadInt32();
    if (dataSize > 0) {
        if (!reply.ReadUInt8Vector(&data)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "read file data failed");
            return ERR_INVALID_VALUE;
        }
    }
    return ERR_OK;
}

int32_t PasteboardFuseGetterProxy::OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat)
{
    MessageParcel request;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC, FUSE_IPC_TIMEOUT_MS);
    if (!request.WriteInterfaceToken(GetDescriptor())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "write interface token failed");
        return ERR_INVALID_VALUE;
    }
    if (!request.WriteUint32(recordId) || !request.WriteString(id)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "write id failed");
        return ERR_INVALID_VALUE;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "remote is null");
        return ERR_INVALID_VALUE;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_STAT), request, reply, option);
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "SendRequest failed, result:%{public}d", result);
        return result;
    }
    result = reply.ReadInt32();
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "OnStat remote failed, result:%{public}d", result);
        return result;
    }
    stat.id = reply.ReadString();
    stat.name = reply.ReadString();
    stat.size = reply.ReadInt64();
    stat.modifyTime = reply.ReadInt64();
    stat.type = reply.ReadInt32();
    return ERR_OK;
}

int32_t PasteboardFuseGetterProxy::OnCancel(uint32_t recordId, const std::vector<std::string> &idList)
{
    MessageParcel request;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_SYNC, FUSE_IPC_TIMEOUT_MS);
    if (!request.WriteInterfaceToken(GetDescriptor())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "write interface token failed");
        return ERR_INVALID_VALUE;
    }
    if (!request.WriteUint32(recordId) || !request.WriteStringVector(idList)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "write idList failed");
        return ERR_INVALID_VALUE;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "remote is null");
        return ERR_INVALID_VALUE;
    }
    int32_t result = remote->SendRequest(
        static_cast<uint32_t>(PasteboardFuseGetterInterfaceCode::ON_CANCEL), request, reply, option);
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "SendRequest failed, result:%{public}d", result);
        return result;
    }
    return reply.ReadInt32();
}
} // namespace MiscServices
} // namespace OHOS
