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

#include "paste_data_record_impl.h"

using namespace OHOS::MiscServices;

namespace OHOS {
namespace MiscServicesCj {

static std::map<std::shared_ptr<PasteDataRecord>, sptr<PasteDataRecordImpl>> g_cjPasteDataRecordMap;
std::recursive_mutex g_PasteDataMutex;

int64_t CreateCjPasteDataRecordObject(std::string mimeType, CJValueType value)
{
    auto pasteDataRecordImpl = FFI::FFIData::Create<PasteDataRecordImpl>(mimeType, value);
    if (pasteDataRecordImpl == nullptr) {
        return 0;
    }
    std::shared_ptr<MiscServices::PasteDataRecord> realValue = pasteDataRecordImpl->GetRealPasteDataRecord();
    std::lock_guard<std::recursive_mutex> lock(g_PasteDataMutex);
    g_cjPasteDataRecordMap.try_emplace(realValue, pasteDataRecordImpl);

    return pasteDataRecordImpl->GetID();
}

sptr<PasteDataRecordImpl> getCjPasteDataRecordImpl(std::shared_ptr<PasteDataRecord> record)
{
    if (record == nullptr) {
        return nullptr;
    }
    std::lock_guard<std::recursive_mutex> lock(g_PasteDataMutex);
    if (g_cjPasteDataRecordMap.find(record) == g_cjPasteDataRecordMap.end()) {
        return nullptr;
    }
    return g_cjPasteDataRecordMap[record];
}

void removeCjPasteDataRecordImpl(std::shared_ptr<MiscServices::PasteDataRecord> record)
{
    std::lock_guard<std::recursive_mutex> lock(g_PasteDataMutex);
    g_cjPasteDataRecordMap.erase(record);
}

void addCjPasteDataRecordImpl(
    std::shared_ptr<MiscServices::PasteDataRecord> record, sptr<PasteDataRecordImpl> pasteDataRecordImpl)
{
    std::lock_guard<std::recursive_mutex> lock(g_PasteDataMutex);
    g_cjPasteDataRecordMap.try_emplace(record, pasteDataRecordImpl);
}

PasteDataRecordImpl::PasteDataRecordImpl()
{
    value_ = std::make_shared<PasteDataRecord>();
}

PasteDataRecordImpl::PasteDataRecordImpl(std::shared_ptr<MiscServices::PasteDataRecord> pasteDataRecord)
{
    value_ = pasteDataRecord;
}

PasteDataRecordImpl::PasteDataRecordImpl(std::string mimeType, CJValueType value)
{
    if (mimeType == "text/html") {
        CreateHtmlDataRecord(mimeType, value);
    } else if (mimeType == "text/plain") {
        CreatePlainTextDataRecord(mimeType, value);
    } else if (mimeType == "text/uri") {
        CreateUriDataRecord(mimeType, value);
    } else if (mimeType == "pixelMap") {
        CreatePixelMapDataRecord(mimeType, value);
    } else if (mimeType == "text/want") {
        CreateWantDataRecord(mimeType, value);
    } else {
        std::vector<uint8_t> arrayBuf(reinterpret_cast<uint8_t *>(value.arrayBufferData),
            reinterpret_cast<uint8_t *>(value.arrayBufferData) + value.arrayBufferSize);
        value_ = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuf);
    }
}

std::shared_ptr<MiscServices::PasteDataRecord> PasteDataRecordImpl::GetRealPasteDataRecord()
{
    if (value_ == nullptr) {
        return nullptr;
    }
    std::shared_ptr<MiscServices::PasteDataRecord> res = value_;
    return res;
}

void PasteDataRecordImpl::CreateHtmlDataRecord(std::string mimeType, CJValueType value)
{
    value_ = PasteboardClient::GetInstance()->CreateHtmlTextRecord(value.stringValue);
}

void PasteDataRecordImpl::CreatePlainTextDataRecord(std::string mimeType, CJValueType value)
{
    value_ = PasteboardClient::GetInstance()->CreatePlainTextRecord(value.stringValue);
}

void PasteDataRecordImpl::CreateUriDataRecord(std::string mimeType, CJValueType value)
{
    value_ = PasteboardClient::GetInstance()->CreateUriRecord(OHOS::Uri(value.stringValue));
}

void PasteDataRecordImpl::CreatePixelMapDataRecord(std::string mimeType, CJValueType value)
{
    value_ = PasteboardClient::GetInstance()->CreatePixelMapRecord(value.pixelMap);
}

void PasteDataRecordImpl::CreateWantDataRecord(std::string mimeType, CJValueType value) {}

} // namespace MiscServicesCj
} // namespace OHOS