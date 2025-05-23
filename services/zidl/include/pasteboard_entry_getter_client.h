/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_ENTRY_GETTER_CLIENT_H
#define PASTEBOARD_ENTRY_GETTER_CLIENT_H

#include "pasteboard_entry_getter_stub.h"

namespace OHOS {
namespace UDMF {
class EntryGetter;
}
namespace MiscServices {
class PasteboardEntryGetterClient : public PasteboardEntryGetterStub {
public:
    explicit PasteboardEntryGetterClient(const std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters);
    ~PasteboardEntryGetterClient() = default;
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry& value) override;
private:
    std::map<uint32_t, std::shared_ptr<UDMF::EntryGetter>> entryGetters_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_ENTRY_GETTER_CLIENT_H