/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef PASTEBOARD_COMMON_EVENT_SUBSCRIBER_H
#define PASTEBOARD_COMMON_EVENT_SUBSCRIBER_H

#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "ipasteboard_service.h"

namespace OHOS::MiscServices {
class PasteboardService;
class PasteBoardCommonEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
    PasteBoardCommonEventSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo,
        sptr<PasteboardService> service) : EventFwk::CommonEventSubscriber(subscribeInfo)
    {
        pasteboardService_ = service;
    }
    ~PasteBoardCommonEventSubscriber() = default;
    void OnReceiveEvent(const EventFwk::CommonEventData &data) override;

private:
    void OnReceiveEventInner(const EventFwk::CommonEventData &data);
    std::mutex mutex_;
    sptr<PasteboardService> pasteboardService_ = nullptr;
};
} // namespace OHOS::MiscServices
#endif // PASTEBOARD_COMMON_EVENT_SUBSCRIBER_H
