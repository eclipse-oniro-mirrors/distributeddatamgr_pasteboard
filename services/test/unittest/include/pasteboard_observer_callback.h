/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef PASTE_SERVICE_TEST_H
#define PASTE_SERVICE_TEST_H

#include "pasteboard_error.h"
#include "pasteboard_observer.h"
#include "pasteboard_observer_stub.h"

namespace OHOS {
namespace MiscServices {
class PasteboardObserverCallback : public PasteboardObserver {
public:
    PasteboardObserverCallback() {};
    ~PasteboardObserverCallback()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "deconstructor.");
    };
    void OnPasteboardChanged() override;
};

class PasteboardEventObserverCallback : public PasteboardObserver {
public:
    PasteboardEventObserverCallback() {};
    ~PasteboardEventObserverCallback()
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "deconstructor.");
    };
    void OnPasteboardEvent(const PasteboardChangedEvent &event) override;
};

} // namespace MiscServices
} // namespace OHOS
#endif // PASTE_SERVICE_TEST_H
