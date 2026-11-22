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

#ifndef PASTEBOARD_FUSE_GETTER_CLIENT_H
#define PASTEBOARD_FUSE_GETTER_CLIENT_H

#include "pasteboard_fuse_getter_stub.h"

namespace OHOS {
namespace MiscServices {
class __attribute__((visibility("default"))) PasteboardFuseGetterClient : public PasteboardFuseGetterStub {
public:
    explicit PasteboardFuseGetterClient(
        const std::map<uint32_t, sptr<IPasteboardFuseGetter>> &providers)
        : providers_(providers) {}
    ~PasteboardFuseGetterClient() = default;
    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override;
    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override;
    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override;

private:
    std::map<uint32_t, sptr<IPasteboardFuseGetter>> providers_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_FUSE_GETTER_CLIENT_H
