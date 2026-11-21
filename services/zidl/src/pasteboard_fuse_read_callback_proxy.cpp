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

#include "pasteboard_fuse_read_callback_proxy.h"

#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
int32_t PasteboardFuseReadCallbackProxy::OnResult(int32_t status, const std::vector<uint8_t> &data)
{
    MessageParcel request;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (!request.WriteInt32(status) || !request.WriteUInt8Vector(data)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "FuseReadCallbackProxy write failed");
        return -EIO;
    }
    auto remote = Remote();
    if (remote == nullptr) {
        return -EIO;
    }
    return remote->SendRequest(static_cast<uint32_t>(FuseReadCallbackCode::ON_RESULT), request, reply, option);
}
} // namespace MiscServices
} // namespace OHOS
