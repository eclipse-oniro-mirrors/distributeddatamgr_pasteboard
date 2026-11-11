/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "pasteboard_delay_manager.h"

#include "message_parcel_warp.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service.h"

namespace OHOS::MiscServices {
enum EntryPriority : uint8_t {
    PRIORITY_PLAIN_TEXT = 1,
    PRIORITY_HYPERLINK = 2,
    PRIORITY_HTML = 3,
    PRIORITY_FILE_URI = 4,
    PRIORITY_PIXEL_MAP = 5,
    PRIORITY_OTHERS = UINT8_MAX,
};

uint8_t DelayManager::GetEntryPriority(const std::string &utdId)
{
    static std::string UTDID_PLAIN_TEXT = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    static std::string UTDID_HYPERLINK = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK);
    static std::string UTDID_HTML = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    static std::string UTDID_FILE_URI = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    static std::string UTDID_PIXEL_MAP = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    static std::unordered_map<std::string, uint8_t> table = {
        {UTDID_PLAIN_TEXT, PRIORITY_PLAIN_TEXT},
        {UTDID_HYPERLINK, PRIORITY_HYPERLINK},
        {UTDID_HTML, PRIORITY_HTML},
        {UTDID_FILE_URI, PRIORITY_FILE_URI},
        {UTDID_PIXEL_MAP, PRIORITY_PIXEL_MAP},
    };
    auto iter = table.find(utdId);
    if (iter != table.end()) {
        return iter->second;
    }
    return PRIORITY_OTHERS;
}

void DelayManager::SortEntryInfo(std::vector<DelayEntryInfo> &entryInfos)
{
    std::sort(entryInfos.begin(), entryInfos.end(), [](const DelayEntryInfo &lhs, const DelayEntryInfo &rhs) {
        return std::tie(lhs.priority, lhs.recordId) < std::tie(rhs.priority, rhs.recordId);
    });
}

std::vector<DelayEntryInfo> DelayManager::GetAllDelayEntryInfo(const PasteData &data)
{
    std::vector<DelayEntryInfo> delayEntryInfos;
    for (const auto &record : data.AllRecords()) {
        if (record == nullptr || !record->IsDelayRecord()) {
            continue;
        }
        for (const auto &entry : record->GetEntries()) {
            if (entry != nullptr && std::holds_alternative<std::monostate>(entry->GetValue())) {
                delayEntryInfos.emplace_back(GetEntryPriority(entry->GetUtdId()), record->GetRecordId(), entry);
            }
        }
    }
    SortEntryInfo(delayEntryInfos);
    return delayEntryInfos;
}

std::vector<DelayEntryInfo> DelayManager::GetPrimaryDelayEntryInfo(const PasteData &data)
{
    std::vector<DelayEntryInfo> delayEntryInfos;
    for (const auto &record : data.AllRecords()) {
        if (record == nullptr || !record->IsDelayRecord()) {
            continue;
        }
        auto entries = record->GetEntries();
        if (entries.empty()) {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "entry empty, recordId=%{public}u", record->GetRecordId());
            continue;
        }
        auto entry = entries[0];
        if (entry != nullptr && std::holds_alternative<std::monostate>(entry->GetValue())) {
            delayEntryInfos.emplace_back(GetEntryPriority(entry->GetUtdId()), record->GetRecordId(), entry);
        }
    }
    SortEntryInfo(delayEntryInfos);
    return delayEntryInfos;
}

void DelayManager::GetLocalEntryValue(const std::vector<DelayEntryInfo> &delayEntryInfos,
    sptr<IPasteboardEntryGetter> entryGetter, PasteData &data)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(entryGetter != nullptr, PASTEBOARD_MODULE_SERVICE, "entryGetter is null");
    for (const auto &entryInfo : delayEntryInfos) {
        auto entry = entryInfo.entry;
        if (entry == nullptr || !std::holds_alternative<std::monostate>(entry->GetValue())) {
            continue;
        }

        PasteDataEntry tmpEntry = *entry;
        int32_t result = entryGetter->GetRecordValueByType(entryInfo.recordId, tmpEntry);
        if (result != static_cast<int32_t>(PasteboardError::E_OK)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "get record value fail, dataId=%{public}d, recordId=%{public}d, utdId=%{public}s",
                data.GetDataId(), entryInfo.recordId, entry->GetUtdId().c_str());
            continue;
        }

        std::unique_lock<std::shared_mutex> write(PasteboardService::pasteDataMutex_);
        if (data.rawDataSize_ + tmpEntry.rawDataSize_ < MessageParcelWarp::GetRawDataSize()) {
            entry->SetValue(tmpEntry.GetValue());
            entry->rawDataSize_ = tmpEntry.rawDataSize_;
            data.rawDataSize_ += tmpEntry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, tmpEntry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, tmpEntry.rawDataSize_);
        }
    }
}
} // namespace OHOS::MiscServices
