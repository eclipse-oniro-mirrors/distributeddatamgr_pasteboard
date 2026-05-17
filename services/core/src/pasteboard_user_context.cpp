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

#include <cstdlib>
#include <inttypes.h>

#include "ipc_skeleton.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {
namespace {
bool IsValidUserId(int32_t userId)
{
    return userId != ERROR_USERID;
}
} // namespace

UserContext UserContextResolver::ResolveCallingUser() const
{
    UserContext context;
    context.source = UserContextSource::CALLER;
    context.uid = IPCSkeleton::GetCallingUid();
    int32_t userId = ERROR_USERID;
    auto ret = AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(context.uid, userId);
    if (ret != ERR_OK || !IsValidUserId(userId)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "resolve calling user failed, uid=%{public}d, ret=%{public}d", context.uid, ret);
        return context;
    }
    context.userId = userId;
    context.isValid = true;
    return context;
}

UserContext UserContextResolver::ResolveMainDisplayUser() const
{
    UserContext context;
    context.source = UserContextSource::MAIN_DISPLAY;
    context.displayId = MAIN_DISPLAY_ID;
    int32_t userId = ERROR_USERID;
    auto ret = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(MAIN_DISPLAY_ID, userId);
    if (ret != ERR_OK || !IsValidUserId(userId)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "resolve main display user failed, displayId=%{public}" PRIu64 ", ret=%{public}d",
            MAIN_DISPLAY_ID, ret);
        return context;
    }
    context.userId = userId;
    context.isValid = true;
    return context;
}

std::vector<UserContext> UserContextResolver::ResolveForegroundUsers() const
{
    std::vector<AccountSA::ForegroundOsAccount> accounts;
    auto ret = AccountSA::OsAccountManager::GetForegroundOsAccounts(accounts);
    if (ret != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "resolve foreground users failed, ret=%{public}d", ret);
        return {};
    }
    std::vector<UserContext> contexts;
    for (const auto &account : accounts) {
        UserContext context;
        context.source = UserContextSource::FOREGROUND;
        context.userId = account.localId;
        context.displayId = account.displayId;
        context.isValid = IsValidUserId(context.userId);
        if (context.isValid) {
            contexts.emplace_back(context);
        }
    }
    return contexts;
}

UserContext UserContextResolver::ResolveUserSwitchedNewUser(const EventFwk::CommonEventData &data) const
{
    return MakeEventContext(data.GetCode(), UserContextSource::USER_SWITCHED_NEW);
}

UserContext UserContextResolver::ResolveUserSwitchedOldUser(const AAFwk::Want &want) const
{
    std::string oldId = want.GetStringParam(USER_SWITCH_OLD_ID);
    if (oldId.empty()) {
        return MakeEventContext(ERROR_USERID, UserContextSource::USER_SWITCHED_OLD);
    }
    char *end = nullptr;
    long value = std::strtol(oldId.c_str(), &end, 10);
    if (end == oldId.c_str() || *end != '\0') {
        return MakeEventContext(ERROR_USERID, UserContextSource::USER_SWITCHED_OLD);
    }
    return MakeEventContext(static_cast<int32_t>(value), UserContextSource::USER_SWITCHED_OLD);
}

UserContext UserContextResolver::ResolveStoppingUser(const EventFwk::CommonEventData &data) const
{
    return MakeEventContext(data.GetCode(), UserContextSource::USER_STOPPING);
}

UserContext UserContextResolver::ResolvePackageRemovedUser(const AAFwk::Want &want) const
{
    return MakeEventContext(want.GetIntParam(PACKAGE_REMOVED_USER_ID, ERROR_USERID),
        UserContextSource::PACKAGE_REMOVED);
}

UserContext UserContextResolver::MakeEventContext(int32_t userId, UserContextSource source) const
{
    UserContext context;
    context.source = source;
    context.userId = userId;
    context.isValid = IsValidUserId(userId);
    return context;
}
} // namespace MiscServices
} // namespace OHOS
