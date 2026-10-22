/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
#ifndef OH_PASTEBOARD_OBSERVER_IMPL_H
#define OH_PASTEBOARD_OBSERVER_IMPL_H

#include "oh_pasteboard_common.h"

struct OH_PasteboardObserver {
    const int64_t cid = SUBSCRIBER_STRUCT_ID;
    Pasteboard_Notify callback{ nullptr };
    void *context{ nullptr };
    Pasteboard_Finalize finalize{ nullptr };
    OHOS::sptr<OHOS::MiscServices::PasteboardObserverCapiImpl> impl{};
    ~OH_PasteboardObserver();
};

namespace OHOS {
namespace MiscServices {
class PasteboardObserverCapiImpl : public PasteboardObserver {
public:
    void OnPasteboardChanged() override;
    void SetType(Pasteboard_NotifyType type);
    Pasteboard_NotifyType GetType();
    void SetInnerObserver(const OH_PasteboardObserver *observer);

private:
    const OH_PasteboardObserver *innerObserver_;
    Pasteboard_NotifyType type_;
};
} // namespace MiscServices
} // namespace OHOS

/** @} */
#endif