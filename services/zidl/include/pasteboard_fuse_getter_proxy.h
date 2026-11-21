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

#ifndef PASTEBOARD_FUSE_GETTER_PROXY_H
#define PASTEBOARD_FUSE_GETTER_PROXY_H

#include "iremote_proxy.h"
#include "ipasteboard_fuse_getter.h"

namespace OHOS {
namespace MiscServices {
// IPC timeout for FUSE getter calls (ms). NAPI callback has 5s timeout;
// proxy gives 10s margin for IPC overhead + process scheduling.
constexpr int FUSE_IPC_TIMEOUT_MS = 10000;
class PasteboardFuseGetterProxy final : public IRemoteProxy<IPasteboardFuseGetter> {
public:
    explicit PasteboardFuseGetterProxy(const sptr<IRemoteObject> &object);
    ~PasteboardFuseGetterProxy() = default;
    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override;
    int32_t OnReadAsync(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        const sptr<IRemoteObject> &callback) override;
    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override;
    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override;
private:
    static inline BrokerDelegator<PasteboardFuseGetterProxy> delegator_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_FUSE_GETTER_PROXY_H
