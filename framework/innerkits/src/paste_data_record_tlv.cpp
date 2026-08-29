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
enum TAG_PASTEBOARD_RECORD : uint16_t {
    TAG_MIMETYPE = TAG_BUFF + 1,
    TAG_HTMLTEXT,
    TAG_WANT,
    TAG_PLAINTEXT,
    TAG_URI,
    TAG_PIXELMAP,
    TAG_CUSTOM_DATA,
    TAG_CONVERT_URI,
    TAG_URI_PERMISSION,
    TAG_UDC_UDTYPE,
    TAG_UDC_DETAILS,
    TAG_UDC_TEXTCONTENT,
    TAG_UDC_SYSTEMCONTENTS,
    TAG_UDC_UDMFVALUE,
    TAG_UDC_ENTRIES,
    TAG_DATA_ID,
    TAG_RECORD_ID,
    TAG_DELAY_RECORD_FLAG,
    TAG_FROM,
};

bool PasteDataRecord::EncodeTLVLocal(WriteOnlyBuffer &buffer) const
{
    bool ret = buffer.Write(TAG_MIMETYPE, mimeType_);
    ret = ret && buffer.Write(TAG_HTMLTEXT, htmlText_);
    ret = ret && buffer.Write(TAG_WANT, TLVUtils::Parcelable2Raw(want_.get()));
    ret = ret && buffer.Write(TAG_PLAINTEXT, plainText_);
    ret = ret && buffer.Write(TAG_URI, TLVUtils::Parcelable2Raw(uri_.get()));
    ret = ret && buffer.Write(TAG_CONVERT_URI, convertUri_);
    ret = ret && buffer.Write(TAG_PIXELMAP, pixelMap_);
    ret = ret && buffer.Write(TAG_CUSTOM_DATA, customData_);
    ret = ret && buffer.Write(TAG_URI_PERMISSION, hasGrantUriPermission_);
    ret = ret && buffer.Write(TAG_UDC_UDTYPE, udType_);
    ret = ret && buffer.Write(TAG_UDC_DETAILS, details_);
    ret = ret && buffer.Write(TAG_UDC_TEXTCONTENT, textContent_);
    ret = ret && buffer.Write(TAG_UDC_SYSTEMCONTENTS, systemDefinedContents_);
    ret = ret && buffer.Write(TAG_UDC_UDMFVALUE, udmfValue_);
    ret = ret && buffer.Write(TAG_UDC_ENTRIES, entries_);
    ret = ret && buffer.Write(TAG_DATA_ID, dataId_);
    ret = ret && buffer.Write(TAG_RECORD_ID, recordId_);
    ret = ret && buffer.Write(TAG_DELAY_RECORD_FLAG, isDelay_);
    ret = ret && buffer.Write(TAG_FROM, from_);
    return ret;
}

bool PasteDataRecord::EncodeTLVRemote(WriteOnlyBuffer &buffer) const
{
    bool ret = true;

    auto remoteValue = Local2Remote();
    if (remoteValue != nullptr) {
        ret = ret && buffer.Write(TAG_MIMETYPE, remoteValue->mimeType_);
        ret = ret && buffer.Write(TAG_UDC_UDTYPE, remoteValue->udType_);
        ret = ret && buffer.Write(TAG_HTMLTEXT, remoteValue->htmlText_);
        ret = ret && buffer.Write(TAG_PLAINTEXT, remoteValue->plainText_);
        ret = ret && buffer.Write(TAG_PIXELMAP, remoteValue->pixelMap_);
        ret = ret && buffer.Write(TAG_WANT, TLVUtils::Parcelable2Raw(remoteValue->want_.get()));
        ret = ret && buffer.Write(TAG_URI, TLVUtils::Parcelable2Raw(remoteValue->uri_.get()));
        ret = ret && buffer.Write(TAG_UDC_UDMFVALUE, remoteValue->udmfValue_);
        ret = ret && buffer.Write(TAG_UDC_ENTRIES, remoteValue->entries_);
    }

    ret = ret && buffer.Write(TAG_CONVERT_URI, convertUri_);
    ret = ret && buffer.Write(TAG_CUSTOM_DATA, customData_);
    ret = ret && buffer.Write(TAG_URI_PERMISSION, hasGrantUriPermission_);
    ret = ret && buffer.Write(TAG_UDC_DETAILS, details_);
    ret = ret && buffer.Write(TAG_UDC_TEXTCONTENT, textContent_);
    ret = ret && buffer.Write(TAG_UDC_SYSTEMCONTENTS, systemDefinedContents_);
    ret = ret && buffer.Write(TAG_DATA_ID, dataId_);
    ret = ret && buffer.Write(TAG_RECORD_ID, recordId_);
    ret = ret && buffer.Write(TAG_DELAY_RECORD_FLAG, isDelay_);
    ret = ret && buffer.Write(TAG_FROM, from_);
    return ret;
}

bool PasteDataRecord::EncodeTLV(WriteOnlyBuffer &buffer) const
{
    return IsRemoteEncode() ? EncodeTLVRemote(buffer) : EncodeTLVLocal(buffer);
}

bool PasteDataRecord::DecodeItem1(uint16_t tag, ReadOnlyBuffer &buffer, TLVHead &head)
{
    switch (tag) {
        case TAG_MIMETYPE:
            return buffer.ReadValue(mimeType_, head);
        case TAG_HTMLTEXT:
            return buffer.ReadValue(htmlText_, head);
        case TAG_WANT:
            return buffer.ReadValue(want_, head);
        case TAG_PLAINTEXT:
            return buffer.ReadValue(plainText_, head);
        case TAG_URI:
            return buffer.ReadValue(uri_, head);
        case TAG_CONVERT_URI:
            return buffer.ReadValue(convertUri_, head);
        case TAG_PIXELMAP:
            return buffer.ReadValue(pixelMap_, head);
        case TAG_CUSTOM_DATA:
            return buffer.ReadValue(customData_, head);
        case TAG_URI_PERMISSION:
            return buffer.ReadValue(hasGrantUriPermission_, head);
        default:
            return DecodeItem2(tag, buffer, head);
    }
}

bool PasteDataRecord::DecodeItem2(uint16_t tag, ReadOnlyBuffer &buffer, TLVHead &head)
{
    switch (tag) {
        case TAG_UDC_UDTYPE:
            return buffer.ReadValue(udType_, head);
        case TAG_UDC_DETAILS:
            return buffer.ReadValue(details_, head);
        case TAG_UDC_TEXTCONTENT:
            return buffer.ReadValue(textContent_, head);
        case TAG_UDC_SYSTEMCONTENTS:
            return buffer.ReadValue(systemDefinedContents_, head);
        case TAG_UDC_UDMFVALUE:
            return buffer.ReadValue(udmfValue_, head);
        case TAG_UDC_ENTRIES:
            return buffer.ReadValue(entries_, head);
        case TAG_DATA_ID:
            return buffer.ReadValue(dataId_, head);
        case TAG_RECORD_ID:
            return buffer.ReadValue(recordId_, head);
        case TAG_DELAY_RECORD_FLAG:
            return buffer.ReadValue(isDelay_, head);
        case TAG_FROM:
            return buffer.ReadValue(from_, head);
        default:
            return buffer.Skip(head.len);
    }
}

bool PasteDataRecord::DecodeTLV(ReadOnlyBuffer &buffer)
{
    for (; buffer.IsEnough();) {
        TLVHead head{};
        bool ret = buffer.ReadHead(head);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, false, PASTEBOARD_MODULE_COMMON, "read head failed");
        ret = DecodeItem1(head.tag, buffer, head);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret, false, PASTEBOARD_MODULE_COMMON,
            "read value failed, tag=%{public}hu, len=%{public}u", head.tag, head.len);
    }

    auto entry = Remote2Local();
    if (entry != nullptr) {
        entries_.insert(entries_.begin(), std::move(entry));
    }
    udmfValue_ = nullptr;
    plainText_ = nullptr;
    htmlText_ = nullptr;
    uri_ = nullptr;
    pixelMap_ = nullptr;
    want_ = nullptr;
    if (mimeType_.empty()) {
        mimeType_ = GetMimeType();
    }
    return true;
}

size_t PasteDataRecord::CountTLVLocal() const
{
    size_t expectedSize = 0;
    expectedSize += TLVCountable::Count(mimeType_);
    expectedSize += TLVCountable::Count(htmlText_);
    expectedSize += TLVCountable::Count(TLVUtils::Parcelable2Raw(want_.get()));
    expectedSize += TLVCountable::Count(plainText_);
    expectedSize += TLVCountable::Count(TLVUtils::Parcelable2Raw(uri_.get()));
    expectedSize += TLVCountable::Count(convertUri_);
    expectedSize += TLVCountable::Count(pixelMap_);
    expectedSize += TLVCountable::Count(customData_);
    expectedSize += TLVCountable::Count(hasGrantUriPermission_);
    expectedSize += TLVCountable::Count(udType_);
    expectedSize += TLVCountable::Count(details_);
    expectedSize += TLVCountable::Count(textContent_);
    expectedSize += TLVCountable::Count(systemDefinedContents_);
    expectedSize += TLVCountable::Count(udmfValue_);
    expectedSize += TLVCountable::Count(entries_);
    expectedSize += TLVCountable::Count(dataId_);
    expectedSize += TLVCountable::Count(recordId_);
    expectedSize += TLVCountable::Count(isDelay_);
    expectedSize += TLVCountable::Count(from_);
    return expectedSize;
}

size_t PasteDataRecord::CountTLVRemote() const
{
    size_t expectedSize = 0;
    auto remoteValue = Local2Remote();
    if (remoteValue != nullptr) {
        expectedSize += TLVCountable::Count(remoteValue->mimeType_);
        expectedSize += TLVCountable::Count(remoteValue->udType_);
        expectedSize += TLVCountable::Count(remoteValue->htmlText_);
        expectedSize += TLVCountable::Count(remoteValue->plainText_);
        expectedSize += TLVCountable::Count(remoteValue->pixelMap_);
        expectedSize += TLVCountable::Count(TLVUtils::Parcelable2Raw(remoteValue->want_.get()));
        expectedSize += TLVCountable::Count(TLVUtils::Parcelable2Raw(remoteValue->uri_.get()));
        expectedSize += TLVCountable::Count(remoteValue->udmfValue_);
        expectedSize += TLVCountable::Count(remoteValue->entries_);
    }

    expectedSize += TLVCountable::Count(convertUri_);
    expectedSize += TLVCountable::Count(customData_);
    expectedSize += TLVCountable::Count(hasGrantUriPermission_);
    expectedSize += TLVCountable::Count(details_);
    expectedSize += TLVCountable::Count(textContent_);
    expectedSize += TLVCountable::Count(systemDefinedContents_);
    expectedSize += TLVCountable::Count(dataId_);
    expectedSize += TLVCountable::Count(recordId_);
    expectedSize += TLVCountable::Count(isDelay_);
    expectedSize += TLVCountable::Count(from_);
    return expectedSize;
}

size_t PasteDataRecord::CountTLV() const
{
    return IsRemoteEncode() ? CountTLVRemote() : CountTLVLocal();
}

std::shared_ptr<PasteDataEntry> PasteDataRecord::Remote2Local() const
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(!mimeType_.empty(), nullptr, PASTEBOARD_MODULE_COMMON, "mimeType empty");

    auto entry = std::make_shared<PasteDataEntry>();
    auto utdId = CommonUtils::Convert2UtdId(udType_, mimeType_);
    entry->SetUtdId(utdId);
    entry->SetMimeType(mimeType_);

    if (udmfValue_ != nullptr) {
        if (std::holds_alternative<std::shared_ptr<Object>>(*udmfValue_)) {
            auto object = std::get<std::shared_ptr<Object>>(*udmfValue_);
            if (object != nullptr && !object->value_.empty()) {
                entry->SetValue(object);
                return entry;
            }
        } else if (std::holds_alternative<std::vector<uint8_t>>(*udmfValue_)) {
            auto array = std::get<std::vector<uint8_t>>(*udmfValue_);
            entry->SetValue(array);
            return entry;
        }
    }

    auto object = std::make_shared<Object>();
    if (mimeType_ == MIMETYPE_TEXT_PLAIN && plainText_ != nullptr) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
        object->value_[UDMF::CONTENT] = *plainText_;
    } else if (mimeType_ == MIMETYPE_TEXT_HTML && htmlText_ != nullptr) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
        object->value_[UDMF::HTML_CONTENT] = *htmlText_;
        if (plainText_ != nullptr) {
            object->value_[UDMF::PLAIN_CONTENT] = *plainText_;
        }
    } else if (mimeType_ == MIMETYPE_TEXT_URI && uri_ != nullptr) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
        object->value_[UDMF::FILE_URI_PARAM] = uri_->ToString();
    } else if (mimeType_ == MIMETYPE_PIXELMAP && pixelMap_ != nullptr) {
        object->value_[UDMF::UNIFORM_DATA_TYPE] = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
        object->value_[UDMF::PIXEL_MAP] = std::move(pixelMap_);
    } else if (mimeType_ == MIMETYPE_TEXT_WANT && want_ != nullptr) {
        entry->SetValue(std::move(want_));
        return entry;
    } else {
        return nullptr;
    }

    entry->SetValue(object);
    return entry;
}

std::shared_ptr<RemoteRecordValue> PasteDataRecord::Local2Remote() const
{
    auto value = std::make_shared<RemoteRecordValue>();
    value->mimeType_ = mimeType_;
    if (entries_.empty()) {
        return value;
    }

    auto firstEntry = entries_.front();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(firstEntry != nullptr, value, PASTEBOARD_MODULE_COMMON, "firstEntry is null");

    auto entryValue = firstEntry->GetValue();
    std::string mimeType = firstEntry->GetMimeType();
    std::string utdId = firstEntry->GetUtdId();
    value->udType_ = UDMF::UtdUtils::GetUtdEnumFromUtdId(utdId);
    value->mimeType_ = mimeType;

    if (mimeType == MIMETYPE_TEXT_PLAIN) {
        value->plainText_ = firstEntry->ConvertToPlainText();
    } else if (mimeType == MIMETYPE_TEXT_WANT) {
        value->want_ = firstEntry->ConvertToWant();
    } else if (mimeType == MIMETYPE_TEXT_URI) {
        value->uri_ = firstEntry->ConvertToUri();
    } else if (mimeType == MIMETYPE_PIXELMAP) {
        value->pixelMap_ = firstEntry->ConvertToPixelMap();
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        value->htmlText_ = firstEntry->ConvertToHtml();
        if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
            auto object = std::get<std::shared_ptr<Object>>(entryValue);
            std::string plainText;
            object->GetValue(UDMF::PLAIN_CONTENT, plainText);
            if (!plainText.empty()) {
                value->plainText_ = std::make_shared<std::string>(plainText);
            }
        }
    }

    value->udmfValue_ = entryValue;
    value->entries_.assign(entries_.begin() + 1, entries_.end());
    return value;
}
} // namespace MiscServices
} // namespace OHOS
