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

#define LOG_TAG "Pasteboard_Observer_Impl"

#include "oh_pasteboard_observer_impl.h"
#include "pasteboard_hilog.h"

OH_PasteboardObserver::~OH_PasteboardObserver() = default;

namespace OHOS {
namespace MiscServices {
void PasteboardObserverCapiImpl::OnPasteboardChanged()
{
    if (innerObserver_ == nullptr || innerObserver_->callback == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "subscriber is nullptr or subscriber callback is nullptr");
        return;
    }
    (innerObserver_->callback)(innerObserver_->context, type_);
}

void PasteboardObserverCapiImpl::SetType(Pasteboard_NotifyType type)
{
    type_ = type;
}

Pasteboard_NotifyType PasteboardObserverCapiImpl::GetType()
{
    return type_;
}

void PasteboardObserverCapiImpl::SetInnerObserver(const OH_PasteboardObserver *innerObserver)
{
    innerObserver_ = innerObserver;
}
} // namespace MiscServices
}