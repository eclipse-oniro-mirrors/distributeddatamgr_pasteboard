/*
 * Copyright (C) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_WEB_CONTROLLER_H
#define PASTEBOARD_WEB_CONTROLLER_H

#include "paste_data.h"

namespace OHOS {
namespace MiscServices {

class API_EXPORT PasteboardWebController : public RefBase {
public:
    PasteboardWebController() = default;
    virtual ~PasteboardWebController() = default;

    static PasteboardWebController &GetInstance();
    bool SplitWebviewPasteData(PasteData &pasteData);
    void SetWebviewPasteData(PasteData &pasteData, const std::pair<std::string, int32_t> &bundleIndex);
    void CheckAppUriPermission(PasteData &pasteData);
    void RetainUri(PasteData &pasteData);
    void RebuildWebviewPasteData(PasteData &pasteData, const std::string &targetBundle = "",
        int32_t appIndex = 0);

private:
    void RefreshUri(std::shared_ptr<PasteDataRecord> &record, const std::string &targetBundle, int32_t appInedx);
    std::shared_ptr<std::string> RebuildHtml(std::shared_ptr<PasteData> pasteData) noexcept;
    std::vector<std::shared_ptr<PasteDataRecord>> SplitHtml2Records(const std::shared_ptr<std::string> &html,
        uint32_t recordId) noexcept;
    void MergeExtraUris2Html(PasteData &data);
    std::vector<std::pair<std::string, uint32_t>> SplitHtmlWithImgLabel(
        const std::shared_ptr<std::string> html) noexcept;
    std::map<std::string, std::vector<uint8_t>> SplitHtmlWithImgSrcLabel(
        const std::vector<std::pair<std::string, uint32_t>> &matchVec) noexcept;
    std::vector<std::shared_ptr<PasteDataRecord>> BuildPasteDataRecords(const std::map<std::string,
        std::vector<uint8_t>> &imgSrcMap, uint32_t recordId) noexcept;

    void RemoveAllRecord(std::shared_ptr<PasteData> pasteData) noexcept;
    void RemoveRecordById(PasteData &pasteData, uint32_t recordId) noexcept;
    bool IsLocalURI(std::string &uri) noexcept;
    std::map<std::uint32_t, std::vector<std::shared_ptr<PasteDataRecord>>> GroupRecordWithFrom(PasteData &data);

    void RemoveExtraUris(PasteData &data);
    void ReplaceHtmlRecordContentByExtraUris(std::vector<std::shared_ptr<PasteDataRecord>> &records);
    void UpdateHtmlRecord(std::shared_ptr<PasteDataRecord> &htmlRecord, std::shared_ptr<std::string> &htmlData);
    int32_t GetNeedCheckUris(PasteData &pasteData, std::vector<std::string> &uris,
        std::vector<size_t> &indexs, bool ancoFlag);
    void SetUriPermission(std::shared_ptr<PasteDataRecord> &record, bool isRead, bool isWrite, bool isNeedPersistance);
};
} // namespace MiscServices
} // namespace OHOS

#endif // PASTEBOARD_WEB_CONTROLLER_H
