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

#ifndef FUSE_GETTER_IPC_MOCK_H
#define FUSE_GETTER_IPC_MOCK_H

#include <gmock/gmock.h>

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"

namespace OHOS {
// Mocks the IPC + AccessToken entry points that PasteboardFuseGetterStub's
// file-static IsNativeCallingToken() consults:
//   - IPCSkeleton::GetCallingTokenID()
//   - Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId)
// IPCSkeleton::GetCallingPid() is mocked as well because the stub logs the
// calling pid on every OnRemoteRequest() entry. The real free functions are
// redirected to this gmock object in fuse_getter_ipc_mock.cpp, mirroring the
// project's accesstoken_kit_mock / fuse_api_mock pattern (gmock interface +
// static mock pointer + GetMock()).
class IFuseGetterIpc {
public:
    virtual ~IFuseGetterIpc() = default;
    virtual uint32_t GetCallingTokenID() = 0;
    virtual pid_t GetCallingPid() = 0;
    virtual Security::AccessToken::ATokenTypeEnum GetTokenTypeFlag(
        Security::AccessToken::AccessTokenID tokenId) = 0;
};

class FuseGetterIpcMock : public IFuseGetterIpc {
public:
    FuseGetterIpcMock();
    ~FuseGetterIpcMock() override;

    MOCK_METHOD(uint32_t, GetCallingTokenID, (), (override));
    MOCK_METHOD(pid_t, GetCallingPid, (), (override));
    MOCK_METHOD(Security::AccessToken::ATokenTypeEnum, GetTokenTypeFlag,
        (Security::AccessToken::AccessTokenID tokenId), (override));

    static FuseGetterIpcMock *GetMock();

private:
    static inline FuseGetterIpcMock *mock_ = nullptr;
};
} // namespace OHOS
#endif // FUSE_GETTER_IPC_MOCK_H
