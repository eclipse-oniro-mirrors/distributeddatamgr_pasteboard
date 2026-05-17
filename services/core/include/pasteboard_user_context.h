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

#ifndef PASTEBOARD_USER_CONTEXT_H
#define PASTEBOARD_USER_CONTEXT_H

#include <cstdint>
#include <string>
#include <vector>

#include "common_event_data.h"
#include "os_account_manager.h"
#include "want.h"

namespace OHOS {
namespace MiscServices {
constexpr int32_t ERROR_USERID = -1;
constexpr uint64_t MAIN_DISPLAY_ID = 0;
constexpr const char *USER_SWITCH_OLD_ID = "oldId";
constexpr const char *PACKAGE_REMOVED_USER_ID = "userId";
constexpr int32_t MAIN_SCREEN_USER_ID = 10;

enum class UserContextSource : int32_t {
    CALLER = 0,
    MAIN_DISPLAY,
    FOREGROUND,
    USER_SWITCHED_NEW,
    USER_SWITCHED_OLD,
    USER_STOPPING,
    PACKAGE_REMOVED,
    EVENT,
    INTERACTION,
};

struct UserContext {
    int32_t userId = ERROR_USERID;
    int32_t accountId = -1;
    int32_t uid = -1;
    uint32_t tokenId = 0;
    uint64_t displayId = MAIN_DISPLAY_ID;
    UserContextSource source = UserContextSource::CALLER;
    bool isValid = false;
};

class UserContextResolver {
public:
    virtual ~UserContextResolver() = default;

    virtual UserContext ResolveCallingUser() const;
    virtual UserContext ResolveEventUser(const EventFwk::CommonEventData &data) const;
    virtual UserContext ResolveMainDisplayUser() const;
    virtual std::vector<UserContext> ResolveForegroundUsers() const;
    virtual UserContext ResolveUserSwitchedNewUser(const EventFwk::CommonEventData &data) const;
    virtual UserContext ResolveUserSwitchedOldUser(const AAFwk::Want &want) const;
    virtual UserContext ResolveStoppingUser(const EventFwk::CommonEventData &data) const;
    virtual UserContext ResolvePackageRemovedUser(const AAFwk::Want &want) const;
    virtual UserContext ResolveInteractionUser(int32_t userId) const;
    virtual std::vector<int32_t> GetForegroundUserIds() const;

private:
    UserContext MakeEventContext(int32_t userId, UserContextSource source) const;
};

bool IsMainScreenUser(int32_t userId);
bool IsMainDisplayUser(int32_t userId);
} // namespace MiscServices
} // namespace OHOS
#endif // PASTEBOARD_USER_CONTEXT_H
