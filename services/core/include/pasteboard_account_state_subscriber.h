/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_ACCOUNT_STATE_SUBSCRIBER_H
#define PASTEBOARD_ACCOUNT_STATE_SUBSCRIBER_H

#include "os_account_manager.h"
#include "os_account_subscribe_info.h"
#include "os_account_subscriber.h"
#include "ipasteboard_service.h"

namespace OHOS::MiscServices {
class PasteboardService;
class PasteBoardAccountStateSubscriber final : public AccountSA::OsAccountSubscriber {
public:
    PasteBoardAccountStateSubscriber(const AccountSA::OsAccountSubscribeInfo &subscribeInfo,
        sptr<PasteboardService> service) : AccountSA::OsAccountSubscriber(subscribeInfo)
    {
        pasteboardService_ = service;
    }
    ~PasteBoardAccountStateSubscriber() = default;
    void OnStateChanged(const AccountSA::OsAccountStateData &data) override;

private:
    void OnStateChangedInner(const AccountSA::OsAccountStateData &data);

    sptr<PasteboardService> pasteboardService_ = nullptr;
};
} // namespace OHOS::MiscServices
#endif // PASTEBOARD_ACCOUNT_STATE_SUBSCRIBER_H
