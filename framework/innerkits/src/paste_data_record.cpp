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
#include "pasteboard_service_loader.h"

using namespace OHOS::Media;

namespace OHOS {
namespace MiscServices {

PasteDataRecord::PasteDataRecord(std::string mimeType, std::shared_ptr<std::string> htmlText,
    std::shared_ptr<OHOS::AAFwk::Want> want, std::shared_ptr<std::string> plainText, std::shared_ptr<OHOS::Uri> uri)
    : mimeType_{ std::move(mimeType) }, htmlText_{ std::move(htmlText) }, want_{ std::move(want) },
      plainText_{ std::move(plainText) }, uri_{ std::move(uri) }
{ // LCOV_EXCL_START
} // LCOV_EXCL_STOP

PasteDataRecord::PasteDataRecord()
{ // LCOV_EXCL_START
} // LCOV_EXCL_STOP

PasteDataRecord::~PasteDataRecord()
{ // LCOV_EXCL_START
    std::vector<std::shared_ptr<PasteDataEntry>>().swap(entries_);
} // LCOV_EXCL_STOP

PasteDataRecord::PasteDataRecord(const PasteDataRecord &record)
    : isDelay_(record.isDelay_), hasGrantUriPermission_(record.hasGrantUriPermission_), udType_(record.udType_),
      dataId_(record.dataId_), recordId_(record.recordId_), from_(record.from_), uriPermission_(record.uriPermission_),
      convertUri_(record.convertUri_), textContent_(record.textContent_), mimeType_(record.mimeType_),
      htmlText_(record.htmlText_), want_(record.want_), plainText_(record.plainText_), uri_(record.uri_),
      pixelMap_(record.pixelMap_), customData_(record.customData_), details_(record.details_),
      systemDefinedContents_(record.systemDefinedContents_), udmfValue_(record.udmfValue_), entries_(record.entries_),
      entryGetter_(record.entryGetter_)
{ // LCOV_EXCL_START
    this->isConvertUriFromRemote = record.isConvertUriFromRemote;
} // LCOV_EXCL_STOP

std::shared_ptr<std::string> PasteDataRecord::GetHtmlTextV0() const
{ // LCOV_EXCL_START
    for (const auto &entry : entries_) {
        if (entry && entry->GetMimeType() == MIMETYPE_TEXT_HTML) {
            return entry->ConvertToHtml();
        }
    }
    return htmlText_;
} // LCOV_EXCL_STOP

std::shared_ptr<std::string> PasteDataRecord::GetHtmlText() const
{ // LCOV_EXCL_START
    auto htmlText = GetHtmlTextV0();
    if (htmlText) {
        return htmlText;
    }
    auto entry = GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    if (entry == nullptr) {
        return htmlText_;
    }
    return entry->ConvertToHtml();
} // LCOV_EXCL_STOP

std::string PasteDataRecord::GetMimeType() const
{ // LCOV_EXCL_START
    if (!mimeType_.empty()) {
        return mimeType_;
    }
    if (!entries_.empty() && entries_.front() != nullptr) {
        return entries_.front()->GetMimeType();
    }
    return this->mimeType_;
} // LCOV_EXCL_STOP

std::shared_ptr<std::string> PasteDataRecord::GetPlainTextV0() const
{ // LCOV_EXCL_START
    for (const auto &entry : entries_) {
        if (entry && entry->GetMimeType() == MIMETYPE_TEXT_PLAIN) {
            return entry->ConvertToPlainText();
        }
    }
    return plainText_;
} // LCOV_EXCL_STOP

std::shared_ptr<std::string> PasteDataRecord::GetPlainText() const
{ // LCOV_EXCL_START
    auto plainText = GetPlainTextV0();
    if (plainText) {
        return plainText;
    }
    auto entry = GetEntryByMimeType(MIMETYPE_TEXT_PLAIN);
    if (entry == nullptr) {
        return plainText_;
    }
    return entry->ConvertToPlainText();
} // LCOV_EXCL_STOP

std::shared_ptr<PixelMap> PasteDataRecord::GetPixelMapV0() const
{ // LCOV_EXCL_START
    for (const auto &entry : entries_) {
        if (entry && entry->GetMimeType() == MIMETYPE_PIXELMAP) {
            return entry->ConvertToPixelMap();
        }
    }
    return pixelMap_;
} // LCOV_EXCL_STOP

std::shared_ptr<PixelMap> PasteDataRecord::GetPixelMap() const
{ // LCOV_EXCL_START
    auto pixelMap = GetPixelMapV0();
    if (pixelMap) {
        return pixelMap;
    }
    auto entry = GetEntryByMimeType(MIMETYPE_PIXELMAP);
    if (entry == nullptr) {
        return pixelMap_;
    }
    return entry->ConvertToPixelMap();
} // LCOV_EXCL_STOP

std::shared_ptr<OHOS::Uri> PasteDataRecord::GetUriV0() const
{ // LCOV_EXCL_START
    if (convertUri_.empty()) {
        return GetOriginUri();
    }
    return std::make_shared<OHOS::Uri>(convertUri_);
} // LCOV_EXCL_STOP

std::shared_ptr<OHOS::Uri> PasteDataRecord::GetUri() const
{ // LCOV_EXCL_START
    auto uri = GetUriV0();
    if (uri) {
        return uri;
    }
    auto entry = GetEntryByMimeType(MIMETYPE_TEXT_URI);
    if (entry == nullptr) {
        return GetUriV0();
    }
    return entry->ConvertToUri();
} // LCOV_EXCL_STOP

void PasteDataRecord::ClearPixelMap()
{ // LCOV_EXCL_START
    this->pixelMap_ = nullptr;
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
        [](const auto &entry) {
            return entry != nullptr && entry->GetMimeType() == MIMETYPE_PIXELMAP;
        }), entries_.end());
} // LCOV_EXCL_STOP

void PasteDataRecord::SetUri(std::shared_ptr<OHOS::Uri> uri)
{ // LCOV_EXCL_START
    if (uri == nullptr) {
        return;
    }

    for (auto &entry : entries_) {
        if (entry != nullptr && entry->GetMimeType() == MIMETYPE_TEXT_URI) {
            auto entryValue = entry->GetValue();
            if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
                auto object = std::get<std::shared_ptr<Object>>(entryValue);
                object->value_[UDMF::FILE_URI_PARAM] = uri->ToString();
            } else {
                entry->SetValue(uri->ToString());
            }
            return;
        }
    }

    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(uri->ToString());
    AddEntryByMimeType(MIMETYPE_TEXT_URI, entry);
} // LCOV_EXCL_STOP

std::shared_ptr<OHOS::Uri> PasteDataRecord::GetOriginUri() const
{ // LCOV_EXCL_START
    for (const auto &entry : entries_) {
        if (entry && entry->GetMimeType() == MIMETYPE_TEXT_URI) {
            return entry->ConvertToUri();
        }
    }
    return uri_;
} // LCOV_EXCL_STOP

std::shared_ptr<OHOS::AAFwk::Want> PasteDataRecord::GetWant() const
{ // LCOV_EXCL_START
    for (const auto &entry : entries_) {
        if (entry && entry->GetMimeType() == MIMETYPE_TEXT_WANT) {
            return entry->ConvertToWant();
        }
    }
    return want_;
} // LCOV_EXCL_STOP

std::shared_ptr<MineCustomData> PasteDataRecord::GetCustomData() const
{ // LCOV_EXCL_START
    std::shared_ptr<MineCustomData> customData = std::make_shared<MineCustomData>();
    if (customData_) {
        const std::map<std::string, std::vector<uint8_t>> &itemData = customData_->GetItemData();
        for (const auto &[key, value] : itemData) {
            customData->AddItemData(key, value);
        }
    }
    for (const auto &entry : entries_) {
        if (entry && entry->GetMimeType() == entry->GetUtdId()) {
            std::shared_ptr<MineCustomData> entryCustomData = entry->ConvertToCustomData();
            if (entryCustomData == nullptr) {
                continue;
            }
            const std::map<std::string, std::vector<uint8_t>> &itemData = entryCustomData->GetItemData();
            for (const auto &[key, value] : itemData) {
                customData->AddItemData(key, value);
            }
        }
    }
    return customData->GetItemData().empty() ? nullptr : customData;
} // LCOV_EXCL_STOP

std::string PasteDataRecord::ConvertToText() const
{ // LCOV_EXCL_START
    auto htmlText = GetHtmlTextV0();
    if (htmlText != nullptr) {
        return *htmlText;
    }
    auto plainText = GetPlainTextV0();
    if (plainText != nullptr) {
        return *plainText;
    }
    auto originUri = GetOriginUri();
    if (originUri != nullptr) {
        return originUri->ToString();
    }
    return "";
} // LCOV_EXCL_STOP

std::string PasteDataRecord::GetPassUri() const
{ // LCOV_EXCL_START
    std::string tempUri;
    if (uri_ != nullptr) {
        tempUri = uri_->ToString();
    }
    if (!convertUri_.empty()) {
        tempUri = convertUri_;
    }
    return tempUri;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetConvertUri(const std::string &value)
{ // LCOV_EXCL_START
    convertUri_ = value;
} // LCOV_EXCL_STOP

std::string PasteDataRecord::GetConvertUri() const
{ // LCOV_EXCL_START
    return convertUri_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetGrantUriPermission(bool hasPermission)
{ // LCOV_EXCL_START
    hasGrantUriPermission_ = hasPermission;
} // LCOV_EXCL_STOP

bool PasteDataRecord::HasGrantUriPermission()
{ // LCOV_EXCL_START
    return hasGrantUriPermission_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetUriPermission(uint32_t uriPermission)
{ // LCOV_EXCL_START
    uriPermission_ = uriPermission;
} // LCOV_EXCL_STOP

uint32_t PasteDataRecord::GetUriPermission()
{ // LCOV_EXCL_START
    return uriPermission_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetTextContent(const std::string &content)
{ // LCOV_EXCL_START
    this->textContent_ = content;
} // LCOV_EXCL_STOP

std::string PasteDataRecord::GetTextContent() const
{ // LCOV_EXCL_START
    return this->textContent_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetDetails(const Details &details)
{ // LCOV_EXCL_START
    this->details_ = std::make_shared<Details>(details);
} // LCOV_EXCL_STOP

std::shared_ptr<Details> PasteDataRecord::GetDetails() const
{ // LCOV_EXCL_START
    return this->details_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetSystemDefinedContent(const Details &contents)
{ // LCOV_EXCL_START
    this->systemDefinedContents_ = std::make_shared<Details>(contents);
} // LCOV_EXCL_STOP

std::shared_ptr<Details> PasteDataRecord::GetSystemDefinedContent() const
{ // LCOV_EXCL_START
    return this->systemDefinedContents_;
} // LCOV_EXCL_STOP

int32_t PasteDataRecord::GetUDType() const
{ // LCOV_EXCL_START
    return this->udType_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetUDType(int32_t type)
{ // LCOV_EXCL_START
    this->udType_ = type;
} // LCOV_EXCL_STOP

std::vector<std::string> PasteDataRecord::GetValidMimeTypes(const std::vector<std::string> &mimeTypes) const
{ // LCOV_EXCL_START
    std::vector<std::string> res;
    auto allTypes = GetMimeTypes();
    for (auto const& type : mimeTypes) {
        if (allTypes.find(type) != allTypes.end()) {
            res.emplace_back(type);
        }
    }
    return res;
} // LCOV_EXCL_STOP

std::vector<std::string> PasteDataRecord::GetValidTypes(const std::vector<std::string> &types) const
{ // LCOV_EXCL_START
    std::vector<std::string> res;
    auto allTypes = GetUtdTypes();
    for (auto const &type : types) {
        if (allTypes.find(type) != allTypes.end()) {
            res.emplace_back(type);
        }
    }
    return res;
} // LCOV_EXCL_STOP

bool PasteDataRecord::HasEmptyEntry() const
{ // LCOV_EXCL_START
    for (auto const &entry : GetEntries()) {
        if (entry == nullptr) {
            continue;
        }
        if (std::holds_alternative<std::monostate>(entry->GetValue())) {
            return true;
        }
    }
    return false;
} // LCOV_EXCL_STOP

uint32_t PasteDataRecord::RemoveEmptyEntry()
{ // LCOV_EXCL_START
    uint32_t removeCnt = 0;
    for (auto iter = entries_.begin(); iter != entries_.end();) {
        auto entry = *iter;
        if (entry == nullptr || std::holds_alternative<std::monostate>(entry->GetValue())) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "recordId=%{public}u, type=%{public}s",
                GetRecordId(), entry ? entry->GetUtdId().c_str() : "null");
            iter = entries_.erase(iter);
            ++removeCnt;
        } else {
            ++iter;
        }
    }
    return removeCnt;
} // LCOV_EXCL_STOP

std::set<std::string> PasteDataRecord::GetUtdTypes() const
{ // LCOV_EXCL_START
    std::set<std::string> types;
    if (!mimeType_.empty()) {
        types.emplace(CommonUtils::Convert2UtdId(udType_, mimeType_));
    }
    for (auto const &entry : entries_) {
        if (entry == nullptr) {
            continue;
        }
        types.emplace(entry->GetUtdId());
    }
    return types;
} // LCOV_EXCL_STOP

std::set<std::string> PasteDataRecord::GetMimeTypes() const
{ // LCOV_EXCL_START
    std::set<std::string> types;
    if (!mimeType_.empty()) {
        types.emplace(mimeType_);
    }
    for (auto const& entry: entries_) {
        if (entry == nullptr) {
            continue;
        }
        types.emplace(entry->GetMimeType());
    }
    return types;
} // LCOV_EXCL_STOP

void PasteDataRecord::AddEntryByMimeType(const std::string &mimeType, std::shared_ptr<PasteDataEntry> value)
{ // LCOV_EXCL_START
    PASTEBOARD_CHECK_AND_RETURN_LOGE(value != nullptr, PASTEBOARD_MODULE_CLIENT, "value is null");
    auto utdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, mimeType);
    value->SetUtdId(utdId);
    value->SetMimeType(mimeType);
    AddEntry(utdId, value);
} // LCOV_EXCL_STOP

void PasteDataRecord::AddEntry(const std::string &utdType, std::shared_ptr<PasteDataEntry> value)
{ // LCOV_EXCL_START
    PASTEBOARD_CHECK_AND_RETURN_LOGE(value != nullptr, PASTEBOARD_MODULE_CLIENT, "Entry value is null");
    if (utdType != value->GetUtdId()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CLIENT, "Type is diff, UtdType:%{public}s, UtdId:%{public}s",
            utdType.c_str(), value->GetUtdId().c_str());
        return;
    }

    bool has = false;
    for (auto &entry : entries_) {
        if (entry == nullptr) {
            continue;
        }
        if (entry->GetUtdId() == utdType ||
            (entry->GetMimeType() == MIMETYPE_TEXT_URI && value->GetMimeType() == MIMETYPE_TEXT_URI)) {
            entry = value;
            has = true;
            break;
        }
    }

    PASTEBOARD_CHECK_AND_RETURN_LOGD(!has, PASTEBOARD_MODULE_COMMON, "replace entry, type=%{public}s", utdType.c_str());
    if (entries_.empty()) {
        auto udType = UDMF::UtdUtils::GetUtdEnumFromUtdId(utdType);
        udType_ = udType == UDMF::UDType::UD_BUTT ? UDMF::UDType::APPLICATION_DEFINED_RECORD : udType;
    }
    entries_.emplace_back(value);
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataEntry> PasteDataRecord::GetEntryByMimeType(const std::string &mimeType) const
{ // LCOV_EXCL_START
    auto utdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, mimeType);
    std::shared_ptr<PasteDataEntry> entry = GetEntry(utdId);
    if (entry == nullptr && customData_ != nullptr) {
        const std::map<std::string, std::vector<uint8_t>> &itemData = customData_->GetItemData();
        for (const auto &[key, value] : itemData) {
            if (mimeType == key) {
                entry = std::make_shared<PasteDataEntry>(utdId, mimeType, value);
                return entry;
            }
        }
    }
    if (entry == nullptr && mimeType == MIMETYPE_TEXT_PLAIN) {
        utdId = CommonUtils::Convert2UtdId(UDMF::UDType::HYPERLINK, mimeType);
        entry = GetEntry(utdId);
    }
    return entry;
} // LCOV_EXCL_STOP

std::shared_ptr<PasteDataEntry> PasteDataRecord::GetEntry(const std::string &utdType) const
{ // LCOV_EXCL_START
    for (auto const &entry : entries_) {
        if (entry == nullptr) {
            continue;
        }
        if (entry->GetUtdId() == utdType ||
            (CommonUtils::IsFileUri(utdType) && CommonUtils::IsFileUri(entry->GetUtdId()))) {
            if (isDelay_ && !entry->HasContent(utdType) && !PasteBoardCommon::IsPasteboardService()) {
                PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "get delay entry value, dataId=%{public}u, "
                    "recordId=%{public}u, type=%{public}s", dataId_, recordId_, utdType.c_str());
                PasteboardServiceLoader::GetInstance().GetRecordValueByType(dataId_, recordId_, *entry);
            }
            if (CommonUtils::IsFileUri(utdType) && GetUriV0() != nullptr) {
                return std::make_shared<PasteDataEntry>(utdType, GetUriV0()->ToString());
            }
            return entry;
        }
    }
    return nullptr;
} // LCOV_EXCL_STOP

std::vector<std::shared_ptr<PasteDataEntry>> PasteDataRecord::GetEntries() const
{ // LCOV_EXCL_START
    return entries_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetDataId(uint32_t dataId)
{ // LCOV_EXCL_START
    dataId_ = dataId;
} // LCOV_EXCL_STOP

uint32_t PasteDataRecord::GetDataId() const
{ // LCOV_EXCL_START
    return dataId_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetRecordId(uint32_t recordId)
{ // LCOV_EXCL_START
    recordId_ = recordId;
} // LCOV_EXCL_STOP

uint32_t PasteDataRecord::GetRecordId() const
{ // LCOV_EXCL_START
    return recordId_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetDelayRecordFlag(bool isDelay)
{ // LCOV_EXCL_START
    isDelay_ = isDelay;
} // LCOV_EXCL_STOP

bool PasteDataRecord::IsDelayRecord() const
{ // LCOV_EXCL_START
    return isDelay_;
} // LCOV_EXCL_STOP

void PasteDataRecord::SetEntryGetter(const std::shared_ptr<UDMF::EntryGetter> entryGetter)
{ // LCOV_EXCL_START
    entryGetter_ = std::move(entryGetter);
} // LCOV_EXCL_STOP

void PasteDataRecord::SetFrom(uint32_t from)
{ // LCOV_EXCL_START
    from_ = from;
} // LCOV_EXCL_STOP

uint32_t PasteDataRecord::GetFrom() const
{ // LCOV_EXCL_START
    return from_;
} // LCOV_EXCL_STOP

std::shared_ptr<UDMF::EntryGetter> PasteDataRecord::GetEntryGetter()
{ // LCOV_EXCL_START
    return entryGetter_;
} // LCOV_EXCL_STOP
} // namespace MiscServices
} // namespace OHOS
