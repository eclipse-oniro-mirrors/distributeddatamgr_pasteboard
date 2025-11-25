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

#ifndef PASTEBOARD_IMG_EXTRACTOR_H
#define PASTEBOARD_IMG_EXTRACTOR_H

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <mutex>
#include <string>
#include <vector>

namespace OHOS {
namespace MiscServices {
class PasteboardImgExtractor {
public:
    static PasteboardImgExtractor &GetInstance();
    std::vector<std::string> ExtractImgSrc(const std::string &htmlContent, const std::string &bundleIndex,
        int32_t userId);

    static constexpr const char *IMG_LOCAL_URI = "file:///";
    static constexpr const char *FILE_SCHEME_PREFIX = "file://";
    static constexpr const char *DOC_LOCAL_URI = "file:///docs/";
    static constexpr const char *DOC_URI_PREFIX = "file://docs/";

private:
    PasteboardImgExtractor();
    ~PasteboardImgExtractor();
    static void FilterFileUris(std::vector<std::string> &uris);
    static void FilterImgUris(std::vector<std::string> &uris);
    static void FilterExistFileUris(std::vector<std::string> &uris, const std::string &bundleIndex, int32_t userId);
    static bool MatchImgExtension(const std::string &uri);
    std::vector<std::string> FindImgsExcludingSpan(xmlDocPtr doc);
    std::vector<std::string> ExecuteXPath(xmlDocPtr doc, const char *xpathExpr);
    std::string SafeXmlToString(const xmlChar *xmlStr);

    std::mutex mutex_;
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_IMG_EXTRACTOR_H
