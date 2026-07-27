/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_PASTEBOARD_FRAMEWORK_COMMON_HISTOGRAM_ENUM_H
#define OHOS_DISTRIBUTED_DATA_PASTEBOARD_FRAMEWORK_COMMON_HISTOGRAM_ENUM_H

namespace OHOS::MiscServices {

enum class HistogramMimeTypeEnum : int32_t {
    HISTOGRAM_TEXT_HTML = 0,
    HISTOGRAM_TEXT_PLAIN = 1,
    HISTOGRAM_TEXT_URI = 2,
    HISTOGRAM_PIXELMAP = 3,
    HISTOGRAM_TEXT_WANT = 4,
    HISTOGRAM_UNKNOWN = 5
};
constexpr int32_t HISTOGRAM_MIMETYPE_BOUNDARY = 6; // HISTOGRAM_UNKNOWN + 1

constexpr int32_t HISTOGRAM_SHAREOPTION_BOUNDARY = 3; // CrossDevice + 1

constexpr int32_t HISTOGRAM_NOTIFYTYPE_BOUNDARY = 3; // NOTIFY_REMOTE_DATA_CHANGE + 1

inline int32_t GetHistogramMimeTypeEnum(const std::string &mimeType)
{
    if (mimeType == "text/html") {
        return static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_HTML);
    } else if (mimeType == "text/plain") {
        return static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_PLAIN);
    } else if (mimeType == "text/uri") {
        return static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_URI);
    } else if (mimeType == "pixelMap") {
        return static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_PIXELMAP);
    } else if (mimeType == "text/want") {
        return static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_TEXT_WANT);
    }
    return static_cast<int32_t>(HistogramMimeTypeEnum::HISTOGRAM_UNKNOWN);
}

} // namespace OHOS::MiscServices
#endif // OHOS_DISTRIBUTED_DATA_PASTEBOARD_FRAMEWORK_COMMON_HISTOGRAM_H