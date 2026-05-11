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
#include <sys/types.h>
#include <vector>

#include "common_event_data.h"
#include "want.h"

namespace OHOS::MiscServices {
constexpr int32_t MAIN_SCREEN_USER_ID = 10;

enum class UserContextSource : int32_t {
    CALLER = 0,
    EVENT,
    PACKAGE_REMOVED,
    INTERACTION,
    FOREGROUND,
};

struct UserContext {
    int32_t userId = -1;
    int32_t accountId = -1;
    uint32_t tokenId = 0;
    int32_t displayId = -1;
    UserContextSource source = UserContextSource::CALLER;
    bool isValid = false;
};

class UserContextResolver {
public:
    virtual ~UserContextResolver() = default;

    virtual UserContext ResolveCaller(uint32_t tokenId, pid_t pid, pid_t uid) const;
    virtual UserContext ResolveEventUser(const EventFwk::CommonEventData &data) const;
    virtual UserContext ResolvePackageRemovedUser(const AAFwk::Want &want) const;
    virtual UserContext ResolveInteractionUser(int32_t userId) const;
    virtual std::vector<int32_t> GetForegroundUserIds() const;
};

bool IsMainScreenUser(int32_t userId);
bool IsMainDisplayUser(int32_t userId);
} // namespace OHOS::MiscServices

#endif // PASTEBOARD_USER_CONTEXT_H
