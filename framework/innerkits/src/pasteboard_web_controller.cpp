/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#include "pasteboard_web_controller.h"

#include <regex>

#include "file_uri.h"
#include "pasteboard_common.h"
#include "uri_permission_manager_client.h"
#include "pasteboard_hilog.h"

namespace {
constexpr const char *IMG_TAG_PATTERN = "<img.*?>";
constexpr const char *IMG_TAG_SRC_PATTERN = "src=(['\"])(.*?)\\1";
constexpr const char *IMG_TAG_SRC_HEAD = "src=\"";
constexpr const char *IMG_LOCAL_URI = "file:///";
constexpr const char *IMG_LOCAL_PATH = "://";
constexpr const char *FILE_SCHEME_PREFIX = "file://";

constexpr uint32_t FOUR_BYTES = 4;
constexpr uint32_t EIGHT_BIT = 8;
constexpr int32_t DOCS_LOCAL_PATH_SUBSTR_START_INDEX = 1;

struct Cmp {
    bool operator()(const uint32_t &lhs, const uint32_t &rhs) const
    {
        return lhs > rhs;
    }
};
} // namespace

namespace OHOS {
namespace MiscServices {

// static
PasteboardWebController &PasteboardWebController::GetInstance()
{
    static PasteboardWebController instance;
    return instance;
}

bool PasteboardWebController::SplitWebviewPasteData(PasteData &pasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "start");
    auto hasExtraRecord = false;
    for (const auto &record : pasteData.AllRecords()) {
        if (record->GetRecordId() == record->GetFrom()) {
            continue;
        }
        auto htmlEntry = record->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
        if (htmlEntry == nullptr) {
            continue;
        }
        std::shared_ptr<std::string> html = htmlEntry->ConvertToHtml();
        if (html == nullptr || html->empty()) {
            continue;
        }
        std::vector<std::shared_ptr<PasteDataRecord>> extraUriRecords = SplitHtml2Records(html, record->GetRecordId());
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "split html, recordId=%{public}u, uri count=%{public}zu",
            record->GetRecordId(), extraUriRecords.size());
        if (extraUriRecords.empty()) {
            continue;
        }
        hasExtraRecord = true;
        for (const auto &item : extraUriRecords) {
            pasteData.AddRecord(item);
        }
        record->SetFrom(record->GetRecordId());
    }
    if (hasExtraRecord) {
        pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "end");
    return hasExtraRecord;
}

void PasteboardWebController::SetWebviewPasteData(PasteData &pasteData,
    const std::pair<std::string, int32_t> &bundleIndex)
{
    if (pasteData.GetTag() != PasteData::WEBVIEW_PASTEDATA_TAG) {
        return;
    }
    for (auto &item : pasteData.AllRecords()) {
        if (item->GetUri() == nullptr || item->GetFrom() == 0 || item->GetRecordId() == item->GetFrom()) {
            continue;
        }
        std::shared_ptr<Uri> uri = item->GetUri();
        std::string puri = uri->ToString();
        if (puri.substr(0, strlen(IMG_LOCAL_URI)) == PasteData::IMG_LOCAL_URI &&
            puri.find(std::string(FILE_SCHEME_PREFIX) + PasteData::PATH_SHARE) == std::string::npos) {
            std::string path = uri->GetPath();
            std::string newUriStr = "";
            if (path.substr(0, std::string(PasteData::DOCS_LOCAL_TAG).size()) == PasteData::DOCS_LOCAL_TAG) {
                newUriStr = FILE_SCHEME_PREFIX;
                newUriStr += path.substr(DOCS_LOCAL_PATH_SUBSTR_START_INDEX);
            } else {
                newUriStr = FILE_SCHEME_PREFIX;
                std::string bundleIndexName;
                PasteBoardCommon::GetDirByBundleNameAndAppIndex(bundleIndex.first,
                    bundleIndex.second, bundleIndexName);
                newUriStr += bundleIndexName + path;
            }
            item->SetUri(std::make_shared<OHOS::Uri>(newUriStr));
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "uri: %{private}s -> %{private}s", puri.c_str(),
                newUriStr.c_str());
        }
    }
}

void PasteboardWebController::CheckAppUriPermission(PasteData &pasteData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "enter");
    std::vector<std::string> uris;
    std::vector<size_t> indexs;
    std::vector<bool> checkResults;
    for (size_t i = 0; i < pasteData.GetRecordCount(); i++) {
        auto item = pasteData.GetRecordAt(i);
        if (item == nullptr || item->GetOriginUri() == nullptr) {
            continue;
        }
        auto uri = item->GetOriginUri()->ToString();
        uris.emplace_back(uri);
        indexs.emplace_back(i);
    }
    if (uris.empty()) {
        return;
    }
    size_t offset = 0;
    size_t length = uris.size();
    size_t count = PasteData::URI_BATCH_SIZE;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "loop for CheckUriAuthorization, uri count=%{public}zu", uris.size());
    while (length > offset) {
        if (length - offset < PasteData::URI_BATCH_SIZE) {
            count = length - offset;
        }
        auto sendValues = std::vector<std::string>(uris.begin() + offset, uris.begin() + offset + count);
        std::vector<bool> ret = AAFwk::UriPermissionManagerClient::GetInstance().CheckUriAuthorization(
            sendValues, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION, pasteData.GetTokenId());
        checkResults.insert(checkResults.end(), ret.begin(), ret.end());
        offset += count;
    }
    if (checkResults.size() != indexs.size()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "check uri authorization fail");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "loop for SetGrantUriPermission");
    for (size_t i = 0; i < indexs.size(); i++) {
        auto item = pasteData.GetRecordAt(indexs[i]);
        if (item == nullptr || item->GetOriginUri() == nullptr) {
            continue;
        }
        item->SetGrantUriPermission(checkResults[i]);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "leave");
}

void PasteboardWebController::RefreshUri(std::shared_ptr<PasteDataRecord> &record, const std::string &targetBundle,
    int32_t appIndex)
{
    std::string bundleIndex;
    PasteBoardCommon::GetDirByBundleNameAndAppIndex(targetBundle, appIndex, bundleIndex);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(record->GetUri() != nullptr, PASTEBOARD_MODULE_COMMON,
        "id=%{public}u, uri is null", record->GetRecordId());
    PASTEBOARD_CHECK_AND_RETURN_LOGD(record->GetFrom() != 0 && record->GetFrom() != record->GetRecordId(),
        PASTEBOARD_MODULE_COMMON, "id=%{public}u, from=%{public}u", record->GetRecordId(), record->GetFrom());

    std::shared_ptr<Uri> uri = record->GetUri();
    std::string puri = uri->ToString();
    std::string realUri = puri;
    if (puri.substr(0, strlen(FILE_SCHEME_PREFIX)) == FILE_SCHEME_PREFIX) {
        AppFileService::ModuleFileUri::FileUri fileUri(puri);
        std::string realPath = PasteBoardCommon::IsPasteboardService() ? fileUri.GetRealPathBySA(bundleIndex) :
            fileUri.GetRealPath();
        PASTEBOARD_CHECK_AND_RETURN_LOGE(!realPath.empty(), PASTEBOARD_MODULE_COMMON,
            "file not exist, id=%{public}u, uri=%{public}s", record->GetRecordId(), puri.c_str());
        realUri = FILE_SCHEME_PREFIX;
        realUri += realPath;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "uri: %{private}s -> %{private}s", puri.c_str(), realUri.c_str());
    }
    if (realUri.find(PasteData::DISTRIBUTEDFILES_TAG) != std::string::npos) {
        record->SetConvertUri(realUri);
    } else {
        record->SetUri(std::make_shared<OHOS::Uri>(realUri));
    }
}

void PasteboardWebController::RetainUri(PasteData &pasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "start");
    if (!pasteData.IsLocalPaste()) {
        return;
    }
    // clear convert uri
    for (size_t i = 0; i < pasteData.GetRecordCount(); ++i) {
        auto record = pasteData.GetRecordAt(i);
        if (record != nullptr) {
            record->SetConvertUri("");
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "end");
}

void PasteboardWebController::RebuildWebviewPasteData(PasteData &pasteData, const std::string &targetBundle,
    int32_t appIndex)
{
    if (pasteData.GetTag() != PasteData::WEBVIEW_PASTEDATA_TAG) {
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "rebuild start, record count=%{public}zu", pasteData.GetRecordCount());
    auto justSplitHtml = false;
    for (auto &item : pasteData.AllRecords()) {
        justSplitHtml = justSplitHtml || item->GetFrom() > 0;
        RefreshUri(item, targetBundle, appIndex);
    }
    if (justSplitHtml) {
        MergeExtraUris2Html(pasteData);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "Rebuild webview PasteData end, merged uris into html.");
        return;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "Rebuild end, record count=%{public}zu", pasteData.GetRecordCount());
}

std::vector<std::shared_ptr<PasteDataRecord>> PasteboardWebController::SplitHtml2Records(
    const std::shared_ptr<std::string> &html, uint32_t recordId) noexcept
{
    std::vector<std::pair<std::string, uint32_t>> matchVec = SplitHtmlWithImgLabel(html);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "matchVec size: %{public}zu", matchVec.size());
    if (matchVec.empty()) {
        return {};
    }
    std::map<std::string, std::vector<uint8_t>> imgSrcMap = SplitHtmlWithImgSrcLabel(matchVec);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "imgSrcMap size: %{public}zu", imgSrcMap.size());
    return BuildPasteDataRecords(imgSrcMap, recordId);
}

void PasteboardWebController::MergeExtraUris2Html(PasteData &data)
{
    auto recordGroups = GroupRecordWithFrom(data);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "recordGroups size: %{public}zu", recordGroups.size());
    for (auto &recordGroup : recordGroups) {
        ReplaceHtmlRecordContentByExtraUris(recordGroup.second);
    }
    RemoveExtraUris(data);
}

std::shared_ptr<std::string> PasteboardWebController::RebuildHtml(std::shared_ptr<PasteData> pasteData) noexcept
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(pasteData != nullptr, nullptr, PASTEBOARD_MODULE_COMMON, "pasteData is null");
    std::vector<std::shared_ptr<PasteDataRecord>> pasteDataRecords = pasteData->AllRecords();
    std::shared_ptr<std::string> htmlData;
    std::map<uint32_t, std::pair<std::string, std::string>, Cmp> replaceUris;

    for (const auto &item : pasteDataRecords) {
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(item != nullptr, nullptr,
                                             PASTEBOARD_MODULE_COMMON, "item is null");
        std::shared_ptr<std::string> html = item->GetHtmlText();
        if (html != nullptr) {
            htmlData = html;
        }
        std::shared_ptr<OHOS::Uri> uri = item->GetUri();
        std::shared_ptr<MiscServices::MineCustomData> customData = item->GetCustomData();
        if (uri == nullptr || customData == nullptr) {
            continue;
        }
        std::map<std::string, std::vector<uint8_t>> customItemData = customData->GetItemData();
        for (const auto &itemData : customItemData) {
            for (uint32_t i = 0; i < itemData.second.size(); i += FOUR_BYTES) {
                uint32_t offset = static_cast<uint32_t>(itemData.second[i]) |
                                  static_cast<uint32_t>(itemData.second[i + 1] << 8) |
                                  static_cast<uint32_t>(itemData.second[i + 2] << 16) |
                                  static_cast<uint32_t>(itemData.second[i + 3] << 24);
                replaceUris[offset] = std::make_pair(uri->ToString(), itemData.first);
            }
        }
    }

    RemoveAllRecord(pasteData);
    for (auto &replaceUri : replaceUris) {
        htmlData->replace(replaceUri.first, replaceUri.second.second.size(), replaceUri.second.first);
    }
    pasteData->AddHtmlRecord(*htmlData);
    return htmlData;
}

std::vector<std::pair<std::string, uint32_t>> PasteboardWebController::SplitHtmlWithImgLabel(
    const std::shared_ptr<std::string> html) noexcept
{
    std::smatch results;
    std::string pattern(IMG_TAG_PATTERN);
    std::regex reg(pattern);
    std::string::const_iterator iterStart = html->begin();
    std::string::const_iterator iterEnd = html->end();
    std::vector<std::pair<std::string, uint32_t>> matchVec;

    try {
        while (std::regex_search(iterStart, iterEnd, results, reg)) {
            std::string tmp = results[0];
            iterStart = results[0].second;
            uint32_t offset = static_cast<uint32_t>(results[0].first - html->begin());

            matchVec.emplace_back(tmp, offset);
        }
    } catch (std::regex_error &e) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "Regex error !");
    }

    return matchVec;
}

std::map<std::string, std::vector<uint8_t>> PasteboardWebController::SplitHtmlWithImgSrcLabel(
    const std::vector<std::pair<std::string, uint32_t>> &matchVec) noexcept
{
    std::map<std::string, std::vector<uint8_t>> res;
    std::smatch match;
    std::regex re(IMG_TAG_SRC_PATTERN);
    for (const auto &node : matchVec) {
        std::string::const_iterator iterStart = node.first.begin();
        std::string::const_iterator iterEnd = node.first.end();

        try {
            while (std::regex_search(iterStart, iterEnd, match, re)) {
                std::string tmp = match[0];
                iterStart = match[0].second;
                uint32_t offset = static_cast<uint32_t>(match[0].first - node.first.begin());
                tmp = tmp.substr(strlen(IMG_TAG_SRC_HEAD));
                tmp.pop_back();
                if (!IsLocalURI(tmp)) {
                    continue;
                }
                offset += strlen(IMG_TAG_SRC_HEAD) + node.second;
                for (uint32_t i = 0; i < FOUR_BYTES; i++) {
                    res[tmp].emplace_back((offset >> (EIGHT_BIT * i)) & 0xff);
                }
            }
        } catch (std::regex_error &e) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "Regex error !");
        }
    }
    return res;
}

std::vector<std::shared_ptr<PasteDataRecord>> PasteboardWebController::BuildPasteDataRecords(
    const std::map<std::string, std::vector<uint8_t>> &imgSrcMap, uint32_t recordId) noexcept
{
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    for (const auto &item : imgSrcMap) {
        PasteDataRecord::Builder builder(MiscServices::MIMETYPE_TEXT_URI);
        auto uri = std::make_shared<OHOS::Uri>(item.first);
        builder.SetUri(uri);
        auto customData = std::make_shared<MiscServices::MineCustomData>();
        customData->AddItemData(item.first, item.second);
        builder.SetCustomData(customData);
        auto record = builder.Build();
        record->SetFrom(recordId);
        records.push_back(record);
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "Build extra records size: %{public}zu", records.size());
    return records;
}

void PasteboardWebController::RemoveRecordById(PasteData &pasteData, uint32_t recordId) noexcept
{
    for (uint32_t i = 0; i < pasteData.GetRecordCount(); i++) {
        if (pasteData.GetRecordAt(i)->GetRecordId() == recordId) {
            if (pasteData.RemoveRecordAt(i)) {
                PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON,
                    "WebClipboardController RemoveRecord success, i=%{public}u", i);
                return;
            }
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_COMMON,
                              "WebClipboardController RemoveRecord failed, i=%{public}u", i);
        }
    }
}

void PasteboardWebController::RemoveAllRecord(std::shared_ptr<PasteData> pasteData) noexcept
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteData != nullptr, PASTEBOARD_MODULE_COMMON, "pasteData is null");
    std::size_t recordCount = pasteData->GetRecordCount();
    for (uint32_t i = 0; i < recordCount; i++) {
        if (!pasteData->RemoveRecordAt(0)) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "WebClipboardController RemoveRecord failed, i=%{public}u", i);
        }
    }
}

bool PasteboardWebController::IsLocalURI(std::string &uri) noexcept
{
    return uri.substr(0, strlen(IMG_LOCAL_URI)) == std::string(IMG_LOCAL_URI) ||
        uri.find(IMG_LOCAL_PATH) == std::string::npos;
}

void PasteboardWebController::UpdateHtmlRecord(
    std::shared_ptr<PasteDataRecord> &htmlRecord, std::shared_ptr<std::string> &htmlData)
{
    if (htmlRecord == nullptr || htmlData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "htmlRecord or htmlData is nullptr");
        return;
    }
    auto entry = htmlRecord->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    if (entry == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "entry is nullptr");
        return;
    }
    auto entryValue = entry->GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry->SetValue(*htmlData);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::HTML_CONTENT] = *htmlData;
        entry->SetValue(newObject);
    }
    htmlRecord->AddEntryByMimeType(MIMETYPE_TEXT_HTML, entry);
    htmlRecord->SetFrom(0);
}

void PasteboardWebController::ReplaceHtmlRecordContentByExtraUris(
    std::vector<std::shared_ptr<PasteDataRecord>> &records)
{
    std::shared_ptr<PasteDataRecord> htmlRecord = nullptr;
    std::shared_ptr<std::string> htmlData;
    std::map<uint32_t, std::pair<std::string, std::string>, Cmp> replaceUris;
    for (const auto &item : records) {
        auto htmlEntry = item->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
        if (htmlEntry != nullptr) {
            auto html = htmlEntry->ConvertToHtml();
            if (html != nullptr && !html->empty()) {
                htmlData = html;
                htmlRecord = item;
                continue;
            }
        }
        std::shared_ptr<OHOS::Uri> uri = item->GetUri();
        std::shared_ptr<MiscServices::MineCustomData> customData = item->GetCustomData();
        if (!uri || !customData) {
            continue;
        }
        std::map<std::string, std::vector<uint8_t>> customItemData = customData->GetItemData();
        for (auto &itemData : customItemData) {
            for (uint32_t i = 0; i < itemData.second.size(); i += FOUR_BYTES) {
                uint32_t offset = static_cast<uint32_t>(itemData.second[i]) |
                                  static_cast<uint32_t>(itemData.second[i + 1] << 8) |
                                  static_cast<uint32_t>(itemData.second[i + 2] << 16) |
                                  static_cast<uint32_t>(itemData.second[i + 3] << 24);
                replaceUris[offset] = std::make_pair(uri->ToString(), itemData.first);
            }
        }
    }
    if (htmlData == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_COMMON, "htmlData is nullptr");
        return;
    }

    for (const auto &replaceUri : replaceUris) {
        htmlData->replace(replaceUri.first, replaceUri.second.second.size(), replaceUri.second.first);
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "replace uri count: %{public}zu", replaceUris.size());
    if (htmlRecord != nullptr) {
        UpdateHtmlRecord(htmlRecord, htmlData);
    }
}

std::map<std::uint32_t, std::vector<std::shared_ptr<PasteDataRecord>>> PasteboardWebController::GroupRecordWithFrom(
    PasteData &data)
{
    std::map<std::uint32_t, std::vector<std::shared_ptr<PasteDataRecord>>> groupMap;
    for (const auto &record : data.AllRecords()) {
        if (record->GetFrom() == 0) {
            continue;
        }
        auto item = groupMap.find(record->GetFrom());
        auto value = item != groupMap.end() ? item->second : std::vector<std::shared_ptr<PasteDataRecord>>();
        value.emplace_back(record);
        groupMap.insert_or_assign(record->GetFrom(), value);
    }
    return groupMap;
}

void PasteboardWebController::RemoveExtraUris(PasteData &data)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "Before remove record count: %{public}zu", data.AllRecords().size());
    for (const auto &record : data.AllRecords()) {
        if (record->GetFrom() > 0 && record->GetMimeType() == MIMETYPE_TEXT_URI) {
            RemoveRecordById(data, record->GetRecordId());
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "After remove record count: %{public}zu", data.AllRecords().size());
}
} // namespace MiscServices
} // namespace OHOS
