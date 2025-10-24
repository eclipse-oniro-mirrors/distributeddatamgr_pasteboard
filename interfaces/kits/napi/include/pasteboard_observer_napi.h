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

#ifndef PASTEBOARD_OBSERVER_NAPI_H
#define PASTEBOARD_OBSERVER_NAPI_H

#include "event_handler.h"
#include "napi_init.h"
#include "pasteboard_client.h"

namespace OHOS {
namespace MiscServicesNapi {
class PasteboardObserverImpl : public std::enable_shared_from_this<PasteboardObserverImpl> {
public:
    explicit PasteboardObserverImpl(napi_threadsafe_function callback, napi_env env);
    ~PasteboardObserverImpl();
    void OnPasteboardChanged() override;

private:
    std::shared_ptr<napi_threadsafe_function> callback_ = nullptr;
    napi_env env_;
};

class PasteboardObserverInstance : public MiscServices::PasteboardObserver {
public:
    explicit PasteboardObserverInstance(napi_threadsafe_function callback, napi_env env);
    ~PasteboardObserverInstance() = default;
    void OnPasteboardChanged() override;

private:
    std::shared_ptr<PasteboardObserverImpl> impl_ = nullptr;
};

class PasteboardNapiScope {
public:
    PasteboardNapiScope(napi_env env)
    {
        env_ = env;
        napi_open_handle_scope(env_, &scope_);
    }

    ~PasteboardNapiScope()
    {
        if (scope_ != nullptr && env_ != nullptr) {
            napi_close_handle_scope(env_, scope_);
        }
        scope_ = nullptr;
        env_ = nullptr;
    }

private:
    napi_handle_scope scope_ = nullptr;
    napi_env env_ = nullptr;
};
} // namespace MiscServicesNapi
} // namespace OHOS
#endif // PASTEBOARD_OBSERVER_NAPI_H