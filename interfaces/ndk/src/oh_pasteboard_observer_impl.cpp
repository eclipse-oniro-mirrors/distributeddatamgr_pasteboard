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
#include <mutex>
#include "pasteboard_hilog.h"

using OHOS::MiscServices::PASTEBOARD;
using OHOS::MiscServices::PASTEBOARD_MODULE_CAPI;

OH_PasteboardObserver::~OH_PasteboardObserver() = default;

void OH_PasteboardObserver::ReleaseRef()
{
    if (refCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        if (finalize != nullptr) {
            (finalize)(context);
            finalize = nullptr;
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CAPI, "context finalized");
        }
        delete this;
    }
}

namespace OHOS {
namespace MiscServices {
void PasteboardObserverCapiImpl::OnPasteboardChanged()
{
    Pasteboard_Notify callback = nullptr;
    void *context = nullptr;
    OH_PasteboardObserver *observer = nullptr;
    {
        std::lock_guard<std::mutex> lock(implMutex_);
        if (innerObserver_ == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "subscriber is nullptr or subscriber callback is nullptr");
            return;
        }
        std::lock_guard<std::mutex> obsLock(innerObserver_->mutex);
        callback = innerObserver_->callback;
        context = innerObserver_->context;
        observer = const_cast<OH_PasteboardObserver *>(innerObserver_);
        observer->refCount.fetch_add(1, std::memory_order_acq_rel);
    }
    if (callback == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "subscriber is nullptr or subscriber callback is nullptr");
        observer->ReleaseRef();
        return;
    }
    callback(context, type_);
    observer->ReleaseRef();
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
    std::lock_guard<std::mutex> lock(implMutex_);
    innerObserver_ = innerObserver;
}
} // namespace MiscServices
}