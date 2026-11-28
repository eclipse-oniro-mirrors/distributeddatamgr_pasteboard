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

#include "pasteboard_fuse_getter_client.h"

#include <cinttypes>

#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
int32_t PasteboardFuseGetterClient::OnRead(
    uint32_t recordId, const std::string &id, int64_t offset, int32_t length, std::vector<uint8_t> &data)
{
    auto it = providers_.find(recordId);
    if (it == providers_.end() || it->second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "OnRead no provider for recordId:%{public}u", recordId);
        return ERR_INVALID_VALUE;
    }
    return it->second->OnRead(recordId, id, offset, length, data);
}

int32_t PasteboardFuseGetterClient::OnStat(
    uint32_t recordId, const std::string &id, FuseFileStat &stat)
{
    auto it = providers_.find(recordId);
    if (it == providers_.end() || it->second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "OnStat no provider for recordId:%{public}u", recordId);
        return ERR_INVALID_VALUE;
    }
    return it->second->OnStat(recordId, id, stat);
}

int32_t PasteboardFuseGetterClient::OnCancel(
    uint32_t recordId, const std::vector<std::string> &idList)
{
    auto it = providers_.find(recordId);
    if (it == providers_.end() || it->second == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "OnCancel no provider for recordId:%{public}u", recordId);
        return ERR_INVALID_VALUE;
    }
    return it->second->OnCancel(recordId, idList);
}
} // namespace MiscServices
} // namespace OHOS
