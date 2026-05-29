/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_PASTEBOARD_SECURITY_LEVEL_H
#define OHOS_PASTEBOARD_SECURITY_LEVEL_H

#include <atomic>
#include <mutex>
#include <string>

namespace OHOS::MiscServices {
class SecurityLevel {
public:
    SecurityLevel();
    bool IsSupportedDistributed(bool needLog);

private:
    uint32_t GetSensitiveLevel();
    uint32_t GetDeviceSecurityLevel();

    std::atomic<uint32_t> securityLevel_;
    std::mutex mutex_;
};
} // namespace OHOS::MiscServices
#endif // OHOS_PASTEBOARD_SECURITY_LEVEL_H
