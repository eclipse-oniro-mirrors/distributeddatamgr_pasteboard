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
#ifndef PASTEBOARD_SUBPROFILE_SUBSCRIBER_H
#define PASTEBOARD_SUBPROFILE_SUBSCRIBER_H

#include "distributed_account_subscribe_callback.h"

namespace OHOS::MiscServices {
class PasteboardService;
class PasteboardSubProfileSubscriber final : public AccountSA::DistributedAccountSubscribeCallback {
public:
    explicit PasteboardSubProfileSubscriber(const sptr<PasteboardService>& service)
        : pasteboardService_(service) {}
    ~PasteboardSubProfileSubscriber() = default;

    void OnSubProfileAccountsChanged(const AccountSA::DistributedAccountSubProfileEventData &eventData) override;

private:
    void OnSubProfileAccountsChangedInner(AccountSA::DistributedAccountSubProfileEventType type, int32_t osAccountId);
    sptr<PasteboardService> pasteboardService_ = nullptr;
};
} // namespace OHOS::MiscServices
#endif // PASTEBOARD_SUBPROFILE_SUBSCRIBER_H
