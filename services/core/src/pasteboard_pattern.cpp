/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include <cstdlib>
#include <dlfcn.h>
#include <libxml/HTMLparser.h>
#include <regex>

#include "pasteboard_hilog.h"
#include "pasteboard_pattern.h"

namespace OHOS::MiscServices {
std::map<uint32_t, std::string> PatternDetection::patterns_{
    { static_cast<uint32_t>(Pattern::URL), std::string("[a-zA-Z0-9+.-]+://[-a-zA-Z0-9+&@#/%?"
                                                       "=~_|!:,.;]*[-a-zA-Z0-9+&@#/%=~_]") },
    { static_cast<uint32_t>(Pattern::NUMBER), std::string("[-+]?[0-9]*\\.?[0-9]+") },
    { static_cast<uint32_t>(Pattern::EMAIL_ADDRESS), std::string("(([a-zA-Z0-9_\\-\\.\\%\\+]+)@"
                                                                "(([a-zA-Z0-9\\-]+(?:\\.[a-zA-Z0-9\\-]+)*)|"
                                                                "(?:\\[([0-9]{1,3}\\.){3}[0-9]{1,3}\\]))"
                                                                "([a-zA-Z]{1,}|[0-9]{1,3}))") },
};

const std::set<Pattern> PatternDetection::Detect(
    const std::set<Pattern> &patternsToCheck, const PasteData &pasteData, bool hasHTML, bool hasPlain)
{
    std::set<Pattern> existedPatterns;
    for (auto &record : pasteData.AllRecords()) {
        if (patternsToCheck == existedPatterns) {
            break;
        }
        if (hasPlain && record->GetPlainTextV0() != nullptr) {
            std::string recordText = *(record->GetPlainTextV0());
            DetectPlainText(existedPatterns, patternsToCheck, recordText);
        }
        if (hasHTML && record->GetHtmlTextV0() != nullptr) {
            std::string recordText = ExtractHtmlContent(*(record->GetHtmlTextV0()));
            DetectPlainText(existedPatterns, patternsToCheck, recordText);
        }
    }
    return existedPatterns;
}

void PatternDetection::DetectPlainText(
    std::set<Pattern> &patternsOut, const std::set<Pattern> &patternsIn, const std::string &plainText)
{
    for (Pattern pattern : patternsIn) {
        if (patternsOut.find(pattern) != patternsOut.end()) {
            continue;
        }
        uint32_t patternUint32 = static_cast<uint32_t>(pattern);
        auto it = patterns_.find(patternUint32);
        if (it == patterns_.end()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "pasteboard pattern, unexpected Pattern value!");
            continue;
        }
        std::regex curRegex(it->second);
        if (std::regex_search(plainText, curRegex)) {
            patternsOut.insert(pattern);
        }
    }
}

std::string PatternDetection::ExtractHtmlContent(const std::string &html_str)
{
    xmlDocPtr doc = htmlReadMemory(html_str.c_str(), html_str.size(), nullptr, nullptr, 0);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(doc != nullptr, "", PASTEBOARD_MODULE_SERVICE,
        "parse html failed, doc is null");
    xmlNode *rootNode = xmlDocGetRootElement(doc);
    if (rootNode == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "parse html failed, rootNode is null");
        xmlFreeDoc(doc);
        return "";
    }
    xmlChar *xmlStr = xmlNodeGetContent(rootNode);
    if (xmlStr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "parse html failed, xmlStr is null");
        xmlFreeDoc(doc);
        return "";
    }

    std::string result(reinterpret_cast<const char *>(xmlStr));
    free(xmlStr);
    xmlFreeDoc(doc);
    return result;
}
} // namespace OHOS::MiscServices