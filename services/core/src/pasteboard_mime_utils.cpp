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

#include "pasteboard_mime_utils.h"

#include <cstdint>
#include <cstring>

namespace OHOS::MiscServices {

std::vector<uint8_t> EncodeMimeTypes(const std::vector<std::string> &mimeTypes)
{
    std::vector<uint8_t> result;
    result.reserve(MAX_TRANSFER_SIZE);
    for (const auto &mimeType : mimeTypes) {
        auto len = mimeType.size();
        if (len > UINT16_MAX) {
            continue;
        }
        uint16_t strLen = static_cast<uint16_t>(len);
        if (result.size() + strLen + UINT16_SIZE > MAX_TRANSFER_SIZE) {
            break;
        }
        result.emplace_back(static_cast<uint8_t>(len & BYTE_MASK));
        result.emplace_back(static_cast<uint8_t>((len >> BYTE_SHIFT) & BYTE_MASK));
        const uint8_t *data = reinterpret_cast<const uint8_t *>(mimeType.data());
        result.insert(result.end(), data, data + strLen);
    }
    result.shrink_to_fit();
    return result;
}

std::vector<std::string> DecodeMimeTypes(const std::vector<uint8_t> &rawData)
{
    std::vector<std::string> mimeTypes;
    const uint8_t *data = rawData.data();
    size_t size = rawData.size();
    size_t index = 0;
    while (index + UINT16_SIZE <= size) {
        uint16_t len = static_cast<uint16_t>(data[index]) | (static_cast<uint16_t>(data[index + 1]) << BYTE_SHIFT);
        index += UINT16_SIZE;
        if (index + len > size) {
            break;
        }
        mimeTypes.emplace_back(reinterpret_cast<const char *>(data + index), len);
        index += len;
    }
    return mimeTypes;
}
} // namespace OHOS::MiscServices
