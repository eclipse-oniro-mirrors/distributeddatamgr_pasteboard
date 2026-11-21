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

#ifndef PASTEBOARD_FUSE_GETTER_STUB_H
#define PASTEBOARD_FUSE_GETTER_STUB_H

#include <map>

#include "iremote_stub.h"
#include "ipasteboard_fuse_getter.h"

namespace OHOS {
namespace MiscServices {
class __attribute__((visibility("default"))) PasteboardFuseGetterStub : public IRemoteStub<IPasteboardFuseGetter> {
public:
    PasteboardFuseGetterStub();
    ~PasteboardFuseGetterStub() = default;
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;
    int32_t OnReadAsync(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        const sptr<IRemoteObject> &callback) override { return 0; }
private:
    int32_t HandleRead(MessageParcel& data, MessageParcel& reply);
    int32_t HandleReadAsync(MessageParcel& data, MessageParcel& reply);
    int32_t HandleStat(MessageParcel& data, MessageParcel& reply);
    int32_t HandleCancel(MessageParcel& data, MessageParcel& reply);
    using Handler = int32_t (PasteboardFuseGetterStub::*)(MessageParcel& data, MessageParcel& reply);
    std::map<uint32_t, Handler> memberFuncMap_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_FUSE_GETTER_STUB_H
