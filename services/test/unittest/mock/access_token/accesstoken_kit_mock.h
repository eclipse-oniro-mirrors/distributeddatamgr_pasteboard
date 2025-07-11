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

#ifndef ACCESSTOKEN_KIT_MOCK_H
#define ACCESSTOKEN_KIT_MOCK_H

#include <gmock/gmock.h>

#include "accesstoken_kit.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
class IAccessTokenKit {
public:
    virtual int32_t VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName) = 0;
};

class AccessTokenKitMock : public IAccessTokenKit {
public:
    AccessTokenKitMock();
    ~AccessTokenKitMock();

    MOCK_METHOD(int32_t, VerifyAccessToken, (AccessTokenID tokenID, const std::string &permissionName), (override));

    static AccessTokenKitMock *GetMock();

private:
    static inline AccessTokenKitMock *mock_ = nullptr;
};
} // namespace AccessToken
} // namespace Security
} // namespace OHOS
#endif // ACCESSTOKEN_KIT_MOCK_H
