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

#include "pasteboard_user_context.h"

#include "accesstoken_kit.h"
#include "errors.h"
#include "os_account_manager.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace OHOS::Security::AccessToken;

UserContext UserContextResolver::ResolveCaller(uint32_t tokenId, pid_t pid, pid_t uid) const
{
    (void)pid;
    (void)uid;
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != ATokenTypeEnum::TOKEN_HAP) {
        return {};
    }

    HapTokenInfo hapInfo;
    if (AccessTokenKit::GetHapTokenInfo(tokenId, hapInfo) != 0) {
        return {};
    }

    UserContext context;
    context.userId = hapInfo.userID;
    context.accountId = hapInfo.userID;
    context.tokenId = tokenId;
    context.source = UserContextSource::CALLER;
    context.isValid = true;
    return context;
}

UserContext UserContextResolver::ResolveEventUser(const EventFwk::CommonEventData &data) const
{
    UserContext context;
    context.userId = data.GetCode();
    context.source = UserContextSource::EVENT;
    context.isValid = context.userId != -1;
    return context;
}

UserContext UserContextResolver::ResolvePackageRemovedUser(const AAFwk::Want &want) const
{
    UserContext context;
    context.userId = want.GetIntParam("userId", -1);
    context.source = UserContextSource::PACKAGE_REMOVED;
    context.isValid = context.userId != -1;
    return context;
}

UserContext UserContextResolver::ResolveInteractionUser(int32_t userId) const
{
    UserContext context;
    context.userId = userId;
    context.source = UserContextSource::INTERACTION;
    context.isValid = context.userId != -1;
    return context;
}

std::vector<int32_t> UserContextResolver::GetForegroundUserIds() const
{
    std::vector<int32_t> accountIds;
    auto ret = AccountSA::OsAccountManager::QueryActiveOsAccountIds(accountIds);
    if (ret != ERR_OK || accountIds.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "query active user failed errCode=%{public}d", ret);
        return {};
    }
    return accountIds;
}

bool IsMainScreenUser(int32_t userId)
{
    return userId == MAIN_SCREEN_USER_ID;
}

bool IsMainDisplayUser(int32_t userId)
{
    return IsMainScreenUser(userId);
}
} // namespace OHOS::MiscServices
