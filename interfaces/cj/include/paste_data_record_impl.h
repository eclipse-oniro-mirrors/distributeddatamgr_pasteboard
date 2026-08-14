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

#ifndef PASTE_DATA_RECORD_IMPL_H
#define PASTE_DATA_RECORD_IMPL_H

#include "pasteboard_client.h"
#include "pixel_map_impl.h"

namespace OHOS {
namespace MiscServicesCj {
struct CJValueType {
    std::string stringValue;
    AAFwk::Want wantValue;
    std::shared_ptr<Media::PixelMap> pixelMap = nullptr;
    uint8_t *arrayBufferData;
    int64_t arrayBufferSize;
};

class PasteDataRecordImpl : public OHOS::FFI::FFIData {
    DECL_TYPE(PasteDataRecordImpl, OHOS::FFI::FFIData)
public:
    PasteDataRecordImpl();
    explicit PasteDataRecordImpl(std::shared_ptr<MiscServices::PasteDataRecord> pasteDataRecord);
    PasteDataRecordImpl(std::string mimeType, CJValueType value);
    std::shared_ptr<MiscServices::PasteDataRecord> GetRealPasteDataRecord();

private:
    void CreateHtmlDataRecord(std::string mimeType, CJValueType value);
    void CreatePlainTextDataRecord(std::string mimeType, CJValueType value);
    void CreateUriDataRecord(std::string mimeType, CJValueType value);
    void CreatePixelMapDataRecord(std::string mimeType, CJValueType value);
    void CreateWantDataRecord(std::string mimeType, CJValueType value);

    std::shared_ptr<MiscServices::PasteDataRecord> value_ = nullptr;
};

int64_t CreateCjPasteDataRecordObject(std::string mimeType, CJValueType value);
sptr<PasteDataRecordImpl> getCjPasteDataRecordImpl(std::shared_ptr<MiscServices::PasteDataRecord> record);
void removeCjPasteDataRecordImpl(std::shared_ptr<MiscServices::PasteDataRecord> record);
void addCjPasteDataRecordImpl(
    std::shared_ptr<MiscServices::PasteDataRecord> record, sptr<PasteDataRecordImpl> pasteDataRecordImpl);

} // namespace MiscServicesCj
} // namespace OHOS

#endif