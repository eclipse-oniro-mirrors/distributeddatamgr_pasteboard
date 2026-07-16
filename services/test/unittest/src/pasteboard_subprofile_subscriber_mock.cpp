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

#include "pasteboard_subprofile_subscriber_test.h"

#include "pasteboard_subprofile_subscriber.h"
#include "os_account_sub_profile_subscribe_callback.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {

int32_t PasteboardService::ClearByUser(int32_t userId)
{
    if (PasteboardServiceInterfaceMock::GetMock() != nullptr) {
        PasteboardServiceInterfaceMock::mockFlag_ = true;
        return PasteboardServiceInterfaceMock::GetMock()->ClearByUser(userId);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByUser real implementation, userId=%{public}d", userId);
    return ERR_OK;
}

} // namespace OHOS::MiscServices