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
#include <cerrno>
#include <cstring>
#include <memory>
#include <sys/stat.h>
#include <unordered_set>

#include "ipc_skeleton.h"
#include "pasteboard_hilog.h"
#include "sandbox_helper.h"

namespace OHOS {
namespace MiscServices {
constexpr uid_t HWF_SERVICE_UID = 7700;

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

std::vector<std::string> PasteboardImgExtractor::ExtractImgSrc(const std::string &htmlContent,
    const std::string &bundleIndex, int32_t userId)
{
    std::lock_guard lock(mutex_);
    int options = HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING;
    xmlDocPtr doc = htmlReadDoc(reinterpret_cast<const xmlChar *>(htmlContent.c_str()), nullptr, nullptr, options);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(doc != nullptr, {}, PASTEBOARD_MODULE_COMMON, "parse html failed");

    std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> docGuard(doc, xmlFreeDoc);
    auto uris = FindImgsExcludingSpan(doc);
    FilterFileUris(uris);
    FilterImgUris(uris);
    FilterExistFileUris(uris, bundleIndex, userId);
    return uris;
}

void PasteboardImgExtractor::FilterExistFileUris(std::vector<std::string> &uris, const std::string &bundleIndex,
    int32_t userId)
{
    std::vector<std::string> existFileUris;
    std::string userIdStr = std::to_string(userId);
    uid_t callingUid = IPCSkeleton::GetCallingUid();
    for (const std::string &uriStr : uris) {
        if (!AppFileService::SandboxHelper::IsValidPath(uriStr)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "uri path invalid, uri=%{private}s", uriStr.c_str());
            continue;
        }
        std::string oldUriStr = uriStr;
        std::string newUriStr;
        if (oldUriStr.find(PasteboardImgExtractor::IMG_LOCAL_URI) != 0) {
            continue;
        } else if (oldUriStr.find(PasteboardImgExtractor::DOC_LOCAL_URI) == 0) {
            newUriStr = oldUriStr.replace(0, std::strlen(PasteboardImgExtractor::DOC_LOCAL_URI),
                PasteboardImgExtractor::DOC_URI_PREFIX);
        } else {
            newUriStr = oldUriStr.replace(0, std::strlen(PasteboardImgExtractor::IMG_LOCAL_URI),
                PasteboardImgExtractor::FILE_SCHEME_PREFIX + bundleIndex + "/");
        }

        std::string physicalPath;
        if (callingUid == HWF_SERVICE_UID && oldUriStr.find("file://docs/storage/Users/currentUser/") == 0) {
            physicalPath = oldUriStr.replace(0, std::strlen("file://docs/storage/Users/currentUser/"), "/mnt/");
        } else {
            int32_t ret = AppFileService::SandboxHelper::GetPhysicalPath(newUriStr, userIdStr, physicalPath);
            if (ret != 0) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "get phy path fail, uri=%{private}s", newUriStr.c_str());
                continue;
            }
        }

        errno = 0;
        struct stat buf = {};
        if (stat(physicalPath.c_str(), &buf) != 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "stat fail, uri=%{private}s, path=%{private}s, err=%{public}d",
                newUriStr.c_str(), physicalPath.c_str(), errno);
            continue;
        }

        if ((buf.st_mode & S_IFMT) == S_IFDIR) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_COMMON, "is dir, uri=%{private}s, path=%{private}s",
                newUriStr.c_str(), physicalPath.c_str());
            continue;
        }

        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_COMMON, "uri=%{private}s, path=%{private}s, size=%{public}zu",
            newUriStr.c_str(), physicalPath.c_str(), static_cast<size_t>(buf.st_size));
        existFileUris.push_back(uriStr);
    }
    uris = existFileUris;
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

void PasteboardImgExtractor::FilterImgUris(std::vector<std::string> &uris)
{
    uris.erase(
        std::remove_if(uris.begin(), uris.end(),
            [](const std::string &str) {
                return !MatchImgExtension(str);
            }),
        uris.end()
    );
}

bool PasteboardImgExtractor::MatchImgExtension(const std::string &uri)
{
    static const std::unordered_set<std::string> IMG_EXTENSIONS = {
        "png", "jpg", "jpeg", "jpe", "tif", "tiff", "xbm", "gif", "djv", "djvu", "jng", "pcx", "pbm", "pgm", "pnm",
        "ppm", "rgb", "svg", "svgz", "wbmp", "xpm", "xwd", "heif", "heifs", "hif", "heic", "heics", "jp2", "jpg2",
        "jpx", "jpf", "jpm", "ief", "bmp", "bm", "ico", "cur", "dds", "odi", "oti", "psd", "ai", "dng", "ras", "dwg",
        "dxf", "tga", "sgi", "exr", "fpx", "cdr", "cdt", "cpt", "pat", "ilbm", "avif", "webp", "xcf", "art", "cr2",
        "cr3", "crw", "arw", "nef", "nrw", "raf", "rw2", "raw", "pef", "srw", "erf", "orf", "apng",
    };

    if (uri.empty()) {
        return false;
    }

    size_t startPos = uri.size() - 1;
    while (startPos > 0) {
        char chr = uri.at(startPos);
        if (chr == '/' || chr == '\\') {
            break;
        }
        startPos--;
    }
    if (startPos + 1 >= uri.size()) {
        return false;
    }

    std::string fileName = uri.substr(startPos + 1);
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos == std::string::npos || dotPos == 0 || dotPos + 1 >= fileName.size()) {
        return false;
    }

    size_t endPos = dotPos + 1;
    size_t length = fileName.size();
    size_t extCount = 0;
    while (endPos < length) {
        char chr = fileName.at(endPos);
        if ((chr >= 'a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') || (chr >= '0' && chr <= '9')) {
            endPos++;
            extCount++;
            continue;
        }
        break;
    }

    std::string extension = fileName.substr(dotPos + 1, extCount);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return IMG_EXTENSIONS.find(extension) != IMG_EXTENSIONS.end();
}

std::vector<std::string> PasteboardImgExtractor::FindImgsExcludingSpan(xmlDocPtr doc)
{
    return ExecuteXPath(doc, "//img[@src]");
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
    for (auto i = 0; i < nodeSet->nodeNr; ++i) {
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