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

#include "pasteboard_subprofile_subscriber.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service.h"
#include "common/pasteboard_common_utils.h"

namespace OHOS::MiscServices {

void PasteboardSubProfileSubscriber::OnSubProfileChanged(
    const AccountSA::SubProfileEventData &eventData)
{
    std::thread thread([service = pasteboardService_,
        type = eventData.type_,
        osAccountId = eventData.osAccountId_]() {
        OnSubProfileAccountsChangedInner(service, type, osAccountId);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "OnSubProfileChanged");
    thread.detach();
}

void PasteboardSubProfileSubscriber::OnSubProfileAccountsChangedInner(
    const sptr<PasteboardService>& service,
    AccountSA::OsAccountSubProfileEventType type, int32_t osAccountId)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "Event received: type=%{public}d, osAccountId=%{public}d",
        static_cast<int32_t>(type), osAccountId);

    PASTEBOARD_CHECK_AND_RETURN_LOGE(service != nullptr,
        PASTEBOARD_MODULE_SERVICE, "service is nullptr");

    PASTEBOARD_CHECK_AND_RETURN_LOGE(type != AccountSA::OsAccountSubProfileEventType::INVALID_TYPE,
        PASTEBOARD_MODULE_SERVICE, "Invalid event type");

    PASTEBOARD_CHECK_AND_RETURN_LOGE(osAccountId >= 0,
        PASTEBOARD_MODULE_SERVICE, "Invalid osAccountId=%{public}d", osAccountId);

    int32_t result = service->ClearByUser(osAccountId);
    if (result != ERR_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE, "ClearByUser failed, osAccountId=%{public}d, result=%{public}d",
            osAccountId, result);
    } else {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ClearByUser successful, osAccountId=%{public}d", osAccountId);
    }
}

} // namespace OHOS::MiscServices
