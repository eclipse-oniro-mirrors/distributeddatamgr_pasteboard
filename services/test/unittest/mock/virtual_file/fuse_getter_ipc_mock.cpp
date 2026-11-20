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

#include "fuse_getter_ipc_mock.h"

namespace OHOS {
FuseGetterIpcMock::FuseGetterIpcMock()
{
    mock_ = this;
}

FuseGetterIpcMock::~FuseGetterIpcMock()
{
    mock_ = nullptr;
}

FuseGetterIpcMock *FuseGetterIpcMock::GetMock()
{
    return mock_;
}

// Redirect IPCSkeleton entry points to the gmock object. These definitions
// live in the test executable and shadow the real ones from ipc:ipc_single at
// link time, exactly like services/test/unittest/src/pasteboard_service_mock_test.cpp
// does. They are member-function definitions of C++-linkage classes, so they
// keep C++ linkage regardless of any surrounding extern "C".
uint32_t IPCSkeleton::GetCallingTokenID()
{
    auto mock = FuseGetterIpcMock::GetMock();
    if (mock == nullptr) {
        return 0;
    }
    return mock->GetCallingTokenID();
}

pid_t IPCSkeleton::GetCallingPid()
{
    auto mock = FuseGetterIpcMock::GetMock();
    if (mock == nullptr) {
        return 0;
    }
    return mock->GetCallingPid();
}
} // namespace OHOS

namespace OHOS {
namespace Security {
namespace AccessToken {
ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(AccessTokenID tokenId)
{
    auto mock = FuseGetterIpcMock::GetMock();
    if (mock == nullptr) {
        return ATokenTypeEnum::TOKEN_INVALID;
    }
    return mock->GetTokenTypeFlag(tokenId);
}
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
