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

#include "pasteboard_fuse_read_callback_stub.h"

#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
int PasteboardFuseReadCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (code == static_cast<uint32_t>(FuseReadCallbackCode::ON_RESULT)) {
        int32_t status = data.ReadInt32();
        std::vector<uint8_t> fileData;
        data.ReadUInt8Vector(&fileData);
        return OnResult(status, fileData);
    }
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t PasteboardFuseReadCallbackStub::OnResult(int32_t status, const std::vector<uint8_t> &data)
{
    std::lock_guard<std::mutex> lock(result_->mutex);
    if (result_->done) {
        return 0;
    }
    result_->done = true;
    result_->status = status;
    result_->data = data;
    result_->cv.notify_one();
    return 0;
}

int32_t PasteboardFuseReadCallbackStub::WaitForResult(std::shared_ptr<Result> result, int32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(result->mutex);
    if (!result->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&result] { return result->done; })) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "FuseReadCallback timeout");
        return -EIO;
    }
    return result->status;
}
} // namespace MiscServices
} // namespace OHOS
