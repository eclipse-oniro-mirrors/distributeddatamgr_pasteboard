/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "pasteboard_observer_napi.h"

#include <sys/syscall.h>
#include "pasteboard_hilog.h"

#include <thread>

using namespace OHOS::MiscServices;

namespace OHOS {
namespace MiscServicesNapi {
PasteboardObserverInstance::PasteboardObserverInstance(napi_threadsafe_function callback, napi_env env)
    : callback_(callback), env_(env)
{
}

PasteboardObserverInstance::~PasteboardObserverInstance()
{
    napi_status status = napi_release_threadsafe_function(callback_, napi_tsfn_release);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(status == napi_ok, PASTEBOARD_MODULE_JS_NAPI,
        "release callback failed, status=%{public}d", status);
}

void PasteboardObserverInstance::OnPasteboardChanged()
{
    pid_t threadId = syscall(SYS_gettid);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "enter");
    auto task = [this, threadId]() {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "napi_send_event start, originTid=%{public}d", threadId);
        napi_status status = napi_acquire_threadsafe_function(callback_);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(status == napi_ok, PASTEBOARD_MODULE_JS_NAPI,
            "acquire callback failed, status=%{public}d", status);
        status = napi_call_threadsafe_function(callback_, nullptr, napi_tsfn_blocking);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "call callback failed, status=%{public}d", status);
        }
        status = napi_release_threadsafe_function(callback_, napi_tsfn_release);
        PASTEBOARD_CHECK_AND_RETURN_LOGE(status == napi_ok, PASTEBOARD_MODULE_JS_NAPI,
            "release callback failed, status=%{public}d", status);
    };
    auto ret = napi_send_event(env_, task, napi_eprio_high);
    PASTEBOARD_CHECK_AND_RETURN_LOGE(ret == napi_ok, PASTEBOARD_MODULE_JS_NAPI,
        "napi_send_event failed, result=%{public}d", ret);
}

} // namespace MiscServicesNapi
} // namespace OHOS