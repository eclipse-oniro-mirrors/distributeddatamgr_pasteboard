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

#include "pasteboard_img_extractor.h"

#include <algorithm>
#include <memory>

#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

PasteboardImgExtractor &PasteboardImgExtractor::GetInstance()
{
    static PasteboardImgExtractor instance;
    return instance;
}

PasteboardImgExtractor::PasteboardImgExtractor()
{
    std::lock_guard lock(mutex_);
    xmlInitParser();
}

PasteboardImgExtractor::~PasteboardImgExtractor()
{
    std::lock_guard lock(mutex_);
    xmlCleanupParser();
}

std::vector<std::string> PasteboardImgExtractor::ExtractImgSrc(const std::string &htmlContent)
{
    std::lock_guard lock(mutex_);
    int options = HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING;
    xmlDocPtr doc = htmlReadDoc(reinterpret_cast<const xmlChar *>(htmlContent.c_str()), nullptr, nullptr, options);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(doc != nullptr, {}, PASTEBOARD_MODULE_COMMON, "parse html failed");

    std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> docGuard(doc, xmlFreeDoc);
    auto uris = FindImgsExcludingSpan(doc);
    FilterFileUris(uris);
    return uris;
}

void PasteboardImgExtractor::FilterFileUris(std::vector<std::string> &uris)
{
    uris.erase(
        std::remove_if(uris.begin(), uris.end(),
            [](const std::string &str) {
                return str.find("file:///") != 0;
            }),
        uris.end()
    );
}

std::vector<std::string> PasteboardImgExtractor::FindImgsExcludingSpan(xmlDocPtr doc)
{
    return ExecuteXPath(doc, "//img[@src and not(ancestor::span)]");
}

std::vector<std::string> PasteboardImgExtractor::ExecuteXPath(xmlDocPtr doc, const char *xpathExpr)
{
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(context != nullptr, {}, PASTEBOARD_MODULE_COMMON, "xpath new context failed");

    std::unique_ptr<xmlXPathContext, decltype(&xmlXPathFreeContext)> contextGuard(context, xmlXPathFreeContext);
    xmlXPathObjectPtr result = xmlXPathEvalExpression(reinterpret_cast<const xmlChar *>(xpathExpr), context);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(result != nullptr, {}, PASTEBOARD_MODULE_COMMON, "xpath eval expr failed");

    std::unique_ptr<xmlXPathObject, decltype(&xmlXPathFreeObject)> resultGuard(result, xmlXPathFreeObject);
    if (!result->nodesetval) {
        return {};
    }

    std::vector<std::string> results;
    xmlNodeSetPtr nodeSet = result->nodesetval;
    for (size_t i = 0; i < nodeSet->nodeNr; ++i) {
        xmlNodePtr node = nodeSet->nodeTab[i];
        if (node == nullptr) {
            continue;
        }

        xmlChar *src = xmlGetProp(node, reinterpret_cast<const xmlChar *>("src"));
        if (src == nullptr) {
            continue;
        }

        std::string srcStr = SafeXmlToString(src);
        if (!srcStr.empty()) {
            results.push_back(srcStr);
        }
        xmlFree(src);
    }
    return results;
}

std::string PasteboardImgExtractor::SafeXmlToString(const xmlChar *xmlStr)
{
    if (xmlStr == nullptr) {
        return "";
    }

    std::string result(reinterpret_cast<const char *>(xmlStr));
    return result;
}
} // namespace MiscServices
} // namespace OHOS