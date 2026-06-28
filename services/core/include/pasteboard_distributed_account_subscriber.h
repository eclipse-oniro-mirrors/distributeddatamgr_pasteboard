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

#ifndef PASTEBOARD_DISTRIBUTED_ACCOUNT_SUBSCRIBER_H
#define PASTEBOARD_DISTRIBUTED_ACCOUNT_SUBSCRIBER_H

#include "os_account_manager.h"
#include "distributed_account_subscribe_callback.h"
#include "ipasteboard_service.h"

namespace OHOS::MiscServices {
class PasteboardService;

class PasteboardDistributedAccountSubscriber 
    : public AccountSA::DistributedAccountSubscribeCallback {
public:
    explicit PasteboardDistributedAccountSubscriber(sptr<PasteboardService> service)
        : pasteboardService_(service) {}
    ~PasteboardDistributedAccountSubscriber() = default;

    void OnSpaceAccountsChanged(
        const AccountSA::DistributedAccountSpaceEventData &eventData) override;

private:
    sptr<PasteboardService> pasteboardService_ = nullptr;
};
} // namespace OHOS::MiscServices

#endif // PASTEBOARD_DISTRIBUTED_ACCOUNT_SUBSCRIBER_H