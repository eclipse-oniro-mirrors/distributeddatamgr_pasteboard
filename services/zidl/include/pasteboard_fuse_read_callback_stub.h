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

#ifndef PASTEBOARD_FUSE_READ_CALLBACK_STUB_H
#define PASTEBOARD_FUSE_READ_CALLBACK_STUB_H

#include <condition_variable>
#include <mutex>
#include <vector>

#include "iremote_stub.h"
#include "ipasteboard_fuse_read_callback.h"

namespace OHOS {
namespace MiscServices {
class PasteboardFuseReadCallbackStub : public IRemoteStub<IPasteboardFuseReadCallback> {
public:
    PasteboardFuseReadCallbackStub() = default;
    ~PasteboardFuseReadCallbackStub() override = default;

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

    int32_t OnResult(int32_t status, const std::vector<uint8_t> &data) override;

    struct Result {
        std::mutex mutex;
        std::condition_variable cv;
        bool done = false;
        int32_t status = -EIO;
        std::vector<uint8_t> data;
    };
    int32_t WaitForResult(int32_t timeoutMs);
    std::vector<uint8_t> TakeData();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    bool done_ = false;
    int32_t status_ = -EIO;
    std::vector<uint8_t> data_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_FUSE_READ_CALLBACK_STUB_H
