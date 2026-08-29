/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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

#include "paste_data_record.h"

#include "pasteboard_common.h"
#include "pasteboard_hilog.h"

using namespace OHOS::Media;

namespace OHOS {
namespace MiscServices {
constexpr int32_t MAX_TEXT_LEN = 100 * 1024 * 1024;

PasteDataRecord::Builder &PasteDataRecord::Builder::SetMimeType(std::string mimeType)
{ // LCOV_EXCL_START
    record_->mimeType_ = std::move(mimeType);
    return *this;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder &PasteDataRecord::Builder::SetHtmlText(std::shared_ptr<std::string> htmlText)
{ // LCOV_EXCL_START
    if (htmlText == nullptr) {
        return *this;
    }
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(*htmlText);
    record_->AddEntryByMimeType(MIMETYPE_TEXT_HTML, entry);
    return *this;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder &PasteDataRecord::Builder::SetWant(std::shared_ptr<OHOS::AAFwk::Want> want)
{ // LCOV_EXCL_START
    if (want == nullptr) {
        return *this;
    }
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(std::move(want));
    record_->AddEntryByMimeType(MIMETYPE_TEXT_WANT, entry);
    return *this;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder &PasteDataRecord::Builder::SetPlainText(std::shared_ptr<std::string> plainText)
{ // LCOV_EXCL_START
    if (plainText == nullptr) {
        return *this;
    }
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(*plainText);
    record_->AddEntryByMimeType(MIMETYPE_TEXT_PLAIN, entry);
    return *this;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder &PasteDataRecord::Builder::SetUri(std::shared_ptr<OHOS::Uri> uri)
{ // LCOV_EXCL_START
    if (uri == nullptr) {
        return *this;
    }
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(uri->ToString());
    record_->AddEntryByMimeType(MIMETYPE_TEXT_URI, entry);
    return *this;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder &PasteDataRecord::Builder::SetPixelMap(std::shared_ptr<OHOS::Media::PixelMap> pixelMap)
{ // LCOV_EXCL_START
    if (pixelMap == nullptr) {
        return *this;
    }
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(std::move(pixelMap));
    record_->AddEntryByMimeType(MIMETYPE_PIXELMAP, entry);
    return *this;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder &PasteDataRecord::Builder::SetCustomData(std::shared_ptr<MineCustomData> customData)
{ // LCOV_EXCL_START
    record_->customData_ = std::move(customData);
    return *this;
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::Builder::Build()
{ // LCOV_EXCL_START
    if (record_->mimeType_.empty()) {
        return record_;
    }
    auto entries = record_->GetEntries();
    auto record = std::make_shared<PasteDataRecord>();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (record_->mimeType_ == entries[i]->GetMimeType()) {
            record->AddEntry(entries[i]->GetUtdId(), entries[i]);
        }
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        if (record_->mimeType_ != entries[i]->GetMimeType()) {
            record->AddEntry(entries[i]->GetUtdId(), entries[i]);
        }
    }
    record->customData_ = std::move(record_->customData_);
    record->mimeType_ = record_->mimeType_;
    return record;
} // LCOV_EXCL_STOP

PasteDataRecord::Builder::Builder(const std::string &mimeType)
{ // LCOV_EXCL_START
    record_ = std::make_shared<PasteDataRecord>();
    if (record_ != nullptr) {
        record_->mimeType_ = mimeType;
        record_->htmlText_ = nullptr;
        record_->want_ = nullptr;
        record_->plainText_ = nullptr;
        record_->uri_ = nullptr;
        record_->convertUri_ = "";
        record_->pixelMap_ = nullptr;
        record_->customData_ = nullptr;
    }
} // LCOV_EXCL_STOP

void PasteDataRecord::AddUriEntry()
{ // LCOV_EXCL_START
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    if (uri_ != nullptr) {
        object->value_[UDMF::FILE_URI_PARAM] = uri_->ToString();
    }
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewHtmlRecord(const std::string &htmlText)
{ // LCOV_EXCL_START
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((htmlText.length() < MAX_TEXT_LEN), nullptr,
        PASTEBOARD_MODULE_CLIENT, "record length not support, length=%{public}zu", htmlText.length());
    return Builder(MIMETYPE_TEXT_HTML).SetHtmlText(std::make_shared<std::string>(htmlText)).Build();
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewWantRecord(std::shared_ptr<OHOS::AAFwk::Want> want)
{ // LCOV_EXCL_START
    return Builder(MIMETYPE_TEXT_WANT).SetWant(std::move(want)).Build();
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewPlainTextRecord(const std::string &text)
{ // LCOV_EXCL_START
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE((text.length() < MAX_TEXT_LEN), nullptr,
        PASTEBOARD_MODULE_CLIENT, "PlainText length not support, length=%{public}zu", text.length());
    return Builder(MIMETYPE_TEXT_PLAIN).SetPlainText(std::make_shared<std::string>(text)).Build();
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewPixelMapRecord(std::shared_ptr<PixelMap> pixelMap)
{ // LCOV_EXCL_START
    return Builder(MIMETYPE_PIXELMAP).SetPixelMap(std::move(pixelMap)).Build();
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewUriRecord(const OHOS::Uri &uri)
{ // LCOV_EXCL_START
    return Builder(MIMETYPE_TEXT_URI).SetUri(std::make_shared<OHOS::Uri>(uri)).Build();
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewKvRecord(
    const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer)
{ // LCOV_EXCL_START
    std::shared_ptr<MineCustomData> customData = std::make_shared<MineCustomData>();
    customData->AddItemData(mimeType, arrayBuffer);
    return Builder(mimeType).SetCustomData(std::move(customData)).Build();
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewMultiTypeRecord(
    std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>> values, const std::string &recordMimeType)
{ // LCOV_EXCL_START
    auto record = std::make_shared<PasteDataRecord>();
    if (values == nullptr) {
        return record;
    }
    if (!recordMimeType.empty()) {
        auto recordDefaultIter = values->find(recordMimeType);
        if (recordDefaultIter != values->end() && recordDefaultIter->second != nullptr) {
            auto utdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, recordMimeType);
            record->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, *(recordDefaultIter->second)));
        }
        record->mimeType_ = recordMimeType;
    }
    for (auto [mimeType, value] : *values) {
        if (mimeType == recordMimeType) {
            continue;
        }
        auto utdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, mimeType);
        record->AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, *value));
    }
    return record;
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataRecord> PasteDataRecord::NewMultiTypeDelayRecord(
    std::vector<std::string> mimeTypes, const std::shared_ptr<UDMF::EntryGetter> entryGetter)
{ // LCOV_EXCL_START
    auto record = std::make_shared<PasteDataRecord>();
    for (auto mimeType : mimeTypes) {
        auto utdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, mimeType);
        auto entry = std::make_shared<PasteDataEntry>();
        entry->SetMimeType(mimeType);
        entry->SetUtdId(utdId);
        record->AddEntry(utdId, entry);
    }
    if (entryGetter != nullptr) {
        record->SetEntryGetter(entryGetter);
        record->SetDelayRecordFlag(true);
    }
    return record;
} // LCOV_EXCL_STOP
} // namespace MiscServices
} // namespace OHOS
