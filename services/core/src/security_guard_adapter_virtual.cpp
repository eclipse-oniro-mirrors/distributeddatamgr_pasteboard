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

#include "security_guard_adapter.h"

#include "pasteboard_error.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServices {

SecurityGuardAdapter::SecurityGuardAdapter() = default;

SecurityGuardAdapter::~SecurityGuardAdapter()
{
    // authProvider_ (shared_ptr<AuthProvider>) and authCallback_ (AuthProviderCallback*)
    // are always null in virtual mode
}

void SecurityGuardAdapter::OnProviderCreate()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "security_guard not enabled, no-op.");
}

void SecurityGuardAdapter::OnProviderDestroy()
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "security_guard not enabled, no-op.");
}

SecurityGuardAdapter::StrategyResult SecurityGuardAdapter::QueryStrategy(uint32_t srcTokenId, uint32_t dstTokenId)
{
    (void)srcTokenId;
    (void)dstTokenId;
    return StrategyResult::ALLOW;
}

bool SecurityGuardAdapter::ShouldRestrictDistributedData() const
{
    return false;
}

SecurityGuardAdapter &SecurityGuardAdapter::GetInstance()
{
    static SecurityGuardAdapter instance;
    return instance;
}

} // namespace MiscServices
} // namespace OHOS
