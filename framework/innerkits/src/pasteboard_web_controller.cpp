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
#include "ipc_skeleton.h"
#include "parameters.h"
#include "pasteboard_common.h"
#include "pasteboard_hilog.h"
#include "pasteboard_img_extractor.h"
#include "uri_permission_manager_client.h"

namespace {
constexpr const char *IMG_TAG_PATTERN = "<img.*?>";
constexpr const char *IMG_TAG_SRC_PATTERN = "src=(['\"])(.*?)\\1";
constexpr const char *IMG_TAG_SRC_HEAD = "src=\"";
constexpr const char *IMG_LOCAL_PATH = "://";
constexpr const char *FILE_SCHEME = "file";

constexpr uint32_t FOUR_BYTES = 4;
constexpr uint32_t EIGHT_BIT = 8;
constexpr uid_t ANCO_SERVICE_BROKER_UID = 5557;

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

bool PasteboardWebController::SplitWebviewPasteData(PasteData &pasteData, const std::string &bundleIndex,
    int32_t userId)
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
        std::vector<std::shared_ptr<PasteDataRecord>> extraUriRecords = SplitHtml2Records(html, record->GetRecordId(),
            bundleIndex, userId);
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

void PasteboardWebController::SetWebviewPasteData(PasteData &pasteData, const std::string &bundleIndex)
{
    if (pasteData.GetTag() != PasteData::WEBVIEW_PASTEDATA_TAG) {
        return;
    }
    for (auto &item : pasteData.AllRecords()) {
        if (item->GetUriV0() == nullptr || item->GetFrom() == 0 || item->GetRecordId() == item->GetFrom()) {
            continue;
        }
        std::shared_ptr<Uri> uri = item->GetUriV0();
        std::string uriStr = uri->ToString();
        std::string newUriStr;
        if (uriStr.find(PasteboardImgExtractor::IMG_LOCAL_URI) == 0) {
            if (uriStr.find(PasteboardImgExtractor::DOC_LOCAL_URI) == 0) {
                newUriStr = uriStr.replace(0, std::strlen(PasteboardImgExtractor::DOC_LOCAL_URI),
                    PasteboardImgExtractor::DOC_URI_PREFIX);
            } else {
                newUriStr = uriStr.replace(0, std::strlen(PasteboardImgExtractor::IMG_LOCAL_URI),
                    PasteboardImgExtractor::FILE_SCHEME_PREFIX + bundleIndex + "/");
            }
            item->SetUri(std::make_shared<OHOS::Uri>(newUriStr));
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "uri: %{private}s -> %{private}s", uriStr.c_str(),
                newUriStr.c_str());
        }
    }
}

void PasteboardWebController::CheckAppUriPermission(PasteData &pasteData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "enter");
    std::vector<std::string> uris;
    std::vector<size_t> indexs;
    std::vector<bool> checkReadResults;
    std::vector<bool> checkWriteResults;
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    bool ancoFlag = (callingUid == ANCO_SERVICE_BROKER_UID);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "callingUid=%{public}d, ancoFlag=%{public}u", callingUid, ancoFlag);
    int32_t uriCount = GetNeedCheckUris(pasteData, uris, indexs, ancoFlag);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "uri count=%{public}d", uriCount);
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
        std::vector<bool> checkReadRet = AAFwk::UriPermissionManagerClient::GetInstance().CheckUriAuthorization(
            sendValues, AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION, pasteData.GetTokenId());
        checkReadResults.insert(checkReadResults.end(), checkReadRet.begin(), checkReadRet.end());
        std::vector<bool> checkWriteRet = AAFwk::UriPermissionManagerClient::GetInstance().CheckUriAuthorization(
            sendValues, AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION, pasteData.GetTokenId());
        checkWriteResults.insert(checkWriteResults.end(), checkWriteRet.begin(), checkWriteRet.end());
        offset += count;
    }
    if (checkReadResults.size() != indexs.size() || checkWriteResults.size() != indexs.size()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "check uri authorization fail");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "loop for SetGrantUriPermission");
    bool isNeedPersistance = OHOS::system::GetBoolParameter("const.pasteboard.uri_persistable_permission", false);
    for (size_t i = 0; i < indexs.size(); i++) {
        auto item = pasteData.GetRecordAt(indexs[i]);
        if (item == nullptr || item->GetOriginUri() == nullptr) {
            continue;
        }
        item->SetGrantUriPermission(checkReadResults[i]);
        SetUriPermission(item, checkReadResults[i], checkWriteResults[i], isNeedPersistance);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "leave");
}

void PasteboardWebController::SetUriPermission(
    std::shared_ptr<PasteDataRecord> &record, bool isRead, bool isWrite, bool isNeedPersistance)
{
    if (!isRead) {
        return;
    }
    if (isNeedPersistance && isWrite) {
        record->SetUriPermission(PasteDataRecord::READ_WRITE_PERMISSION);
        return;
    }
    record->SetUriPermission(PasteDataRecord::READ_PERMISSION);
}

int32_t PasteboardWebController::GetNeedCheckUris(PasteData &pasteData, std::vector<std::string> &uris,
    std::vector<size_t> &indexs, bool ancoFlag)
{
    int32_t uriCount = 0;
    bool isNeedPersistance = OHOS::system::GetBoolParameter("const.pasteboard.uri_persistable_permission", false);
    uint32_t flag = isNeedPersistance ? PasteDataRecord::READ_WRITE_PERMISSION : PasteDataRecord::READ_PERMISSION;
    for (size_t i = 0; i < pasteData.GetRecordCount(); i++) {
        auto item = pasteData.GetRecordAt(i);
        if (item == nullptr || item->GetOriginUri() == nullptr) {
            continue;
        }
        uriCount++;
        if (ancoFlag) {
            item->SetGrantUriPermission(true);
            item->SetUriPermission(flag);
            continue;
        }
        auto uri = item->GetOriginUri()->ToString();
        uris.emplace_back(uri);
        indexs.emplace_back(i);
    }
    return uriCount;
}

void PasteboardWebController::RefreshUri(std::shared_ptr<PasteDataRecord> &record, const std::string &targetBundle,
    int32_t appIndex)
{
    std::string bundleIndex;
    PasteBoardCommon::GetDirByBundleNameAndAppIndex(targetBundle, appIndex, bundleIndex);
    PASTEBOARD_CHECK_AND_RETURN_LOGD(record->GetUriV0() != nullptr, PASTEBOARD_MODULE_COMMON,
        "id=%{public}u, uri is null", record->GetRecordId());
    PASTEBOARD_CHECK_AND_RETURN_LOGD(record->GetFrom() != 0 && record->GetFrom() != record->GetRecordId(),
        PASTEBOARD_MODULE_COMMON, "id=%{public}u, from=%{public}u", record->GetRecordId(), record->GetFrom());

    std::shared_ptr<Uri> uri = record->GetUriV0();
    std::string puri = uri->ToString();
    std::string realUri = puri;
    if (puri.find(PasteboardImgExtractor::FILE_SCHEME_PREFIX) == 0) {
        AppFileService::ModuleFileUri::FileUri fileUri(puri);
        std::string realPath = PasteBoardCommon::IsPasteboardService() ? fileUri.GetRealPathBySA(bundleIndex) :
            fileUri.GetRealPath();
        PASTEBOARD_CHECK_AND_RETURN_LOGE(!realPath.empty(), PASTEBOARD_MODULE_COMMON,
            "file not exist, id=%{public}u, uri=%{public}s", record->GetRecordId(), puri.c_str());
        realUri = PasteboardImgExtractor::FILE_SCHEME_PREFIX;
        realUri += realPath;
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "uri: %{private}s -> %{private}s", puri.c_str(), realUri.c_str());
    }
    if (realUri.find(PasteData::DISTRIBUTEDFILES_TAG) != std::string::npos) {
        record->SetConvertUri(realUri);
    } else {
        record->SetUri(std::make_shared<OHOS::Uri>(realUri));
    }
}

bool PasteboardWebController::IsValidUri(const std::shared_ptr<OHOS::Uri> uriPtr, bool hasPermission) noexcept
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uriPtr != nullptr, false, PASTEBOARD_MODULE_COMMON, "uri is empty");

    std::string scheme = uriPtr->GetScheme();
    std::string authority = uriPtr->GetAuthority();
    std::string uriStr = uriPtr->ToString();

    if (scheme.empty() || (scheme == FILE_SCHEME && authority.empty())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "invalid uri:%{private}s, scheme:%{public}s, authority:%{public}s",
            uriStr.c_str(), scheme.c_str(), authority.c_str());
        return false;
    }
    if (scheme == FILE_SCHEME && !hasPermission) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "no perm uri:%{private}s, scheme:%{public}s, authority:%{public}s",
            uriStr.c_str(), scheme.c_str(), authority.c_str());
        return false;
    }
    return true;
}

bool PasteboardWebController::RemoveInvalidUri(PasteDataEntry &entry)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry.GetMimeType() == MIMETYPE_TEXT_URI, false, PASTEBOARD_MODULE_COMMON,
        "entry type invalid, type=%{public}s", entry.GetMimeType().c_str());

    auto uriPtr = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uriPtr != nullptr, false, PASTEBOARD_MODULE_COMMON,
        "entry convert uri failed");

    if (IsValidUri(uriPtr, true)) {
        return false;
    }

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue("");
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::FILE_URI_PARAM] = "";
        entry.SetValue(newObject);
    }
    return true;
}

void PasteboardWebController::RemoveInvalidUri(PasteData &data)
{
    if (data.IsLocalPaste()) {
        return;
    }

    uint32_t removeCount = 0;
    auto emptyUri = std::make_shared<OHOS::Uri>("");
    size_t recordCount = data.GetRecordCount();
    for (size_t i = 0; i < recordCount; ++i) {
        auto record = data.GetRecordAt(i);
        if (record == nullptr) {
            continue;
        }
        auto uriPtr = record->GetOriginUri();
        if (uriPtr == nullptr) {
            continue;
        }
        if (IsValidUri(uriPtr, record->HasGrantUriPermission())) {
            continue;
        }
        record->SetUri(emptyUri);
        record->SetConvertUri("");
        removeCount++;
    }

    if (removeCount > 0) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_COMMON, "remove count=%{public}u", removeCount);
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

void PasteboardWebController::RemoveInvalidImgSrc(const std::vector<std::string> &validImgSrcList,
    std::map<std::string, std::vector<uint8_t>> &imgSrcMap) noexcept
{
    std::unordered_set<std::string> validImgSrcSet(validImgSrcList.begin(), validImgSrcList.end());
    auto it = imgSrcMap.begin();
    while (it != imgSrcMap.end()) {
        if (validImgSrcSet.find(it->first) == validImgSrcSet.end()) {
            it = imgSrcMap.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<std::shared_ptr<PasteDataRecord>> PasteboardWebController::SplitHtml2Records(
    const std::shared_ptr<std::string> &html, uint32_t recordId, const std::string &bundleIndex,
    int32_t userId) noexcept
{
    if (html == nullptr) {
        return {};
    }
    std::vector<std::pair<std::string, uint32_t>> matchVec = SplitHtmlWithImgLabel(html);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "matchVec size: %{public}zu", matchVec.size());
    if (matchVec.empty()) {
        return {};
    }
    std::map<std::string, std::vector<uint8_t>> imgSrcMap = SplitHtmlWithImgSrcLabel(matchVec);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_COMMON, "imgSrcMap size: %{public}zu", imgSrcMap.size());
    if (imgSrcMap.empty()) {
        return {};
    }
    auto validImgSrcList = PasteboardImgExtractor::GetInstance().ExtractImgSrc(*html, bundleIndex, userId);
    RemoveInvalidImgSrc(validImgSrcList, imgSrcMap);
    if (imgSrcMap.empty()) {
        return {};
    }
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
        std::shared_ptr<std::string> html = item->GetHtmlTextV0();
        if (html != nullptr) {
            htmlData = html;
        }
        std::shared_ptr<OHOS::Uri> uri = item->GetUriV0();
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

    while (std::regex_search(iterStart, iterEnd, results, reg)) {
        std::string tmp = results[0];
        iterStart = results[0].second;
        uint32_t offset = static_cast<uint32_t>(results[0].first - html->begin());
        matchVec.emplace_back(tmp, offset);
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
        if (pasteData.GetRecordAt(i) != nullptr && pasteData.GetRecordAt(i)->GetRecordId() == recordId) {
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
    return uri.find(PasteboardImgExtractor::IMG_LOCAL_URI) == 0 ||
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
        std::shared_ptr<OHOS::Uri> uri = item->GetUriV0();
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

int32_t PasteboardWebController::GetGlobalShareOption(const std::vector<uint32_t> &tokenIds,
    std::unordered_map<uint32_t, int32_t>& funcResult)
{
    if (!IsCallerUidValid()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No Permission");
        funcResult = {};
        return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
    }
    std::map<uint32_t, ShareOption> result;
    if (tokenIds.empty()) {
        globalShareOptions_.ForEach([&result](const uint32_t &key, GlobalShareOption &value) {
            result[key] = value.shareOption;
            return false;
        });
        for (const auto &pair : result) {
            funcResult[pair.first] = static_cast<int32_t>(pair.second);
        }
        return ERR_OK;
    }
    for (const uint32_t &tokenId : tokenIds) {
        globalShareOptions_.ComputeIfPresent(tokenId, [&result](const uint32_t &key, GlobalShareOption &value) {
            result[key] = value.shareOption;
            return true;
        });
    }
    for (const auto &pair : result) {
        funcResult[pair.first] = static_cast<int32_t>(pair.second);
    }
    return ERR_OK;
}

bool PasteboardWebController::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "called token id: %{public}" PRIu64, tokenId);
    return (tokenId & SYSTEM_APP_MASK) == SYSTEM_APP_MASK;
}

int32_t PasteboardWebController::SetAppShareOptions(int32_t shareOptions)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(PasteData::IsValidShareOption(shareOptions),
        static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), PASTEBOARD_MODULE_SERVICE,
        "shareOptions invalid, shareOptions=%{public}d", shareOptions);
    auto fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (!IsSystemAppByFullTokenID(fullTokenId)) {
        if (shareOptions != ShareOption::InApp) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "param is invalid");
            return static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR);
        }
        auto isManageGrant = PermissionUtils::IsPermissionGranted(MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION,
            tokenId);
        if (!isManageGrant) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No permission, token id: 0x%{public}x.", tokenId);
            return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
        }
    }
    GlobalShareOption option = {.source = APP, .shareOption = static_cast<ShareOption>(shareOptions)};
    auto isAbsent = globalShareOptions_.ComputeIfAbsent(tokenId, [&option](const uint32_t &tokenId) {
        return option;
    });
    if (!isAbsent) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Settings already exist, token id: 0x%{public}x.", tokenId);
        return static_cast<int32_t>(PasteboardError::INVALID_OPERATION_ERROR);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Set token id: 0x%{public}x share options: %{public}d success.",
        tokenId, shareOptions);
    return 0;
}

int32_t PasteboardWebController::RemoveAppShareOptions()
{
    auto fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    if (!IsSystemAppByFullTokenID(fullTokenId)) {
        auto isManageGrant = PermissionUtils::IsPermissionGranted(MANAGE_PASTEBOARD_APP_SHARE_OPTION_PERMISSION,
            tokenId);
        if (!isManageGrant) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "No permission, token id: 0x%{public}x.", tokenId);
            return static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR);
        }
    }
    std::map<uint32_t, GlobalShareOption> result;
    globalShareOptions_.ComputeIfPresent(tokenId, [&result](const uint32_t &key, GlobalShareOption &value) {
        result[key] = value;
        return true;
    });
    if (!result.empty()) {
        if (result[tokenId].source == APP) {
            globalShareOptions_.Erase(tokenId);
            PASTEBOARD_HILOGI(
                PASTEBOARD_MODULE_SERVICE, "Remove token id: 0x%{public}x share options success.", tokenId);
            return 0;
        } else {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Can not remove token id: 0x%{public}x.", tokenId);
            return 0;
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "This token id: 0x%{public}x not set.", tokenId);
    return 0;
}

void PasteboardWebController::UpdateShareOption(PasteData &pasteData)
{
    globalShareOptions_.ComputeIfPresent(
        pasteData.GetTokenId(), [&pasteData](const uint32_t &tokenId, GlobalShareOption &option) {
            pasteData.SetShareOption(option.shareOption);
            return true;
        });
}

bool PasteboardWebController::CheckMdmShareOption(PasteData &pasteData)
{
    bool result = false;
    globalShareOptions_.ComputeIfPresent(
        pasteData.GetTokenId(), [&result](const uint32_t &tokenId, GlobalShareOption &option) {
            if (option.source == MDM) {
                result = true;
            }
            return true;
        });
    return result;
}

bool PasteboardWebController::IsCallerUidValid()
{
    pid_t callingUid = IPCSkeleton::GetCallingUid();
    if (callingUid == EDM_UID || (uid_ != -1 && callingUid == uid_)) {
        return true;
    }
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "callingUid error: %{public}d.", callingUid);
    return false;
}

void PasteboardWebController::ThawInputMethod(pid_t imePid)
{
    auto type = ResourceSchedule::ResType::RES_TYPE_SA_CONTROL_APP_EVENT;
    auto status = ResourceSchedule::ResType::SaControlAppStatus::SA_START_APP;

    std::unordered_map<std::string, std::string> payload = {
        { "saId", std::to_string(PASTEBOARD_SERVICE_ID) },
        { "saName", PASTEBOARD_SERVICE_SA_NAME },
        { "extensionType", std::to_string(static_cast<int32_t>(AppExecFwk::ExtensionAbilityType::INPUTMETHOD)) },
        { "pid", std::to_string(imePid) },
        { "isDelay", std::to_string(true) } };
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "report RSS need thaw:pid = %{public}d", imePid);
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(type, status, payload);
}

bool PasteboardWebController::IsNeedThaw()
{
    auto imc = InputMethodController::GetInstance();
    if (imc == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "InputMethodController is nullptr!");
        return false;
    }

    std::shared_ptr<Property> property;
    int32_t ret = imc->GetDefaultInputMethod(property);
    if (ret != ErrorCode::NO_ERROR || property == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "default input method is nullptr!");
        return false;
    }
    return true;
}

void PasteboardWebController::NotifyObservers(std::string bundleName, int32_t userId, PasteboardEventStatus status)
{
    auto [hasPid, pid] = imeMap_.Find(userId);
    if (hasPid && IsNeedThaw()) {
        ThawInputMethod(pid);
    }
    std::thread thread([this, bundleName, userId, status]() {
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto &observers : observerLocalChangedMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerLocalChangedMap_.second is nullptr");
                continue;
            }
            for (const auto &observer : *(observers.second)) {
                if (status != PasteboardEventStatus::PASTEBOARD_READ && userId == observers.first.first) {
                    observer->OnPasteboardChanged();
                }
            }
        }
        IPasteboardChangedObserver::PasteboardChangedEvent event;
        event.status = static_cast<int32_t>(status);
        event.userId = userId;
        event.bundleName = bundleName;
        for (auto &observers : observerEventMap_) {
            if (observers.second == nullptr) {
                PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "observerEventMap_.second is nullptr");
                continue;
            }
            for (const auto &observer : *(observers.second)) {
                observer->OnPasteboardEvent(event);
            }
        }
    });
    thread.detach();
}

bool PasteboardWebController::SetPasteboardHistory(HistoryInfo &info)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(info.userId != ERROR_USERID, false,
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::string history = std::move(info.time) + " " + std::move(info.bundleName) + " " + std::move(info.state) + " " +
                          " " + std::move(info.remote) + " userId:" + std::to_string(info.userId);
    constexpr const size_t DATA_HISTORY_SIZE = 10;
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    if (dataHistory_.size() == DATA_HISTORY_SIZE) {
        dataHistory_.erase(dataHistory_.begin());
    }
    dataHistory_.push_back(std::move(history));
    return true;
}

int PasteboardWebController::Dump(int fd, const std::vector<std::u16string> &args)
{
    int uid = static_cast<int>(IPCSkeleton::GetCallingUid());
    const int maxUid = 10000;
    if (uid > maxUid) {
        return 0;
    }

    std::vector<std::string> argsStr;
    for (auto item : args) {
        argsStr.emplace_back(Str16ToStr8(item));
    }

    if (PasteboardDumpHelper::GetInstance().Dump(fd, argsStr)) {
        return 0;
    }
    return 0;
}

std::string PasteboardWebController::GetTime()
{
    constexpr int USEC_TO_MSEC = 1000;
    time_t timeSeconds = time(0);
    if (timeSeconds == -1) {
        return FAIL_TO_GET_TIME_STAMP;
    }
    struct tm nowTime;
    localtime_r(&timeSeconds, &nowTime);

    struct timeval timeVal = { 0, 0 };
    gettimeofday(&timeVal, nullptr);

    std::string targetTime = std::to_string(nowTime.tm_year + 1900) + "-" + std::to_string(nowTime.tm_mon + 1) + "-" +
                             std::to_string(nowTime.tm_mday) + " " + std::to_string(nowTime.tm_hour) + ":" +
                             std::to_string(nowTime.tm_min) + ":" + std::to_string(nowTime.tm_sec) + "." +
                             std::to_string(timeVal.tv_usec / USEC_TO_MSEC);
    return targetTime;
}

std::string PasteboardWebController::DumpHistory() const
{
    std::string result;
    std::lock_guard<decltype(historyMutex_)> lg(historyMutex_);
    auto userId = GetCurrentAccountId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(userId != ERROR_USERID, "Access history fail! invalid userId.",
        PASTEBOARD_MODULE_SERVICE, "invalid userId");
    if (!dataHistory_.empty()) {
        result.append("Access history last ten times: ").append("\n");
        for (auto iter = dataHistory_.rbegin(); iter != dataHistory_.rend(); ++iter) {
            std::string userIdPrefix = " userId:" + std::to_string(userId);
            size_t userIdPos = (*iter).find(userIdPrefix);
            if (userIdPos != std::string::npos) {
                std::string historyWithoutUserId = (*iter).substr(0, userIdPos);
                result.append("          ").append(historyWithoutUserId).append("\n");
            }
        }
    } else {
        result.append("Access history fail! dataHistory_ no data.").append("\n");
    }
    return result;
}

std::string PasteboardWebController::DumpData()
{
    auto userId = GetCurrentAccountId();
    if (userId == ERROR_USERID) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query active user failed.");
        return "";
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "id = %{public}d", userId);
    auto it = clips_.Find(userId);
    std::string result;
    if (it.first && it.second != nullptr) {
        size_t recordCounts = it.second->GetRecordCount();
        auto property = it.second->GetProperty();
        std::string shareOption;
        PasteData::ShareOptionToString(property.shareOption, shareOption);
        std::string sourceDevice;
        if (property.isRemote) {
            sourceDevice = "remote";
        } else {
            sourceDevice = "local";
        }
        result.append("|Owner       :  ")
            .append(property.bundleName)
            .append("\n")
            .append("|Timestamp   :  ")
            .append(property.setTime)
            .append("\n")
            .append("|Share Option:  ")
            .append(shareOption)
            .append("\n")
            .append("|Record Count:  ")
            .append(std::to_string(recordCounts))
            .append("\n")
            .append("|Mime types  :  {");
        if (!property.mimeTypes.empty()) {
            for (size_t i = 0; i < property.mimeTypes.size(); ++i) {
                result.append(property.mimeTypes[i]).append(",");
            }
        }
        result.append("}").append("\n").append("|source device:  ").append(sourceDevice);
    } else {
        result.append("No copy data.").append("\n");
    }
    return result;
}

bool PasteboardWebController::IsFocusedApp(uint32_t tokenId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "caller is not application");
        return true;
    }
    FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance().GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance().GetFocusWindowInfo(info);
#endif
    auto callPid = IPCSkeleton::GetCallingPid();
    if (callPid == info.pid_) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pid is same, it is focused app");
        return true;
    }
    bool isFocused = false;
    int32_t ret = PasteboardAbilityManager::CheckUIExtensionIsFocused(tokenId, isFocused);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "check result:%{public}d, isFocused:%{public}d", ret, isFocused);
    return ret == NO_ERROR && isFocused;
}

void PasteboardWebController::DeletePreSyncP2pFromP2pMap(const std::string &networkId)
{
    std::string taskName = P2P_PRESYNC_ID + networkId;
    if (ffrtTimer_) {
        ffrtTimer_->CancelTimer(taskName);
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.ComputeIfPresent(networkId, [this](const auto &key, auto &value) {
        value.ComputeIfPresent(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
            return false;
        });
        return true;
    });
    DeletePreSyncP2pMap(networkId);
}

void PasteboardWebController::DeletePreSyncP2pMap(const std::string &networkId)
{
    auto p2pIter = preSyncP2pMap_.find(networkId);
    if (p2pIter != preSyncP2pMap_.end()) {
        if (p2pIter->second) {
            p2pIter->second->SetValue(SET_VALUE_SUCCESS);
        }
        preSyncP2pMap_.erase(networkId);
    }
}

void PasteboardWebController::AddPreSyncP2pTimeoutTask(const std::string &networkId)
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    std::string taskName = P2P_PRESYNC_ID + networkId;
    ffrtTimer_->CancelTimer(taskName);
    FFRTTask p2pTask = [this, networkId] {
        std::thread thread([=]() {
            PasteComplete(networkId, P2P_PRESYNC_ID);
            std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
            DeletePreSyncP2pMap(networkId);
        });
        thread.detach();
    };
    ffrtTimer_->SetTimer(taskName, p2pTask, PRE_ESTABLISH_P2P_LINK_TIME);
}

void PasteboardWebController::RegisterPreSyncCallback(std::shared_ptr<ClipPlugin> clipPlugin)
{
    if (!clipPlugin) {
        return;
    }
    clipPlugin->RegisterPreSyncCallback(std::bind(&PasteboardWebController::PreEstablishP2PLinkCallback,
        this, std::placeholders::_1, std::placeholders::_2));
    clipPlugin->RegisterPreSyncMonitorCallback(std::bind(&PasteboardWebController::PreSyncSwitchMonitorCallback, this));
}

bool PasteboardWebController::OpenP2PLinkForPreEstablish(const std::string &networkId, ClipPlugin *clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    DmDeviceInfo remoteDevice;
    auto ret = DMAdapter::GetInstance().GetRemoteDeviceInfo(networkId, remoteDevice);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        DeletePreSyncP2pFromP2pMap(networkId);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote device is not exist, ret:%{public}d", ret);
        return false;
    }
    auto status = DistributedFileDaemonManager::GetInstance().ConnectDfs(networkId);
    if (status != RESULT_OK) {
        DeletePreSyncP2pFromP2pMap(networkId);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "open p2p error, status:%{public}d", status);
        return false;
    }
    std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
    p2pMap_.Compute(networkId, [](const auto &key, auto &value) {
        value.Compute(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
            value.isSuccess = true;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "preP2pLink isSuccess:%{public}d", value.isSuccess);
            return true;
        });
        return true;
    });
    if (clipPlugin) {
        status = clipPlugin->PublishServiceState(networkId, ClipPlugin::ServiceStatus::CONNECT_SUCC);
        if (status != RESULT_OK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "Publish state connect_succ error, status:%{public}d", status);
        }
    }
    AddPreSyncP2pTimeoutTask(networkId);
    return true;
#else
    return false;
#endif
}

void PasteboardWebController::PreEstablishP2PLink(const std::string &networkId, ClipPlugin *clipPlugin)
{
#ifdef PB_DEVICE_MANAGER_ENABLE
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink enter");
    std::shared_ptr<BlockObject<int32_t>> pasteBlock = nullptr;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        if (p2pEstablishInfo_.pasteBlock && p2pEstablishInfo_.networkId == networkId) {
            return;
        }
        auto p2pNetwork = p2pMap_.Find(networkId);
        bool isP2pSuccess = p2pNetwork.first && p2pNetwork.second.Find(P2P_PRESYNC_ID).first &&
            p2pNetwork.second.Find(P2P_PRESYNC_ID).second.isSuccess == true;
        if (isP2pSuccess) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Pre P2pEstablish exist");
            AddPreSyncP2pTimeoutTask(networkId);
            return;
        }
        pasteBlock = std::make_shared<BlockObject<int32_t>>(MIN_TRANMISSION_TIME, 0);
        if (!pasteBlock) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc BlockObject");
            return;
        }
        p2pMap_.Compute(networkId, [this](const auto &key, auto &value) {
            value.Compute(P2P_PRESYNC_ID, [](const auto &key, auto &value) {
                value.callPid = 0;
                value.isSuccess = false;
                return true;
            });
            return true;
        });
        preSyncP2pMap_.emplace(networkId, pasteBlock);
    }
    if (OpenP2PLinkForPreEstablish(networkId, clipPlugin)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink Finish");
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLink failed");
    }
    pasteBlock->SetValue(SET_VALUE_SUCCESS);
#endif
}

void PasteboardWebController::PreEstablishP2PLinkCallback(const std::string &networkId, ClipPlugin *clipPlugin)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLinkCallback enter");
    if (networkId.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "PreEstablishP2PLinkCallback failed, networkId is null");
        return;
    }
    if (!clipPlugin) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        return;
    }
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
#ifdef PB_DEVICE_MANAGER_ENABLE
    FFRTTask p2pTask = [this, networkId, clipPlugin] {
        std::thread thread([=]() {
            PreEstablishP2PLink(networkId, clipPlugin);
        });
        thread.detach();
    };
    std::string taskName = "PreEstablishP2PLink_";
    taskName += networkId;
    ffrtTimer_->SetTimer(taskName, p2pTask);
#endif
}

void PasteboardWebController::PreSyncRemotePasteboardData()
{
    auto clipPlugin = GetClipPlugin();
    if (!clipPlugin) {
        return;
    }
    if (!clipPlugin->NeedSyncTopEvent()) {
        return;
    }
    const int32_t DEFAULT_USER_ID = 0;
    clipPlugin->SendPreSyncEvent(DEFAULT_USER_ID);
}

void PasteboardWebController::PreSyncSwitchMonitorCallback()
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    FFRTTask monitorTask = [this] {
        std::thread thread([=]() {
            RegisterPreSyncMonitor();
        });
        thread.detach();
    };
    ffrtTimer_->SetTimer(REGISTER_PRESYNC_MONITOR, monitorTask);
}

void PasteboardWebController::RegisterPreSyncMonitor()
{
    if (!ffrtTimer_) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ffrtTimer_ is null");
        return;
    }
    if (!MMI::InputManager::GetInstance()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "MMI::InputManager is null");
        return;
    }
    FFRTTask monitorTask = [this] {
        std::thread thread([=]() {
            UnRegisterPreSyncMonitor();
        });
        thread.detach();
    };
    if (subscribeActiveId_ != INVALID_SUBSCRIBE_ID) {
        ffrtTimer_->SetTimer(UNREGISTER_PRESYNC_MONITOR, monitorTask, PRESYNC_MONITOR_TIME);
        return;
    }
    std::shared_ptr<InputEventCallback> preSyncMonitor =
        std::make_shared<InputEventCallback>(InputEventCallback::INPUTTYPE_PRESYNC, this);
    if (!preSyncMonitor) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "failed to alloc InputEventCallback");
        return;
    }
    subscribeActiveId_ = MMI::InputManager::GetInstance()->SubscribeInputActive(
        std::static_pointer_cast<MMI::IInputEventConsumer>(preSyncMonitor), PRESYNC_MONITOR_INTERVAL_MILLISECONDS);
    if (subscribeActiveId_ < 0) {
        subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SubscribeInputActive failed");
        return;
    }
    ffrtTimer_->SetTimer(UNREGISTER_PRESYNC_MONITOR, monitorTask, PRESYNC_MONITOR_TIME);
}

void PasteboardWebController::UnRegisterPreSyncMonitor()
{
    if (!MMI::InputManager::GetInstance()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "MMI::InputManager is null");
        return;
    }
    if (subscribeActiveId_ != INVALID_SUBSCRIBE_ID) {
        MMI::InputManager::GetInstance()->UnsubscribeInputActive(subscribeActiveId_);
        subscribeActiveId_ = INVALID_SUBSCRIBE_ID;
    }
}

FocusedAppInfo PasteboardWebController::GetFocusedAppInfo(void) const
{
    FocusedAppInfo appInfo = { 0 };
    FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    WindowManagerLite::GetInstance().GetFocusWindowInfo(info);
#else
    WindowManager::GetInstance().GetFocusWindowInfo(info);
#endif
    appInfo.windowId = info.windowId_;
    appInfo.abilityToken = info.abilityToken_;
    return appInfo;
}

void PasteboardWebController::SetPasteDataDot(PasteData &pasteData, const int32_t &userId)
{
    auto bundleName = pasteData.GetBundleName();
    HistoryInfo info{ pasteData.GetTime(), bundleName, "set", "", userId };
    SetPasteboardHistory(info);

    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData Report!");
    Reporter::GetInstance().PasteboardBehaviour().Report(
        { static_cast<int>(BehaviourPasteboardState::BPS_COPY_STATE), bundleName });

    int state = static_cast<int>(StatisticPasteboardState::SPS_COPY_STATE);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData GetTextSize!");
    size_t dataSize = pasteData.GetTextSize();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "SetPasteData timeC!");
    CalculateTimeConsuming timeC(dataSize, state);
}

void PasteboardWebController::GetPasteDataDot(PasteData &pasteData, const std::string &bundleName, const int32_t &userId)
{
    std::string remote;
    if (pasteData.IsRemote()) {
        remote = "remote";
    }
    std::string time = GetTime();
    HistoryInfo info{ time, bundleName, "get", remote, userId };
    SetPasteboardHistory(info);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "GetPasteData Report!");
    int pState = StatisticPasteboardState::SPS_INVALID_STATE;
    int bState = BehaviourPasteboardState::BPS_INVALID_STATE;
    if (pasteData.IsRemote()) {
        pState = static_cast<int>(StatisticPasteboardState::SPS_REMOTE_PASTE_STATE);
        bState = static_cast<int>(BehaviourPasteboardState::BPS_REMOTE_PASTE_STATE);
    } else {
        pState = static_cast<int>(StatisticPasteboardState::SPS_PASTE_STATE);
        bState = static_cast<int>(BehaviourPasteboardState::BPS_PASTE_STATE);
    };

    Reporter::GetInstance().PasteboardBehaviour().Report({ bState, bundleName });
    size_t dataSize = pasteData.GetTextSize();
    CalculateTimeConsuming timeC(dataSize, pState);
}

std::pair<std::shared_ptr<PasteData>, PasteDateResult> PasteboardWebController::GetDistributedData(
    const Event &event, int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    PasteDateResult pasteDateResult;
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_TASK_ERROR);
        return std::make_pair(nullptr, pasteDateResult);
    }
    std::vector<uint8_t> rawData;
    auto result = clipPlugin->GetPasteData(event, rawData);
    if (result.first != 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "get data failed");
        Reporter::GetInstance().PasteboardFault().Report({ user, "GET_REMOTE_DATA_FAILED" });
        pasteDateResult.syncTime = -1;
        pasteDateResult.errorCode = result.first;
        return std::make_pair(nullptr, pasteDateResult);
    }
    if (static_cast<int64_t>(rawData.size()) > maxLocalCapacity_.load()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "remote dataSize exceeded, dataSize=%{public}zu", rawData.size());
        pasteDateResult.syncTime = 0;
        pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::REMOTE_DATA_SIZE_EXCEEDED);
        return std::make_pair(nullptr, pasteDateResult);
    }
    currentEvent_ = std::move(event);
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->Decode(rawData);
    pasteData->SetOriginAuthority(std::make_pair(pasteData->GetBundleName(), pasteData->GetAppIndex()));
    pasteData->rawDataSize_ = static_cast<int64_t>(rawData.size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "set remote data, dataSize=%{public}" PRId64, pasteData->rawDataSize_);
    for (size_t i = 0; i < pasteData->GetRecordCount(); i++) {
        auto item = pasteData->GetRecordAt(i);
        if (item == nullptr || item->GetConvertUri().empty()) {
            continue;
        }
        if (item->GetOriginUri() == nullptr) {
            item->SetConvertUri("");
            continue;
        }
        item->isConvertUriFromRemote = true;
    }
    pasteDateResult.syncTime = result.second;
    pasteDateResult.errorCode = static_cast<int32_t>(PasteboardError::E_OK);
    return std::make_pair(pasteData, pasteDateResult);
}

bool PasteboardWebController::IsConstraintEnabled(int32_t user)
{
    bool isConstraintEnabled = false;
    ErrCode err = AccountSA::OsAccountManager::CheckOsAccountConstraintEnabled(user, CONSTRAINT, isConstraintEnabled);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(
        err == ERR_OK, false, PASTEBOARD_MODULE_SERVICE, "CheckOsAccountConstraintEnabled failed, %{public}d", err);
    return isConstraintEnabled;
}

bool PasteboardWebController::IsDisallowDistributed()
{
    pid_t uid = IPCSkeleton::GetCallingUid();
    if (uid == DEVICE_COLLABORATION_UID) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uid from device collaboration");
        return true;
    }
    return false;
}

bool PasteboardWebController::SetDistributedData(int32_t user, PasteData &data)
{
    auto networkId = DMAdapter::GetInstance().GetLocalNetworkId();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!networkId.empty(), false, PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
    Event event;
    event.user = user;
    event.seqId = ++sequenceId_;
    auto expiration = PasteBoardTime::GetBootTimeMs() + EXPIRATION_INTERVAL;
    event.expiration = static_cast<uint64_t>(expiration);
    event.deviceId = networkId;
    event.account = AccountManager::GetInstance().GetCurrentAccount();
    event.status = ClipPlugin::EVT_NORMAL;
    event.dataType = data.GetMimeTypes();
    event.isDelay = data.IsDelayRecord();
    event.dataId = data.GetDataId();
    currentEvent_ = event;

    if (IsConstraintEnabled(user) || IsDisallowDistributed()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "not allowed to send, user:%{public}d", user);
        return false;
    }
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", data.GetDataId());
        return false;
    }
    ShareOption shareOpt = data.GetShareOption();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::InApp, false, PASTEBOARD_MODULE_SERVICE,
        "data share option is in app, dataId:%{public}u", data.GetDataId());
    if (CheckMdmShareOption(data)) {
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(shareOpt != ShareOption::LocalDevice, false, PASTEBOARD_MODULE_SERVICE,
            "data share option is local device, dataId:%{public}u", data.GetDataId());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, isDelay:%{public}d,"
        "expiration:%{public}" PRIu64, event.dataId, event.seqId, event.isDelay, event.expiration);
    return SetCurrentDistributedData(data, event);
}

bool PasteboardWebController::SetCurrentDistributedData(PasteData &data, Event event)
{
    std::thread thread([this, data, event]() mutable {
        {
            std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
            setDistributedMemory_.event = event;
            setDistributedMemory_.data = std::make_shared<PasteData>(data);
            if (setDistributedMemory_.isRunning) {
                return;
            }
            setDistributedMemory_.isRunning = true;
        }
        bool isNeedCheck = false;
        while (true) {
            auto block = std::make_shared<BlockObject<bool>>(SET_DISTRIBUTED_DATA_INTERVAL, false);
            {
                std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
                if ((event.seqId == setDistributedMemory_.event.seqId && isNeedCheck) ||
                    setDistributedMemory_.data == nullptr) {
                    setDistributedMemory_.data = nullptr;
                    setDistributedMemory_.isRunning = false;
                    break;
                }
                if (!isNeedCheck) {
                    isNeedCheck = true;
                }
                std::thread thread([this, event, block]() mutable {
                    PasteData data;
                    {
                        std::lock_guard<std::mutex> lock(setDistributedMemory_.mutex);
                        if (setDistributedMemory_.data == nullptr) {
                            block->SetValue(true);
                            return;
                        }
                        event = setDistributedMemory_.event;
                        data = *setDistributedMemory_.data;
                    }
                    auto result = SetCurrentData(event, data);
                    block->SetValue(true);
                });
                thread.detach();
            }
            bool ret = block->GetValue();
            if (!ret) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "SetCurrentData timeout,seqId:%{public}hu", event.seqId);
            }
        }
    });
    thread.detach();
    return true;
}

bool PasteboardWebController::SetCurrentData(Event event, PasteData &data)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_CHECK_ONLINE_DEVICE, DFX_SUCCESS);
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clip plugin is null, dataId:%{public}u", data.GetDataId());
        return false;
    }
    RADAR_REPORT(DFX_SET_PASTEBOARD, DFX_LOAD_DISTRIBUTED_PLUGIN, DFX_SUCCESS);
    bool needFull = data.IsDelayRecord() &&
        moduleConfig_.GetRemoteDeviceMinVersion() == DistributedModuleConfig::FIRST_VERSION;
    if (needFull) {
        GetFullDelayPasteData(event.user, data);
        event.isDelay = false;
        {
            std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
            std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
            PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_);
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }
    GenerateDistributedUri(data);
    std::vector<uint8_t> rawData;
    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    {
        std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
        if (!data.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::SECOND_VERSION)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "distributed data encode failed, dataId:%{public}u, seqId:%{public}hu", event.dataId, event.seqId);
            return false;
        }
    }
    if (data.IsDelayRecord() && !needFull) {
        clipPlugin->RegisterDelayCallback(
            std::bind(&PasteboardWebController::GetDistributedDelayData, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3),
            std::bind(&PasteboardWebController::GetDistributedDelayEntry, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }
    clipPlugin->SetPasteData(event, rawData);
    return true;
}

int32_t PasteboardWebController::GetDistributedDelayEntry(const Event &evt, uint32_t recordId, const std::string &utdId,
    std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64
        ", recordId:%{public}u, type:%{public}s", evt.dataId, evt.seqId, evt.expiration, recordId, utdId.c_str());
    auto [hasData, data] = clips_.Find(evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    auto record = data->GetRecordById(recordId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID),
        PASTEBOARD_MODULE_SERVICE, "recordId=%{public}u invalid, max=%{public}zu", recordId, data->GetRecordCount());

    PasteDataEntry entry;
    entry.SetUtdId(utdId);
    int32_t ret = GetLocalEntryValue(evt.user, *data, *record, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, seqId=%{public}hu, dataId=%{public}u, recordId=%{public}u"
        ", type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_URI) {
        ret = ProcessDistributedDelayUri(evt.user, *data, entry, rawData);
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessDistributedDelayHtml(*data, entry, rawData);
    } else {
        ret = ProcessDistributedDelayEntry(entry, rawData);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "process distributed entry failed, seqId=%{public}hu, dataId=%{public}u, "
        "recordId=%{public}u, type=%{public}s, ret=%{public}d", evt.seqId, evt.dataId, recordId, utdId.c_str(), ret);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "type=%{public}s, size=%{public}zu", utdId.c_str(), rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::ProcessDistributedDelayUri(int32_t userId, PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");

    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    std::string localUri = uri->ToString();
    std::vector<std::string> localUris = { localUri };
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    int32_t ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(localUris, userId, dfsUris);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE,
        "generate distributed uri failed, uri=%{private}s", localUri.c_str());

    auto dfsUri = dfsUris.find(localUri);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(dfsUri != dfsUris.end(), static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "dfsUris is null");
    std::string distributedUri = dfsUri->second.uriStr;
    size_t fileSize = dfsUri->second.fileSize;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri: %{private}s -> %{private}s, fileSize=%{public}zu",
        localUri.c_str(), distributedUri.c_str(), fileSize);

    auto entryValue = entry.GetValue();
    if (std::holds_alternative<std::string>(entryValue)) {
        entry.SetValue(distributedUri);
    } else if (std::holds_alternative<std::shared_ptr<Object>>(entryValue)) {
        auto object = std::get<std::shared_ptr<Object>>(entryValue);
        auto newObject = std::make_shared<Object>();
        newObject->value_ = object->value_;
        newObject->value_[UDMF::FILE_URI_PARAM] = distributedUri;
        entry.SetValue(newObject);
        entry.SetFileSize(static_cast<int64_t>(fileSize));
    }

    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode uri failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::ProcessDistributedDelayHtml(PasteData &data, PasteDataEntry &entry,
    std::vector<uint8_t> &rawData)
{
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        if (PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, data.userId_)) {
            PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
            PasteboardWebController::GetInstance().CheckAppUriPermission(data);
        }
    }

    PasteData tmp;
    std::shared_ptr<std::string> html = entry.ConvertToHtml();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(html != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert to html failed");

    tmp.AddHtmlRecord(*html);
    tmp.SetBundleInfo(data.GetBundleName(), data.GetAppIndex());
    tmp.SetOriginAuthority(data.GetOriginAuthority());
    tmp.SetTokenId(data.GetTokenId());
    std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
    if (PasteboardWebController::GetInstance().SplitWebviewPasteData(tmp, bundleIndex, data.userId_)) {
        PasteboardWebController::GetInstance().SetWebviewPasteData(tmp, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(tmp);
        GenerateDistributedUri(tmp);
    }

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    bool encodeSucc = tmp.Encode(rawData, remoteVersionMin <= DistributedModuleConfig::SECOND_VERSION);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode html failed");
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::ProcessDistributedDelayEntry(PasteDataEntry &entry, std::vector<uint8_t> &rawData)
{
    bool encodeSucc = entry.Encode(rawData, true);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode entry failed, type=%{public}s", entry.GetUtdId().c_str());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::GetDistributedDelayData(const Event &evt, uint8_t version, std::vector<uint8_t> &rawData)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "dataId:%{public}u, seqId:%{public}hu, expiration:%{public}" PRIu64,
        evt.dataId, evt.seqId, evt.expiration);
    auto [hasData, data] = clips_.Find(evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", evt.user);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(evt.dataId == data->GetDataId(),
        static_cast<int32_t>(PasteboardError::INVALID_DATA_ID), PASTEBOARD_MODULE_SERVICE,
        "dataId=%{public}u mismatch, local=%{public}u", evt.dataId, data->GetDataId());

    int32_t ret = static_cast<int32_t>(PasteboardError::E_OK);
    if (version == 0) {
        ret = GetFullDelayPasteData(evt.user, *data);
    } else if (version == 1) {
        ret = GetDelayPasteRecord(evt.user, *data);
    }
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get delay data failed, version=%{public}hhu", version);

    auto authorityInfo = data->GetOriginAuthority();
    data->SetBundleInfo(authorityInfo.first, authorityInfo.second);
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(authorityInfo);
        PasteboardWebController::GetInstance().SplitWebviewPasteData(*data, bundleIndex, evt.user);
        PasteboardWebController::GetInstance().SetWebviewPasteData(*data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(*data);
    }
    GenerateDistributedUri(*data);

    auto remoteVersionMin = moduleConfig_.GetRemoteDeviceMinVersion();
    std::shared_lock<std::shared_mutex> read(pasteDataMutex_);
    bool encodeSucc = data->Encode(rawData, remoteVersionMin <= DistributedModuleConfig::SECOND_VERSION);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(encodeSucc, static_cast<int32_t>(PasteboardError::DATA_ENCODE_ERROR),
        PASTEBOARD_MODULE_SERVICE, "encode data failed, dataId:%{public}u, seqId:%{public}hu", evt.dataId, evt.seqId);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "size=%{public}zu", rawData.size());
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::GetLocalEntryValue(int32_t userId, PasteData &data, PasteDataRecord &record,
    PasteDataEntry &value)
{
    std::string utdId = value.GetUtdId();
    auto entry = record.GetEntry(utdId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(entry != nullptr, static_cast<int32_t>(PasteboardError::INVALID_MIMETYPE),
        PASTEBOARD_MODULE_SERVICE, "entry is null, recordId=%{public}u, type=%{public}s", record.GetRecordId(),
        utdId.c_str());

    std::string mimeType = entry->GetMimeType();
    value.SetMimeType(mimeType);
    if (entry->HasContent(utdId)) {
        value.SetValue(entry->GetValue());
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    int32_t ret = getter.first->GetRecordValueByType(record.GetRecordId(), value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get local entry failed, type=%{public}s, ret=%{public}d", utdId.c_str(), ret);

    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + value.rawDataSize_ < maxLocalCapacity_.load()) {
            record.AddEntry(utdId, std::make_shared<PasteDataEntry>(value));
            data.rawDataSize_ += value.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, value.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, value.rawDataSize_);
        }
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::GetRemoteEntryValue(const AppInfo &appInfo, PasteData &data, PasteDataRecord &record,
    PasteDataEntry &entry)
{
    auto clipPlugin = GetClipPlugin();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(clipPlugin != nullptr, static_cast<int32_t>(PasteboardError::PLUGIN_IS_NULL),
        PASTEBOARD_MODULE_SERVICE, "plugin is null");

    auto [distRet, distEvt] = GetValidDistributeEvent(appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(distRet == static_cast<int32_t>(PasteboardError::E_OK) ||
        distRet == static_cast<int32_t>(PasteboardError::GET_SAME_REMOTE_DATA), distRet,
        PASTEBOARD_MODULE_SERVICE, "get distribute event failed, ret=%{public}d", distRet);

    std::vector<uint8_t> rawData;
    std::string utdId = entry.GetUtdId();
    int32_t ret = clipPlugin->GetPasteDataEntry(distEvt, record.GetRecordId(), utdId, rawData);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == 0, ret, PASTEBOARD_MODULE_SERVICE, "get remote raw data failed");

    std::string mimeType = entry.GetMimeType();
    if (mimeType == MIMETYPE_TEXT_HTML) {
        ret = ProcessRemoteDelayHtml(distEvt.deviceId, appInfo, rawData, data, record, entry);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "process remote delay html failed");
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    PasteDataEntry tmpEntry;
    tmpEntry.Decode(rawData);
    entry.SetValue(tmpEntry.GetValue());
    entry.rawDataSize_ = static_cast<int64_t>(rawData.size());
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < maxLocalCapacity_.load()) {
            record.AddEntry(utdId, std::make_shared<PasteDataEntry>(entry));
            data.rawDataSize_ += entry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        }
    }

    if (mimeType != MIMETYPE_TEXT_URI) {
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    return ProcessRemoteDelayUri(distEvt.deviceId, appInfo, data, record, entry);
}

int32_t PasteboardWebController::ProcessRemoteDelayUri(const std::string &deviceId, const AppInfo &appInfo,
    PasteData &data, PasteDataRecord &record, PasteDataEntry &entry)
{
    auto uri = entry.ConvertToUri();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(uri != nullptr, static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED),
        PASTEBOARD_MODULE_SERVICE, "convert entry to uri failed");
    std::string distributedUri = uri->ToString();
    record.SetConvertUri(distributedUri);
    record.isConvertUriFromRemote = true;
    record.SetGrantUriPermission(true);

    int64_t uriFileSize = entry.GetFileSize();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "uri=%{private}s, fileSize=%{public}" PRId64,
        distributedUri.c_str(), uriFileSize);
    if (uriFileSize > 0) {
        int64_t dataFileSize = data.GetFileSize();
        int64_t fileSize = (uriFileSize > INT64_MAX - dataFileSize) ? INT64_MAX : uriFileSize + dataFileSize;
        data.SetFileSize(fileSize);
    }
    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(deviceId, data.GetPasteId());
        int32_t ret = GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant remote uri failed, uri=%{private}s, ret=%{public}d",
            distributedUri.c_str(), ret);
    }
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::ProcessRemoteDelayHtml(const std::string &remoteDeviceId, const AppInfo &appInfo,
    const std::vector<uint8_t> &rawData, PasteData &data, PasteDataRecord &record, PasteDataEntry &entry)
{
    PasteData tmpData;
    tmpData.Decode(rawData);
    auto htmlRecord = tmpData.GetRecordById(1);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(htmlRecord != nullptr,
        static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED), PASTEBOARD_MODULE_SERVICE, "record is null");
    auto htmlEntry = htmlRecord->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(htmlEntry != nullptr,
        static_cast<int32_t>(PasteboardError::GET_ENTRY_VALUE_FAILED), PASTEBOARD_MODULE_SERVICE, "htmlEntry is null");
    entry.SetValue(htmlEntry->GetValue());
    entry.rawDataSize_ = static_cast<int64_t>(rawData.size());
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        if (data.rawDataSize_ + entry.rawDataSize_ < maxLocalCapacity_.load()) {
            record.AddEntry(entry.GetUtdId(), std::make_shared<PasteDataEntry>(entry));
            data.rawDataSize_ += entry.rawDataSize_;
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add entry, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        } else {
            PASTEBOARD_HILOGW(PASTEBOARD_MODULE_SERVICE, "no space, dataSize=%{public}" PRId64
                ", entrySize=%{public}" PRId64, data.rawDataSize_, entry.rawDataSize_);
        }

        PASTEBOARD_CHECK_AND_RETURN_RET_LOGD(htmlRecord->GetFrom() != 0, static_cast<int32_t>(PasteboardError::E_OK),
            PASTEBOARD_MODULE_SERVICE, "no uri");

        data.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
        uint32_t htmlRecordId = record.GetRecordId();
        record.SetFrom(htmlRecordId);
        for (auto &recordItem : tmpData.AllRecords()) {
            if (recordItem == nullptr) {
                continue;
            }
            if (!recordItem->GetConvertUri().empty()) {
                recordItem->isConvertUriFromRemote = true;
            }
            if (recordItem->GetFrom() > 0 && recordItem->GetRecordId() != recordItem->GetFrom()) {
                recordItem->SetFrom(htmlRecordId);
                data.AddRecord(*recordItem);
            }
        }
    }
    return ProcessRemoteDelayHtmlInner(remoteDeviceId, appInfo, tmpData, data, entry);
}

int32_t PasteboardWebController::ProcessRemoteDelayHtmlInner(const std::string &remoteDeviceId, const AppInfo &appInfo,
    PasteData &tmpData, PasteData &data, PasteDataEntry &entry)
{
    int64_t htmlFileSize = tmpData.GetFileSize();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "htmlFileSize=%{public}" PRId64, htmlFileSize);
    if (htmlFileSize > 0) {
        int64_t dataFileSize = data.GetFileSize();
        int64_t fileSize = (htmlFileSize > INT64_MAX - dataFileSize) ? INT64_MAX : htmlFileSize + dataFileSize;
        data.SetFileSize(fileSize);
    }

    bool isInvalid = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(!isInvalid, static_cast<int32_t>(PasteboardError::INVALID_URI_ERROR),
        PASTEBOARD_MODULE_SERVICE, "uri invalid");

    std::map<uint32_t, std::vector<Uri>> grantUris = CheckUriPermission(
        data, std::make_pair(appInfo.bundleName, appInfo.appIndex));
    if (!grantUris.empty()) {
        EstablishP2PLink(remoteDeviceId, data.GetPasteId());
        int32_t ret = GrantUriPermission(grantUris, appInfo.bundleName, data.IsRemote(), appInfo.appIndex);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
            PASTEBOARD_MODULE_SERVICE, "grant to %{public}s failed, ret=%{public}d", appInfo.bundleName.c_str(), ret);
    }

    tmpData.SetOriginAuthority(data.GetOriginAuthority());
    tmpData.SetTokenId(data.GetTokenId());
    tmpData.SetRemote(data.IsRemote());
    SetLocalPasteFlag(tmpData.IsRemote(), appInfo.tokenId, tmpData);
    int32_t ret = PostProcessDelayHtmlEntry(tmpData, appInfo, entry);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "post process remote html failed, ret=%{public}d", ret);
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::GetFullDelayPasteData(int32_t userId, PasteData &data)
{
    auto [hasGetter, getter] = entryGetters_.Find(userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasGetter && getter.first,
        static_cast<int32_t>(PasteboardError::NO_DELAY_GETTER), PASTEBOARD_MODULE_SERVICE,
        "entry getter not find, userId=%{public}d, dataId=%{public}u", userId, data.GetDataId());

    auto delayEntryInfos = DelayManager::GetAllDelayEntryInfo(data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGI(!delayEntryInfos.empty(), static_cast<int32_t>(PasteboardError::E_OK),
        PASTEBOARD_MODULE_SERVICE, "no delay entry");
    DelayManager::GetLocalEntryValue(delayEntryInfos, getter.first, data);
    {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        std::string bundleIndex = PasteBoardCommon::GetDirByAuthority(data.GetOriginAuthority());
        PasteboardWebController::GetInstance().SplitWebviewPasteData(data, bundleIndex, userId);
        PasteboardWebController::GetInstance().SetWebviewPasteData(data, bundleIndex);
        PasteboardWebController::GetInstance().CheckAppUriPermission(data);
    }
    clips_.ComputeIfPresent(userId, [&data](auto, auto &value) {
        if (data.GetDataId() != value->GetDataId()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "set data fail, data is out time, pre dataId is %{public}d, cur dataId is %{public}d",
                data.GetDataId(), value->GetDataId());
            return true;
        }
        value = std::make_shared<PasteData>(data);
        return true;
    });
    return static_cast<int32_t>(PasteboardError::E_OK);
}

int32_t PasteboardWebController::SyncDelayedData()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto appInfo = GetAppInfo(tokenId);
    auto [hasData, data] = clips_.Find(appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(hasData && data, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "data not find, userId=%{public}u", appInfo.userId);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(tokenId == data->GetTokenId(),
        static_cast<int32_t>(PasteboardError::INVALID_TOKEN_ID), PASTEBOARD_MODULE_SERVICE,
        "tokenId=%{public}u mismatch, local=%{public}u", tokenId, data->GetTokenId());

    int32_t ret = GetFullDelayPasteData(appInfo.userId, *data);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(ret == static_cast<int32_t>(PasteboardError::E_OK), ret,
        PASTEBOARD_MODULE_SERVICE, "get full delay failed, ret=%{public}d", ret);

    std::thread thread([=, userId = appInfo.userId, data = data] {
        std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(data != nullptr, PASTEBOARD_MODULE_SERVICE, "sync delayed data is null");
        data->RemoveEmptyEntry();
        clips_.ComputeIfPresent(userId, [=](auto, auto &value) {
            if (data->GetDataId() == value->GetDataId()) {
                value = std::move(data);
            }
            return true;
        });
    });
    thread.detach();
    return ERR_OK;
}

void PasteboardWebController::GenerateDistributedUri(PasteData &data)
{
    std::vector<std::string> uris;
    std::vector<size_t> indexes;
    auto userId = GetCurrentAccountId();
    PASTEBOARD_CHECK_AND_RETURN_LOGE(userId != ERROR_USERID, PASTEBOARD_MODULE_SERVICE, "invalid userId");
    std::unique_lock<std::shared_mutex> write(pasteDataMutex_);
    for (size_t i = 0; i < data.GetRecordCount(); i++) {
        auto item = data.GetRecordAt(i);
        if (item == nullptr) {
            continue;
        }
        item->SetConvertUri("");
        const auto &uri = item->GetOriginUri();
        if (uri == nullptr) {
            continue;
        }
        auto hasGrantUriPermission = item->HasGrantUriPermission();
        const std::string &bundleName = data.GetOriginAuthority().first;
        if (!IsBundleOwnUriPermission(bundleName, *uri) && !hasGrantUriPermission) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "uri:%{private}s, bundleName:%{public}s, appIndex:%{public}d,"
                " has grant:%{public}d", uri->ToString().c_str(), bundleName.c_str(), data.GetOriginAuthority().second,
                hasGrantUriPermission);
            continue;
        }
        uris.emplace_back(uri->ToString());
        indexes.emplace_back(i);
    }
    size_t fileSize = 0;
    std::unordered_map<std::string, HmdfsUriInfo> dfsUris;
    if (!uris.empty()) {
        int ret = Storage::DistributedFile::FileMountManager::GetDfsUrisDirFromLocal(uris, userId, dfsUris);
        if (ret != 0 || dfsUris.empty()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
                "Get remoteUri failed, ret:%{public}d, userId:%{public}d, uri size:%{public}zu.",
                ret, userId, uris.size());
        }
        for (size_t i = 0; i < indexes.size(); i++) {
            auto item = data.GetRecordAt(indexes[i]);
            if (item == nullptr || item->GetOriginUri() == nullptr) {
                continue;
            }
            auto it = dfsUris.find(item->GetOriginUri()->ToString());
            if (it != dfsUris.end()) {
                item->SetConvertUri(it->second.uriStr);
                fileSize += it->second.fileSize;
            } else {
                item->SetConvertUri(" ");
            }
        }
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "file size: %{public}zu", fileSize);
    data.SetFileSize(static_cast<int64_t>(fileSize));
}

std::shared_ptr<ClipPlugin> PasteboardWebController::GetClipPlugin()
{
    auto isOn = moduleConfig_.IsOn();
    if (isOn) {
        auto securityLevel = securityLevel_.GetDeviceSecurityLevel();
#ifdef PB_DATACLASSIFICATION_ENABLE
        if (securityLevel < DATA_SEC_LEVEL3) {
            return nullptr;
        }
#endif
    }
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    if (!isOn || clipPlugin_ != nullptr) {
        return clipPlugin_;
    }
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    RegisterPreSyncCallback(clipPlugin_);
    return clipPlugin_;
}

void PasteboardWebController::CleanDistributedData(int32_t user)
{
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->Clear(user);
}

void PasteboardWebController::CloseDistributedStore(int32_t user, bool isNeedClear)
{
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
    if (isNeedClear) {
        clipPlugin_->Clear(user);
    }
    clipPlugin_->Close(user);
}

void PasteboardWebController::OnConfigChange(bool isOn)
{
    std::thread thread([=]() {
        OnConfigChangeInner(isOn);
    });
    thread.detach();
}

void PasteboardWebController::OnConfigChangeInner(bool isOn)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConfigChange isOn: %{public}d.", isOn);
    if (!isOn) {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.ForEach([this](const auto &deviceId, auto &value) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "configChange is off, need close p2p link.");
            CloseP2PLink(deviceId);
            return false;
        });
        p2pMap_.Clear();
    }
    std::lock_guard<decltype(mutex)> lockGuard(mutex);
    if (!isOn) {
        PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        int32_t userId = GetCurrentAccountId();
        clipPlugin_->Close(userId);
        clipPlugin_ = nullptr;
        return;
    }
    SetCriticalTimer();
    auto securityLevel = securityLevel_.GetDeviceSecurityLevel();
#ifdef PB_DATACLASSIFICATION_ENABLE
    if (securityLevel < DATA_SEC_LEVEL3) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "device sec level is %{public}u less than 3.", securityLevel);
        return;
    }
#endif
    if (clipPlugin_ != nullptr) {
        return;
    }
    SubscribeKeyboardEvent();
    Loader loader;
    loader.LoadComponents();
    auto release = [this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME, plugin);
    };

    clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME), release);
    RegisterPreSyncCallback(clipPlugin_);
}

std::string PasteboardWebController::GetAppLabel(uint32_t tokenId)
{
    auto iBundleMgr = GetAppBundleManager();
    if (iBundleMgr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to cast bundle mgr service.");
        return PasteboardDialog::DEFAULT_LABEL;
    }
    AppInfo info = GetAppInfo(tokenId);
    AppExecFwk::ApplicationInfo appInfo;
    auto result = iBundleMgr->GetApplicationInfo(info.bundleName, 0, info.userId, appInfo);
    if (!result) {
        return PasteboardDialog::DEFAULT_LABEL;
    }
    auto &resource = appInfo.labelResource;
    auto label = iBundleMgr->GetStringById(resource.bundleName, resource.moduleName, resource.id, info.userId);
    return label.empty() ? PasteboardDialog::DEFAULT_LABEL : label;
}

sptr<AppExecFwk::IBundleMgr> PasteboardWebController::GetAppBundleManager()
{
    auto systemAbilityManager = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to get SystemAbilityManager.");
        return nullptr;
    }
    auto remoteObject = systemAbilityManager->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, " Failed to get bundle mgr service.");
        return nullptr;
    }
    return OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
}

void PasteboardWebController::ChangeStoreStatus(int32_t userId)
{
    PasteboardWebController::currentUserId_ = userId;
    auto clipPlugin = GetClipPlugin();
    if (clipPlugin == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "clipPlugin null.");
        return;
    }
    clipPlugin->ChangeStoreStatus(userId);
}

void PasteBoardCommonEventSubscriber::OnReceiveEvent(const EventFwk::CommonEventData &data)
{
    std::thread thread([=] {
        OnReceiveEventInner(data);
    });
    thread.detach();
}

void PasteBoardCommonEventSubscriber::OnReceiveEventInner(const EventFwk::CommonEventData &data)
{
    auto want = data.GetWant();
    std::string action = want.GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED) {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t userId = data.GetCode();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id switched: %{public}d", userId);
        if (pasteboardService_ != nullptr) {
            pasteboardService_->ChangeStoreStatus(userId);
            auto accountId = pasteboardService_->GetCurrentAccountId();
            pasteboardService_->switch_.DeInit();
            pasteboardService_->switch_.Init(accountId);
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "SetSwitch end");
        }
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING) {
        std::lock_guard<std::mutex> lock(mutex_);
        int32_t userId = data.GetCode();
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "user id is stopping: %{public}d", userId);
        if (pasteboardService_ != nullptr) {
            pasteboardService_->Clear();
        }
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED) {
        std::lock_guard<std::mutex> lock(mutex_);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen is locked");
        PasteboardWebController::currentScreenStatus = ScreenEvent::ScreenLocked;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED) {
        std::lock_guard<std::mutex> lock(mutex_);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "screen is unlocked");
        PasteboardWebController::currentScreenStatus = ScreenEvent::ScreenUnlocked;
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED) {
        auto tokenId = want.GetIntParam("accessTokenId", -1);
        if (pasteboardService_ != nullptr) {
            pasteboardService_->ClearUriOnUninstall(tokenId);
        }
    }
}

void PasteboardWebController::ClearUriOnUninstall(int32_t tokenId)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(tokenId >= 0, PASTEBOARD_MODULE_SERVICE, "tokenId is invalids");
    auto userId = GetCurrentAccountId();
    clips_.ComputeIfPresent(userId, [this, tokenId, userId](auto, auto &pasteData) {
        if (pasteData == nullptr) {
            return true;
        }
        if (pasteData->GetTokenId() != static_cast<uint32_t>(tokenId)) {
            return true;
        }
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "clear uri, tokenId=%{public}d", tokenId);
        ClearUriOnUninstall(pasteData);
        delayGetters_.ComputeIfPresent(userId, [](auto, auto &delayGetter) {
            if (delayGetter.first != nullptr && delayGetter.second != nullptr) {
                delayGetter.first->AsObject()->RemoveDeathRecipient(delayGetter.second);
            }
            return false;
        });
        entryGetters_.ComputeIfPresent(userId, [](auto, auto &entryGetter) {
            if (entryGetter.first != nullptr && entryGetter.second != nullptr) {
                entryGetter.first->AsObject()->RemoveDeathRecipient(entryGetter.second);
            }
            return false;
        });
        return true;
    });
}

void PasteboardWebController::ClearUriOnUninstall(std::shared_ptr<PasteData> pasteData)
{
    PASTEBOARD_CHECK_AND_RETURN_LOGE(pasteData != nullptr, PASTEBOARD_MODULE_SERVICE, "pasteData is null");
    std::thread thread([pasteData, this]() {
        {
            std::unique_lock<std::shared_mutex> threadWriteLock(pasteDataMutex_);
            if (!pasteData->HasMimeType(MIMETYPE_TEXT_URI)) {
                return;
            }

            auto emptyUri = std::make_shared<OHOS::Uri>("");
            size_t recordCount = pasteData->GetRecordCount();
            for (size_t i = 0; i < recordCount; i++) {
                auto item = pasteData->GetRecordAt(i);
                if (item == nullptr || item->GetOriginUri() == nullptr) {
                    continue;
                }
                item->SetUri(emptyUri);
            }
        }

        std::lock_guard<decltype(mutex)> lockGuard(mutex);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(clipPlugin_ != nullptr, PASTEBOARD_MODULE_SERVICE, "clipPlugin is null");
        clipPlugin_->Clear(pasteData->userId_);
    });
    thread.detach();
}

void PasteBoardAccountStateSubscriber::OnStateChanged(const AccountSA::OsAccountStateData &data)
{
    std::thread thread([=]() {
        OnStateChangedInner(data);
    });
    thread.detach();
}

void PasteBoardAccountStateSubscriber::OnStateChangedInner(const AccountSA::OsAccountStateData &data)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "state: %{public}d, fromId: %{public}d, toId: %{public}d,"
        "callback is nullptr: %{public}d", data.state, data.fromId, data.toId, data.callback == nullptr);
    if (data.state == AccountSA::OsAccountState::STOPPING && pasteboardService_ != nullptr) {
        pasteboardService_->CloseDistributedStore(data.fromId, true);
    }
    if (data.callback != nullptr) {
        data.callback->OnComplete();
    }
}

bool PasteboardWebController::SubscribeKeyboardEvent()
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    if (inputEventCallback_ != nullptr) {
        return true;
    }
    inputEventCallback_ = std::make_shared<InputEventCallback>();
    int32_t monitorId = MMI::InputManager::GetInstance()->AddMonitor(
        std::static_pointer_cast<MMI::IInputEventConsumer>(inputEventCallback_), MMI::HANDLE_EVENT_TYPE_KEY);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "add monitor ret is: %{public}d", monitorId);
    return monitorId >= 0;
}

void PasteboardWebController::PasteboardEventSubscriber()
{
    EventCenter::GetInstance().Subscribe(PasteboardEvent::DISCONNECT, [this](const OHOS::MiscServices::Event &event) {
        auto &evt = static_cast<const PasteboardEvent &>(event);
        auto networkId = evt.GetNetworkId();
        if (networkId.empty()) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "networkId is empty.");
            return;
        }
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.EraseIf([networkId, this](auto &key, auto &value) {
            if (key == networkId) {
                CloseP2PLink(networkId);
                return true;
            }
            return false;
        });
    });
}

void PasteboardWebController::CommonEventSubscriber()
{
    if (commonEventSubscriber_ != nullptr) {
        return;
    }
    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_USER_STOPPING);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    commonEventSubscriber_ = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, this);
    EventFwk::CommonEventManager::SubscribeCommonEvent(commonEventSubscriber_);
}

void PasteboardWebController::AccountStateSubscriber()
{
    if (accountStateSubscriber_ != nullptr) {
        return;
    }
    std::set<AccountSA::OsAccountState> states = { AccountSA::OsAccountState::STOPPING,
        AccountSA::OsAccountState::CREATED, AccountSA::OsAccountState::SWITCHING,
        AccountSA::OsAccountState::SWITCHED, AccountSA::OsAccountState::UNLOCKED,
        AccountSA::OsAccountState::STOPPED, AccountSA::OsAccountState::REMOVED };
    AccountSA::OsAccountSubscribeInfo subscribeInfo(states, true);
    accountStateSubscriber_ = std::make_shared<PasteBoardAccountStateSubscriber>(subscribeInfo, this);
    AccountSA::OsAccountManager::SubscribeOsAccount(accountStateSubscriber_);
}

void PasteboardWebController::RemoveObserverByPid(int32_t userId, pid_t pid, ObserverMap &observerMap)
{
    std::lock_guard<std::mutex> lock(observerMutex_);
    auto callObserverKey = std::make_pair(userId, pid);
    auto it = observerMap.find(callObserverKey);
    if (it == observerMap.end()) {
        return;
    }
    observerMap.erase(callObserverKey);
}

int32_t PasteboardWebController::AppExit(pid_t pid)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid %{public}d exit.", pid);
    int32_t userId = GetCurrentAccountId();
    RemoveObserverByPid(userId, pid, observerLocalChangedMap_);
    RemoveObserverByPid(userId, pid, observerRemoteChangedMap_);
    RemoveObserverByPid(COMMON_USERID, pid, observerEventMap_);
    entityObserverMap_.Erase(pid);
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);
    ClearInputMethodPidByPid(userId, pid);
    std::vector<std::string> networkIds;
    {
        std::lock_guard<std::mutex> tmpMutex(p2pMapMutex_);
        p2pMap_.EraseIf([pid, &networkIds, this](auto &networkId, auto &pidMap) {
            pidMap.EraseIf([pid, this](const auto &key, const auto &value) {
                if (value.callPid == pid) {
                    PasteStart(key);
                    return true;
                }
                return false;
            });
            if (pidMap.Empty()) {
                networkIds.emplace_back(networkId);
                return true;
            }
            return false;
        });
    }
    for (const auto &id : networkIds) {
        CloseP2PLink(id);
    }
    bool isExist = clients_.ComputeIfPresent(pid, [pid](auto, auto &value) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "find client death recipient succeed, pid=%{public}d", pid);
        PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(value.first != nullptr && value.second != nullptr, false,
            PASTEBOARD_MODULE_SERVICE, "client death recipient is null");
        value.first->RemoveDeathRecipient(value.second);
        return false;
    });
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(isExist, static_cast<int32_t>(PasteboardError::NO_DATA_ERROR),
        PASTEBOARD_MODULE_SERVICE, "find client death recipient failed, pid=%{public}d", pid);
    return ERR_OK;
}

void PasteboardWebController::PasteboardDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    (void)remote;
    service_.AppExit(pid_);
}

PasteboardWebController::PasteboardDeathRecipient::PasteboardDeathRecipient(
    PasteboardService &service, pid_t pid) : service_(service), pid_(pid)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Construct Pasteboard Client Death Recipient, pid: %{public}d", pid);
}

int32_t PasteboardWebController::RegisterClientDeathObserver(const sptr<IRemoteObject> &observer)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    sptr<PasteboardDeathRecipient> deathRecipient = sptr<PasteboardDeathRecipient>::MakeSptr(*this, pid);
    observer->AddDeathRecipient(deathRecipient);
    clients_.InsertOrAssign(pid, std::make_pair(observer, deathRecipient));
    return ERR_OK;
}

int32_t PasteboardWebController::DetachPasteboard()
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    return AppExit(pid);
}

std::function<void(const OHOS::MiscServices::Event &)> PasteboardWebController::RemotePasteboardChange()
{
    return [this](const OHOS::MiscServices::Event &event) {
        (void)event;
        std::lock_guard<std::mutex> lock(observerMutex_);
        for (auto &observers : observerRemoteChangedMap_) {
            for (const auto &observer : *(observers.second)) {
                observer->OnPasteboardChanged();
            }
        }
    };
}

int32_t PasteboardWebController::CallbackEnter(uint32_t code)
{
    if (!IPCSkeleton::IsLocalCalling()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid request, only support local, cmd:%{public}u", code);
        return ERR_TRANSACTION_FAILED;
    }
    if (code == static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA)) {
        return ERR_NONE;
    }
    pid_t pid = IPCSkeleton::GetCallingPid();
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid:%{public}d, uid:%{public}d, cmd:%{public}u", pid, uid, code);
    return ERR_NONE;
}

int32_t PasteboardWebController::CallbackExit(uint32_t code, int32_t result)
{
    if (code == static_cast<uint32_t>(IPasteboardServiceIpcCode::COMMAND_HAS_PASTE_DATA)) {
        return ERR_NONE;
    }
    pid_t pid = IPCSkeleton::GetCallingPid();
    pid_t uid = IPCSkeleton::GetCallingUid();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "pid:%{public}d, uid:%{public}d, cmd:%{public}u, ret:%{public}d",
        pid, uid, code, result);
    return ERR_NONE;
}

void InputEventCallback::OnKeyInputEventForPaste(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
    auto keyItems = keyEvent->GetKeyItems();
    if (keyItems.size() != CTRLV_EVENT_SIZE) {
        return;
    }
    if ((keyEvent->GetKeyAction() == MMI::KeyEvent::KEY_ACTION_DOWN) &&
        ((keyItems[0].GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_LEFT) ||
        (keyItems[0].GetKeyCode() == MMI::KeyEvent::KEYCODE_CTRL_RIGHT)) &&
        keyItems[1].GetKeyCode() == MMI::KeyEvent::KEYCODE_V) {
        int32_t windowId = keyEvent->GetTargetWindowId();
        std::unique_lock<std::shared_mutex> lock(inputEventMutex_);
        windowPid_ = MMI::InputManager::GetInstance()->GetWindowPid(windowId);
        actionTime_ =
            static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
        std::shared_ptr<BlockObject<int32_t>> block = nullptr;
        {
            std::unique_lock<std::shared_mutex> blockMapLock(blockMapMutex_);
            auto it = blockMap_.find(windowPid_);
            if (it != blockMap_.end()) {
                block = it->second;
            } else {
                block = std::make_shared<BlockObject<int32_t>>(WAIT_TIME_OUT, 0);
                blockMap_.insert(std::make_pair(windowPid_, block));
            }
        }
        if (block != nullptr) {
            block->SetValue(SET_VALUE_SUCCESS);
        }
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::KeyEvent> keyEvent) const
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "keyEvent, inputType_ = %{public}d", inputType_);
    if (inputType_ == INPUTTYPE_PASTE) {
        OnKeyInputEventForPaste(keyEvent);
    } else if (inputType_ == INPUTTYPE_PRESYNC) {
        if (pasteboardService_) {
            pasteboardService_->PreSyncRemotePasteboardData();
        }
    } else {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "invalid inputType_ = %{public}d", inputType_);
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::PointerEvent> pointerEvent) const
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "pointerEvent, inputType_ = %{public}d", inputType_);
    if (inputType_ == INPUTTYPE_PRESYNC) {
        if (pasteboardService_) {
            pasteboardService_->PreSyncRemotePasteboardData();
        }
    }
}

void InputEventCallback::OnInputEvent(std::shared_ptr<MMI::AxisEvent> axisEvent) const {}

bool InputEventCallback::IsCtrlVProcess(uint32_t callingPid, bool isFocused)
{
    std::shared_ptr<BlockObject<int32_t>> block = nullptr;
    {
        std::unique_lock<std::shared_mutex> blockMapLock(blockMapMutex_);
        auto it = blockMap_.find(callingPid);
        if (it != blockMap_.end()) {
            block = it->second;
        } else {
            block = std::make_shared<BlockObject<int32_t>>(WAIT_TIME_OUT, 0);
            blockMap_.insert(std::make_pair(callingPid, block));
        }
    }
    if (block != nullptr) {
        block->GetValue();
    }
    std::shared_lock<std::shared_mutex> lock(inputEventMutex_);
    auto curTime = static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    auto ret = (callingPid == static_cast<uint32_t>(windowPid_) || isFocused) && curTime >= actionTime_ &&
        curTime - actionTime_ < EVENT_TIME_OUT;
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "windowPid is: %{public}d, callingPid is: %{public}d,"
            "curTime is: %{public}" PRIu64 ", actionTime is: %{public}" PRIu64 ", isFocused is: %{public}d",
            windowPid_, callingPid, curTime, actionTime_, isFocused);
    }
    return ret;
}

void InputEventCallback::Clear()
{
    std::unique_lock<std::shared_mutex> lock(inputEventMutex_);
    actionTime_ = 0;
    windowPid_ = 0;
    std::unique_lock<std::shared_mutex> blockMapLock(blockMapMutex_);
    blockMap_.clear();
}
} // namespace MiscServices
} // namespace OHOS
