/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYSTEM_PASTEBOARD_IMPL_H
#define SYSTEM_PASTEBOARD_IMPL_H

#include "paste_data_impl.h"

namespace OHOS {
namespace MiscServicesCj {
#define PASTEBOARD_TASK_PROCESSING 12900003
#define PASTEBOARD_COPY_FORBIDDEN 12900004
#define PASTEBOARD_INVALID_PARAMETERS 401

class SystemPasteboardImpl : public OHOS::FFI::FFIData {
    DECL_TYPE(SystemPasteboardImpl, OHOS::FFI::FFIData)
public:
    SystemPasteboardImpl();
    static int32_t GetSystemPasteboardImpl(int64_t &id);
    int32_t SetData(sptr<PasteDataImpl> dataImpl, std::shared_ptr<MiscServices::PasteData> data);
    sptr<PasteDataImpl> GetSystemPasteboardPasteDataImpl();
    int32_t GetData(MiscServices::PasteData &pasteData);
    bool HasData();
    void ClearData();
    bool IsRemoteData();
    bool HasDataType(std::string mimeType);
    std::string GetDataSource();

private:
    sptr<PasteDataImpl> value_;
    std::shared_ptr<MiscServices::PasteData> pasteData_;
};

} // namespace MiscServicesCj
} // namespace OHOS

#endif