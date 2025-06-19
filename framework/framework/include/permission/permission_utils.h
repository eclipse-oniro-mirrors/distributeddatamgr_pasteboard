/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_PERMISSION_UTILS_H
#define PASTEBOARD_PERMISSION_UTILS_H

#include <cstdint>
#include <string>

#include "api/visibility.h"

namespace OHOS {
namespace MiscServices {
class API_EXPORT PermissionUtils {
public:
    static bool IsPermissionGranted(const std::string &perm, uint32_t tokenId);

    static inline const char *PERMISSION_READ_PASTEBOARD = "ohos.permission.READ_PASTEBOARD";
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_PERMISSION_UTILS_H
