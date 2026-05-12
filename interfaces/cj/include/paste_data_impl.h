/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef PASTE_DATA_IMPL_H
#define PASTE_DATA_IMPL_H

#include "paste_data_record_impl.h"
#include "pasteboard_error.h"
#include "pasteboard_ffi.h"
#include "tlv_buffer.h"

namespace OHOS {
namespace MiscServicesCj {
class PasteDataImpl : public OHOS::FFI::FFIData {
public:
    PasteDataImpl();
    explicit PasteDataImpl(std::shared_ptr<MiscServices::PasteData> pasteData);
    PasteDataImpl(std::string mimeType, const CJValueType &value);
    std::shared_ptr<MiscServices::PasteData> GetRealPasteData();
    OHOS::FFI::RuntimeType *GetRuntimeType() override
    {
        return GetClassType();
    }

private:
    friend class OHOS::FFI::RuntimeType;
    friend class OHOS::FFI::TypeBase;
    static OHOS::FFI::RuntimeType *GetClassType();
    void CreateHtmlData(std::string mimeType, const CJValueType &value);
    void CreatePlainTextData(std::string mimeType, const CJValueType &value);
    void CreateUriData(std::string mimeType, const CJValueType &value);
    void CreatePixelMapData(std::string mimeType, const CJValueType &value);
    void CreateWantData(std::string mimeType, CJValueType value);

    std::shared_ptr<MiscServices::PasteData> value_ = nullptr;
};

int64_t CreateCjPasteDataObject(std::string mimeType, CJValueType value);

int32_t CreateCjMultiTypePasteDataObject(
    const std::map<std::string, std::shared_ptr<MiscServices::EntryValue>> &typeValueMap,
    const std::string &primaryMimeType,
    int64_t &outId);

} // namespace MiscServicesCj
} // namespace OHOS

#endif