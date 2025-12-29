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

#ifndef SANDBOX_HELPER_MOCK
#define SANDBOX_HELPER_MOCK

#include <gmock/gmock.h>

#include "sandbox_helper.h"

namespace OHOS {
namespace AppFileService {
class SandboxHelperMock {
public:
    SandboxHelperMock();
    ~SandboxHelperMock();
    static SandboxHelperMock *GetMock();

    MOCK_METHOD(bool, IsValidPath, (const std::string &path), (const));
    MOCK_METHOD(int32_t, GetPhysicalPath, (const std::string &uri, const std::string &userId, std::string &physicalPath), (const));

private:
    static inline SandboxHelperMock *mock_ = nullptr;
};
} // namespace AppFileService
} // namespace OHOS

#endif // SANDBOX_HELPER_MOCK
