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

#include "auth_provider_subscribe_client.h"

class MockAuthProviderSubscribeClient : public OHOS::Security::SecurityGuard::AuthProviderSubscribeClient {
public:
    MOCK_METHOD(int32_t, RequestAuthResult,
        (const OHOS::Security::SecurityGuard::AuthEventInfo &event,
         OHOS::Security::SecurityGuard::AuthResultCallback callback),
        (override));
    MOCK_METHOD(void, DeleteProvider, (), (override));
};

class SgAuthClientMock {
public:
    SgAuthClientMock();
    ~SgAuthClientMock();

    MOCK_METHOD(int32_t, CreateProvider,
        (const OHOS::Security::SecurityGuard::AuthProviderCallback &callback,
         const std::string &serviceId,
         std::shared_ptr<OHOS::Security::SecurityGuard::AuthProviderSubscribeClient> &provider));

    static SgAuthClientMock *GetMock();

private:
    static inline SgAuthClientMock *mock_ = nullptr;
};

#endif // SG_AUTH_CLIENT_MOCK_H
