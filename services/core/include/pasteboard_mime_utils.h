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

#ifndef PASTEBOARD_MIME_UTILS_H
#define PASTEBOARD_MIME_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS::MiscServices {
constexpr uint16_t MAX_TRANSFER_SIZE = 1300;
constexpr uint8_t UINT16_SIZE = 2;
constexpr uint8_t BYTE_MASK = 0xFF;
constexpr uint8_t BYTE_SHIFT = 8;

std::vector<uint8_t> EncodeMimeTypes(const std::vector<std::string> &mimeTypes);
std::vector<std::string> DecodeMimeTypes(const std::vector<uint8_t> &rawData);
} // namespace OHOS::MiscServices
#endif // PASTEBOARD_MIME_UTILS_H
