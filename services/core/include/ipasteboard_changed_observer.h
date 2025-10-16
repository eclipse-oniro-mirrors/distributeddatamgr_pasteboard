/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef PASTE_BOARD_CHANGER_OBSERVER_INTERFACE_H
#define PASTE_BOARD_CHANGER_OBSERVER_INTERFACE_H

#include "iremote_broker.h"

namespace OHOS {
namespace MiscServices {
class IPasteboardChangedObserver : public IRemoteBroker {
public:
    struct PasteboardChangedEvent {
        std::string bundleName;
        int32_t status;
        int32_t userId;
    };
    virtual void OnPasteboardChanged() = 0;
    virtual void OnPasteboardEvent(const PasteboardChangedEvent &event) = 0;
    virtual ~IPasteboardChangedObserver() = default;
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.miscservices.pasteboard.IPasteboardChangedObserver");
};
} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_BOARD_CHANGER_OBSERVER_INTERFACE_H
