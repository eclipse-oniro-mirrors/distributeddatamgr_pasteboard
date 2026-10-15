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

#include "sg_auth_client_mock.h"

SgAuthClientMock::SgAuthClientMock() { mock_ = this; }
SgAuthClientMock::~SgAuthClientMock() { mock_ = nullptr; }
SgAuthClientMock *SgAuthClientMock::GetMock() { return mock_; }

int32_t OHOS::Security::SecurityGuard::CreateAuthProvider(
    const OHOS::Security::SecurityGuard::AuthProviderCallback *callback,
    const std::string &serviceId,
    std::unique_ptr<OHOS::Security::SecurityGuard::AuthProvider> &provider)
{
    return SgAuthClientMock::GetMock()->CreateAuthProvider(callback, serviceId, provider);
}
