/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "pasteboard_distributed_account_subscriber.h"
#include "pasteboard_service.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {

void PasteboardDistributedAccountSubscriber::OnSpaceAccountsChanged(
    const AccountSA::DistributedAccountSpaceEventData &eventData)
{
    if (pasteboardService_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "pasteboardService_ is nullptr");
        return;
    }

    if (eventData.type_ != AccountSA::DistributedAccountSpaceEventType::SWITCHING) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, 
            "Ignore event type %{public}d, not SWITCHING", 
            static_cast<int32_t>(eventData.type_));
        return;
    }

    int32_t userId = eventData.osAccountId_;

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, 
        "SWITCHING event: osAccountId=%{public}d, subspaceId=%{public}d, "
        "previousSubspaceId=%{public}d, clear clipboard for userId=%{public}d",
        eventData.osAccountId_, eventData.subspaceId_, 
        eventData.previousSubspaceId_, userId);

    pasteboardService_->ClearByUser(userId);
}

} // namespace OHOS::MiscServices