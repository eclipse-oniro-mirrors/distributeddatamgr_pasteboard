/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "napi_pasteboard_common.h"
#include "pasteboard_hilog.h"
#include "pasteboard_progress_signal_napi.h"

using namespace OHOS::MiscServices;
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServicesNapi {
static thread_local napi_ref g_progressSignal = nullptr;
constexpr size_t MAX_ARGS = 4;

ProgressSignalNapi::ProgressSignalNapi() : env_(nullptr) {}

ProgressSignalNapi::~ProgressSignalNapi() {}

napi_value ProgressSignalNapi::Cancel(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Cancel is called!");
    ProgressSignalClient::GetInstance().Cancel();
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

void ProgressSignalNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Destructor");
    ProgressSignalNapi *obj = static_cast<ProgressSignalNapi *>(nativeObject);
    delete obj;
}

napi_value ProgressSignalNapi::New(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "proc.");

    // get native object
    ProgressSignalNapi *obj = new (std::nothrow) ProgressSignalNapi();
    if (obj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "New obj is null.");
        return nullptr;
    }
    obj->env_ = env;
    ASSERT_CALL(env, napi_wrap(env, thisVar, obj, ProgressSignalNapi::Destructor,
                       nullptr, // finalize_hint
                       nullptr), obj);
    ProgressSignalClient::GetInstance().Init();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return thisVar;
}

napi_value ProgressSignalNapi::ProgressSignalNapiInit(napi_env env, napi_value exports)
{
    napi_status status = napi_ok;
    napi_property_descriptor descriptors[] = {
        DECLARE_NAPI_WRITABLE_FUNCTION("cancel", Cancel),
    };

    napi_value constructor;
    napi_define_class(env, "ProgressSignal", NAPI_AUTO_LENGTH, New, nullptr,
        sizeof(descriptors) / sizeof(napi_property_descriptor), descriptors, &constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to define class at ProgressSignalNapiInit");
        return nullptr;
    }

    napi_create_reference(env, constructor, 1, &g_progressSignal);
    status = napi_set_named_property(env, exports, "ProgressSignal", constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Set property failed when ProgressSignalNapiInit");
        return nullptr;
    }
    return exports;
}
} // namespace MiscServicesNapi
} // namespace OHOS
