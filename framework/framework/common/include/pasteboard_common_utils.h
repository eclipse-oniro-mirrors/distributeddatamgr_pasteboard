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

#ifndef PASTEBOARD_COMMON_UTILS_H
#define PASTEBOARD_COMMON_UTILS_H

#include <string>
#include <thread>

namespace OHOS {
namespace MiscServices {
class PasteBoardCommonUtils {
public:
    static void SetThreadTaskName(std::thread &thread, const std::string &taskName);
    static void SetTaskName(const std::string &taskName);
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_COMMON_UTILS_H
