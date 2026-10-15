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

#ifndef SG_AUTH_CLIENT_MOCK_H
#define SG_AUTH_CLIENT_MOCK_H

#include <gmock/gmock.h>

#include "sg_auth_client.h"

class MockAuthProvider : public OHOS::Security::SecurityGuard::AuthProvider {
public:
    MOCK_METHOD(int32_t, RequestAuthResult,
        (const OHOS::Security::SecurityGuard::AuthEventInfo &event,
         const OHOS::Security::SecurityGuard::AuthResultCallback &callback),
        (override));
};

class SgAuthClientMock {
public:
    SgAuthClientMock();
    ~SgAuthClientMock();

    MOCK_METHOD(int32_t, CreateAuthProvider,
        (const OHOS::Security::SecurityGuard::AuthProviderCallback *callback,
         const std::string &serviceId,
         std::unique_ptr<OHOS::Security::SecurityGuard::AuthProvider> &provider));

    static SgAuthClientMock *GetMock();

private:
    static inline SgAuthClientMock *mock_ = nullptr;
};

#endif // SG_AUTH_CLIENT_MOCK_H
