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

#ifndef I_PASTEBOARD_FUSE_GETTER_H
#define I_PASTEBOARD_FUSE_GETTER_H

#include "iremote_broker.h"

namespace OHOS {
namespace MiscServices {
inline constexpr int32_t MAX_READ_SIZE = 1 * 1024 * 1024;

struct FuseFileStat {
    std::string id;
    std::string name;
    int64_t size = 0;
    int64_t modifyTime = 0;
    int32_t type = 0; // 0=file, 1=dir
};

class IPasteboardFuseGetter : public IRemoteBroker {
public:
    virtual ~IPasteboardFuseGetter() = default;
    virtual int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) = 0;
    virtual int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) = 0;
    virtual int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) = 0;
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.miscservices.pasteboard.IPasteboardFuseGetter");
};
} // namespace MiscServices
} // namespace OHOS
#endif // I_PASTEBOARD_FUSE_GETTER_H
