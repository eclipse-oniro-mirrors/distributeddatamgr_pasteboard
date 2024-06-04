/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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
#include <thread>
#include <uv.h>

#include "napi_common_want.h"
#include "pasteboard_common.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_js_err.h"
#include "systempasteboard_napi.h"
using namespace OHOS::MiscServices;
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServicesNapi {
static thread_local napi_ref g_systemPasteboard = nullptr;
static thread_local napi_ref g_systemPasteboard_instance = nullptr;
thread_local std::map<napi_ref, std::shared_ptr<PasteboardObserverInstance>> SystemPasteboardNapi::observers_;
std::shared_ptr<PasteboardDelayGetterInstance> SystemPasteboardNapi::delayGetter_;
std::mutex SystemPasteboardNapi::delayMutex_;
constexpr int ARGC_TYPE_SET1 = 1;
constexpr size_t MAX_ARGS = 6;
constexpr size_t SYNC_TIMEOUT = 5000;
constexpr size_t DELAY_TIMEOUT = 2;
const std::string STRING_UPDATE = "update";
PasteboardObserverInstance::PasteboardObserverInstance(const napi_env &env, const napi_ref &ref) : env_(env), ref_(ref)
{
    stub_ = new (std::nothrow) PasteboardObserverInstance::PasteboardObserverImpl();
}

PasteboardObserverInstance::~PasteboardObserverInstance()
{
    napi_delete_reference(env_, ref_);
}

sptr<PasteboardObserverInstance::PasteboardObserverImpl> PasteboardObserverInstance::GetStub()
{
    return stub_;
}

void UvQueueWorkOnPasteboardChanged(uv_work_t *work, int status)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "UvQueueWorkOnPasteboardChanged start");
    if (work == nullptr) {
        return;
    }
    PasteboardDataWorker *pasteboardDataWorker = (PasteboardDataWorker *)work->data;
    if (pasteboardDataWorker == nullptr || pasteboardDataWorker->observer == nullptr) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "pasteboardDataWorker or ref is null");
        delete work;
        work = nullptr;
        return;
    }

    auto env = pasteboardDataWorker->observer->GetEnv();
    auto ref = pasteboardDataWorker->observer->GetRef();

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "scope null");
        return;
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value callback = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(env, ref, &callback);
    napi_value result = NapiGetNull(env);
    napi_call_function(env, undefined, callback, 0, &result, &resultOut);

    napi_close_handle_scope(env, scope);
    delete pasteboardDataWorker;
    pasteboardDataWorker = nullptr;
    delete work;
}

void PasteboardObserverInstance::OnPasteboardChanged()
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "OnPasteboardChanged is called!");
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "loop instance is nullptr");
        return;
    }

    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "work is null");
        return;
    }
    PasteboardDataWorker *pasteboardDataWorker = new (std::nothrow) PasteboardDataWorker();
    if (pasteboardDataWorker == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "pasteboardDataWorker is null");
        delete work;
        work = nullptr;
        return;
    }
    pasteboardDataWorker->observer = shared_from_this();

    work->data = (void *)pasteboardDataWorker;

    int ret = uv_queue_work(
        loop, work, [](uv_work_t *work) {}, UvQueueWorkOnPasteboardChanged);
    if (ret != 0) {
        delete pasteboardDataWorker;
        pasteboardDataWorker = nullptr;
        delete work;
        work = nullptr;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "OnPasteboardChanged end");
}

PasteboardDelayGetterInstance::PasteboardDelayGetterInstance(const napi_env &env, const napi_ref &ref)
    : env_(env), ref_(ref)
{
    stub_ = std::make_shared<PasteboardDelayGetterInstance::PasteboardDelayGetterImpl>();
}

PasteboardDelayGetterInstance::~PasteboardDelayGetterInstance()
{
    napi_delete_reference(env_, ref_);
}

void UvQueueWorkGetDelayPasteData(uv_work_t *work, int status)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "UvQueueWorkGetDelayPasteData start");
    if (work == nullptr) {
        return;
    }
    PasteboardDelayWorker *pasteboardDelayWorker = (PasteboardDelayWorker *)work->data;
    if (pasteboardDelayWorker == nullptr || pasteboardDelayWorker->delayGetter == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "pasteboardDataWorker or delayGetter is null");
        delete work;
        work = nullptr;
        return;
    }
    auto env = pasteboardDelayWorker->delayGetter->GetEnv();
    auto ref = pasteboardDelayWorker->delayGetter->GetRef();
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    if (scope == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "scope is null");
        return;
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    napi_value argv[1] = { CreateNapiString(env, pasteboardDelayWorker->dataType) };
    napi_value callback = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(env, ref, &callback);
    {
        std::unique_lock<std::mutex> lock(pasteboardDelayWorker->mutex);
        auto ret = napi_call_function(env, undefined, callback, 1, argv, &resultOut);
        if (ret == napi_ok) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "get delay data success");
            UDMF::UnifiedDataNapi *unifiedDataNapi = nullptr;
            napi_unwrap(env, resultOut, reinterpret_cast<void **>(&unifiedDataNapi));
            if (unifiedDataNapi != nullptr) {
                pasteboardDelayWorker->unifiedData = unifiedDataNapi->value_;
            }
        }
        napi_close_handle_scope(env, scope);
        pasteboardDelayWorker->complete = true;
        if (!pasteboardDelayWorker->clean) {
            pasteboardDelayWorker->cv.notify_all();
            return;
        }
    }
    delete pasteboardDelayWorker;
    pasteboardDelayWorker = nullptr;
    delete work;
    work = nullptr;
}

void PasteboardDelayGetterInstance::GetUnifiedData(const std::string &type, UDMF::UnifiedData &data)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (loop == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "loop instance is nullptr");
        return;
    }
    uv_work_t *work = new (std::nothrow) uv_work_t;
    if (work == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "work is null");
        return;
    }
    PasteboardDelayWorker *pasteboardDelayWorker = new (std::nothrow) PasteboardDelayWorker();
    if (pasteboardDelayWorker == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "pasteboardDataWorker is null");
        delete work;
        work = nullptr;
        return;
    }
    pasteboardDelayWorker->delayGetter = shared_from_this();
    pasteboardDelayWorker->dataType = type;
    work->data = (void *)pasteboardDelayWorker;
    bool noNeedClean = false;
    {
        std::unique_lock<std::mutex> lock(pasteboardDelayWorker->mutex);
        int ret = uv_queue_work(loop, work, [](uv_work_t *work) {}, UvQueueWorkGetDelayPasteData);
        if (ret != 0) {
            delete pasteboardDelayWorker;
            pasteboardDelayWorker = nullptr;
            delete work;
            work = nullptr;
            return;
        }
        if (pasteboardDelayWorker->cv.wait_for(lock, std::chrono::seconds(DELAY_TIMEOUT),
            [pasteboardDelayWorker] { return pasteboardDelayWorker->complete; }) &&
            pasteboardDelayWorker->unifiedData != nullptr) {
            data = *(pasteboardDelayWorker->unifiedData);
        }
        if (!pasteboardDelayWorker->complete && uv_cancel((uv_req_t*)work) != 0) {
            pasteboardDelayWorker->clean = true;
            noNeedClean = true;
        }
    }
    if (!noNeedClean) {
        delete pasteboardDelayWorker;
        pasteboardDelayWorker = nullptr;
        delete work;
        work = nullptr;
    }
}

bool SystemPasteboardNapi::CheckAgrsOfOnAndOff(napi_env env, bool checkArgsCount, napi_value *argv, size_t argc)
{
    if (!CheckExpression(
        env, checkArgsCount, JSErrorCode::INVALID_PARAMETERS, "Parameter error. The number of arguments is wrong.") ||
        !CheckArgsType(env, argv[0], napi_string, "Parameter error. The type of mimeType must be string.")) {
        return false;
    }
    std::string mimeType;
    bool ret = GetValue(env, argv[0], mimeType);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "GetValue failed");
        return false;
    }
    if (!CheckExpression(env, mimeType == STRING_UPDATE, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The value of type must be update")) {
        return false;
    }
    return true;
}

napi_value SystemPasteboardNapi::On(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi on() is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = 0;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    // on(type: 'update', callback: () => void) has 2 args
    if (!CheckAgrsOfOnAndOff(env, argc >= 2, argv, argc) ||
        !CheckArgsType(env, argv[1], napi_function, "Parameter error. The type of callback must be function.")) {
        return nullptr;
    }

    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    auto observer = GetObserver(env, argv[1]);
    if (observer != nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "observer exist.");
        return result;
    }
    napi_ref ref = nullptr;
    napi_create_reference(env, argv[1], 1, &ref);
    observer = std::make_shared<PasteboardObserverInstance>(env, ref);
    observer->GetStub()->SetObserverWrapper(observer);
    PasteboardClient::GetInstance()->AddPasteboardChangedObserver(observer->GetStub());
    observers_[ref] = observer;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi on() is end!");
    return result;
}

napi_value SystemPasteboardNapi::Off(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi off () is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = 0;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    // off(type: 'update', callback?: () => void) has at least 1 arg
    if (!CheckAgrsOfOnAndOff(env, argc >= 1, argv, argc)) {
        return nullptr;
    }

    std::shared_ptr<PasteboardObserverInstance> observer = nullptr;
    // 1: is the observer parameter
    if (argc > 1) {
        if (!CheckArgsType(env, argv[1], napi_function, "Parameter error. The type of callback must be function.")) {
            return nullptr;
        }
        observer = GetObserver(env, argv[1]);
    }

    DeleteObserver(observer);
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi off () is called!");
    return result;
}

napi_value SystemPasteboardNapi::Clear(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Clear is called!");
    auto context = std::make_shared<AsyncCall::Context>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        // clear has 0 or 1 args
        if (argc > 0 &&
            !CheckArgsType(env, argv[0], napi_function, "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec Clear");
        PasteboardClient::GetInstance()->Clear();
    };
    context->SetAction(std::move(input));
    // 0: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, context, 0);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::ClearData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "ClearData is called!");
    return Clear(env, info);
}

napi_value SystemPasteboardNapi::HasPasteData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "HasPasteData is called!");
    auto context = std::make_shared<HasContextInfo>();
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        // hasPasteData has 0 or 1 args
        if (argc > 0 &&
            !CheckArgsType(env, argv[0], napi_function, "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_status status = napi_get_boolean(env, context->hasPasteData, result);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_get_boolean status = %{public}d", status);
        return status;
    };
    auto exec = [context](AsyncCall::Context *ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec HasPasteData");
        context->hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "HasPasteData result = %{public}d", context->hasPasteData);
        context->status = napi_ok;
    };
    context->SetAction(std::move(input), std::move(output));
    // 0: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, context, 0);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::HasData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "HasData is called!");
    return HasPasteData(env, info);
}

void SystemPasteboardNapi::GetDataCommon(std::shared_ptr<GetContextInfo> &context)
{
    auto input = [](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        // 1: GetPasteData has 0 or 1 args
        if (argc > 0 &&
            !CheckArgsType(env, argv[0], napi_function, "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        return napi_ok;
    };

    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        napi_value instance = nullptr;
        PasteDataNapi::NewInstance(env, instance);
        PasteDataNapi *obj = nullptr;
        napi_status ret = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
        if ((ret == napi_ok) || (obj != nullptr)) {
            obj->value_ = context->pasteData;
        } else {
            return napi_generic_failure;
        }
        *result = instance;
        return napi_ok;
    };
    context->SetAction(std::move(input), std::move(output));
}

napi_value SystemPasteboardNapi::GetPasteData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetPasteData is called!");

    auto context = std::make_shared<GetContextInfo>();
    context->pasteData = std::make_shared<PasteData>();
    GetDataCommon(context);

    auto exec = [context](AsyncCall::Context *ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetPasteData Begin");
        PasteboardClient::GetInstance()->GetPasteData(*context->pasteData);
        context->status = napi_ok;
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetPasteData End");
    };

    // 0: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, context, 0);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::GetData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetData is called!");

    auto context = std::make_shared<GetContextInfo>();
    context->pasteData = std::make_shared<PasteData>();
    GetDataCommon(context);

    auto exec = [context](AsyncCall::Context *ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetData Begin");
        int32_t ret = PasteboardClient::GetInstance()->GetPasteData(*context->pasteData);
        if (ret == static_cast<int32_t>(PasteboardError::E_IS_BEGING_PROCESSED)) {
            context->SetErrInfo(ret, "Another getData is being processed");
        } else {
            context->status = napi_ok;
        }
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetData End");
    };
    // 0: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, context, 0);
    return asyncCall.Call(env, exec);
}

void SystemPasteboardNapi::SetDataCommon(std::shared_ptr<SetContextInfo> &context)
{
    auto input = [context](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        // setData has 1 or 2 args
        if (!CheckExpression(
            env, argc > 0, JSErrorCode::INVALID_PARAMETERS,
            "Parameter error. The number of arguments must be greater than zero.") ||
            !CheckExpression(env, PasteDataNapi::IsPasteData(env, argv[0]), JSErrorCode::INVALID_PARAMETERS,
            "Parameter error. The Type of data must be pasteData.")) {
            return napi_invalid_arg;
        }
        if (argc > 1 &&
            !CheckArgsType(env, argv[1], napi_function, "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        PasteDataNapi *pasteData = nullptr;
        napi_unwrap(env, argv[0], reinterpret_cast<void **>(&pasteData));
        if (pasteData != nullptr) {
            context->obj = pasteData->value_;
        }
        return napi_ok;
    };
    context->SetAction(std::move(input));
}

napi_value SystemPasteboardNapi::SetPasteData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SetPasteData is called!");
    auto context = std::make_shared<SetContextInfo>();
    SetDataCommon(context);

    auto exec = [context](AsyncCall::Context *ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec SetPasteData");
        if (context->obj != nullptr) {
            PasteboardClient::GetInstance()->SetPasteData(*(context->obj));
            context->obj = nullptr;
        }
        context->status = napi_ok;
    };
    // 1: the AsyncCall at the second position
    AsyncCall asyncCall(env, info, context, 1);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::SetData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SetData is called!");
    auto context = std::make_shared<SetContextInfo>();
    SetDataCommon(context);

    auto exec = [context](AsyncCall::Context *ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec SetPasteData");
        int32_t ret = static_cast<int32_t>(PasteboardError::E_ERROR);
        if (context->obj != nullptr) {
            ret = PasteboardClient::GetInstance()->SetPasteData(*(context->obj));
            context->obj = nullptr;
        }
        if (ret == static_cast<int>(PasteboardError::E_OK)) {
            context->status = napi_ok;
        } else if (ret == static_cast<int>(PasteboardError::E_COPY_FORBIDDEN)) {
            context->SetErrInfo(ret, "The system prohibits copying");
        } else if (ret == static_cast<int>(PasteboardError::E_IS_BEGING_PROCESSED)) {
            context->SetErrInfo(ret, "Another setData is being processed");
        }
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec context->status[%{public}d]", context->status);
    };
    // 1: the AsyncCall at the second position
    AsyncCall asyncCall(env, info, context, 1);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::SetUnifiedData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SetUnifiedData is called!");
    auto context = std::make_shared<SetUnifiedContextInfo>();
    SetDataCommon(context);

    auto exec = [context](AsyncCall::Context* ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec SetPasteData");
        int32_t ret = static_cast<int32_t>(PasteboardError::E_ERROR);
        if (context->obj != nullptr) {
            if (context->isDelay && context->delayGetter != nullptr) {
                ret = PasteboardClient::GetInstance()->SetUnifiedData(*(context->obj), context->delayGetter->GetStub());
            } else {
                ret = PasteboardClient::GetInstance()->SetUnifiedData(*(context->obj));
            }
            context->obj = nullptr;
        }
        if (ret == static_cast<int>(PasteboardError::E_OK)) {
            context->status = napi_ok;
        } else if (ret == static_cast<int>(PasteboardError::E_COPY_FORBIDDEN)) {
            context->SetErrInfo(ret, "The system prohibits copying");
        } else if (ret == static_cast<int>(PasteboardError::E_IS_BEGING_PROCESSED)) {
            context->SetErrInfo(ret, "Another setData is being processed");
        }
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "exec context->status[%{public}d]", context->status);
    };
    // 1: the AsyncCall at the second position
    AsyncCall asyncCall(env, info, context, 1);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::GetUnifiedData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetUnifiedData is called!");

    auto context = std::make_shared<GetUnifiedContextInfo>();
    context->unifiedData = std::make_shared<UDMF::UnifiedData>();
    GetDataCommon(context);

    auto exec = [context](AsyncCall::Context* ctx) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetUnifiedData Begin");
        int32_t ret = PasteboardClient::GetInstance()->GetUnifiedData(*context->unifiedData);
        if (ret == static_cast<int32_t>(PasteboardError::E_IS_BEGING_PROCESSED)) {
            context->SetErrInfo(ret, "Another getData is being processed");
        } else {
            context->status = napi_ok;
        }
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetUnifiedData End");
    };
    // 0: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, context, 0);
    return asyncCall.Call(env, exec);
}

napi_value SystemPasteboardNapi::GetUnifiedDataSync(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi GetUnifiedDataSync is called!");
    napi_value instance = nullptr;
    std::shared_ptr<UDMF::UnifiedData> unifiedData = std::make_shared<UDMF::UnifiedData>();

    NAPI_CALL(env, UDMF::UnifiedDataNapi::NewInstance(env, unifiedData, instance));
    UDMF::UnifiedDataNapi* obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void**>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        return nullptr;
    }
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, &obj]() mutable {
        auto ret = PasteboardClient::GetInstance()->GetUnifiedData(*(obj->value_));
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, GetUnifiedDataSync failed.");
        return nullptr;
    }

    if (*value != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "operate invalid, GetUnifiedDataSync failed");
        return nullptr;
    }
    return instance;
}

napi_value SystemPasteboardNapi::SetUnifiedDataSync(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi SetUnifiedDataSync is called!");
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    if (!CheckExpression(
        env, argc > 0, JSErrorCode::INVALID_PARAMETERS, "Parameter error. Wrong number of arguments.")) {
        return nullptr;
    }
    UDMF::UnifiedDataNapi* unifiedDataNapi = nullptr;
    napi_unwrap(env, argv[0], reinterpret_cast<void**>(&unifiedDataNapi));
    if (!CheckExpression(env, unifiedDataNapi != nullptr,
        JSErrorCode::INVALID_PARAMETERS, "Parameter error. The Type of data must be unifiedData.")) {
        return nullptr;
    }
    auto properties = unifiedDataNapi->GetPropertiesNapi(env);
    bool isDelay = false;
    std::shared_ptr<PasteboardDelayGetterInstance> delayGetter = nullptr;
    if (properties != nullptr && properties->delayDataRef_ != nullptr) {
        delayGetter = std::make_shared<PasteboardDelayGetterInstance>(env, properties->delayDataRef_);
        delayGetter->GetStub()->SetDelayGetterWrapper(delayGetter);
        isDelay = true;
    }
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, unifiedDataNapi, isDelay, delayGetter]() mutable {
        int32_t ret = isDelay ?
            PasteboardClient::GetInstance()->SetUnifiedData(*(unifiedDataNapi->value_), delayGetter->GetStub()) :
            PasteboardClient::GetInstance()->SetUnifiedData(*(unifiedDataNapi->value_));
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, SetUnifiedDataSync failed.");
        return nullptr;
    }

    if (*value != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "operate invalid, SetUnifiedDataSync failed");
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> lck(delayMutex_);
        delayGetter_ = delayGetter;
    }
    return nullptr;
}

napi_value SystemPasteboardNapi::SetAppShareOptions(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {0};
    napi_value thisArg = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisArg, &data));
    if (!CheckExpression(env, argc > 0, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. Mandatory parameters are left unspecified.")) {
        return nullptr;
    }
    int32_t shareOptions;
    auto status = napi_get_value_int32(env, argv[0], &shareOptions);
    if (!CheckExpression(env, status == napi_ok, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. Incorrect parameter types.")) {
        return nullptr;
    }
    if (!CheckExpression(env, shareOptions >= ShareOption::InApp && shareOptions <= ShareOption::CrossDevice,
        JSErrorCode::INVALID_PARAMETERS, "Parameter error. Parameter verification failed.")) {
        return nullptr;
    }
    auto result = PasteboardClient::GetInstance()->SetAppShareOptions(static_cast<ShareOption>(shareOptions));
    if (!CheckExpression(env, result != static_cast<int32_t>(PasteboardError::E_NO_PERMISSION),
        JSErrorCode::NO_SYSTEM_PERMISSION,
        "Permission verification failed. A non-system application calls a system API.")) {
        return nullptr;
    }
    if (!CheckExpression(env, result != static_cast<int32_t>(PasteboardError::E_INVALID_OPERATION),
        JSErrorCode::SETTINGS_ALREADY_EXIST, "Settings already exist.")) {
        return nullptr;
    }
    return nullptr;
}

napi_value SystemPasteboardNapi::RemoveAppShareOptions(napi_env env, napi_callback_info info)
{
    auto result = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    if (CheckExpression(env, result != static_cast<int32_t>(PasteboardError::E_NO_PERMISSION),
        JSErrorCode::NO_SYSTEM_PERMISSION,
        "Permission verification failed. A non-system application calls a system API.")) {
        return nullptr;
    }
    return nullptr;
}

void SystemPasteboardNapi::SetDataCommon(std::shared_ptr<SetUnifiedContextInfo>& context)
{
    auto input = [context](napi_env env, size_t argc, napi_value* argv, napi_value self) -> napi_status {
        // setData has 1 arg
        if (!CheckExpression(
            env, argc > 0, JSErrorCode::INVALID_PARAMETERS, "Parameter error. Wrong number of arguments.")) {
            return napi_invalid_arg;
        }
        UDMF::UnifiedDataNapi* unifiedDataNapi = nullptr;
        context->status = napi_unwrap(env, argv[0], reinterpret_cast<void**>(&unifiedDataNapi));
        if (!CheckExpression(env, unifiedDataNapi != nullptr,
            JSErrorCode::INVALID_PARAMETERS, "Parameter error. The Type of data must be unifiedData.")) {
            return napi_invalid_arg;
        }
        context->obj = unifiedDataNapi->value_;
        auto properties = unifiedDataNapi->GetPropertiesNapi(env);
        if (properties != nullptr && properties->delayDataRef_ != nullptr) {
            context->delayGetter = std::make_shared<PasteboardDelayGetterInstance>(env, properties->delayDataRef_);
            context->delayGetter->GetStub()->SetDelayGetterWrapper(context->delayGetter);
            context->isDelay = true;
        }
        return napi_ok;
    };
    auto output = [context](napi_env env, napi_value *result) -> napi_status {
        if (context->status == napi_ok) {
            std::lock_guard<std::mutex> lck(delayMutex_);
            delayGetter_ = std::move(context->delayGetter);
        }
        return napi_ok;
    };
    context->SetAction(std::move(input), std::move(output));
}

void SystemPasteboardNapi::GetDataCommon(std::shared_ptr<GetUnifiedContextInfo>& context)
{
    auto input = [](napi_env env, size_t argc, napi_value* argv, napi_value self) -> napi_status {
        // 1: GetPasteData has 0 or 1 args
        if (argc > 0 &&
            !CheckArgsType(env, argv[0], napi_function, "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        return napi_ok;
    };

    auto output = [context](napi_env env, napi_value* result) -> napi_status {
        napi_value instance = nullptr;
        std::shared_ptr<UDMF::UnifiedData> unifiedData = std::make_shared<UDMF::UnifiedData>();
        UDMF::UnifiedDataNapi::NewInstance(env, unifiedData, instance);

        UDMF::UnifiedDataNapi* obj = nullptr;
        napi_status ret = napi_unwrap(env, instance, reinterpret_cast<void**>(&obj));
        if ((ret == napi_ok) || (obj != nullptr)) {
            obj->value_ = context->unifiedData;
        } else {
            return napi_generic_failure;
        }
        *result = instance;
        return napi_ok;
    };
    context->SetAction(std::move(input), std::move(output));
}

napi_value SystemPasteboardNapi::IsRemoteData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi IsRemoteData() is called!");
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block]() {
        auto ret = PasteboardClient::GetInstance()->IsRemoteData();
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "value=%{public}d", ret);
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "Request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, IsRemoteData failed.");
        return nullptr;
    }
    napi_value result = nullptr;
    napi_get_boolean(env, *value, &result);
    return result;
}

napi_value SystemPasteboardNapi::GetDataSource(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi GetDataSource() is called!");
    std::string bundleName;
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, &bundleName]() mutable {
        auto ret = PasteboardClient::GetInstance()->GetDataSource(bundleName);
        std::shared_ptr<int> value = std::make_shared<int>(ret);
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "Request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, GetDataSource failed.");
        return nullptr;
    }

    if (*value != static_cast<int>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetDataSource, failed, ret = %{public}d", *value);
        return nullptr;
    }
    napi_value result = nullptr;
    napi_create_string_utf8(env, bundleName.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value SystemPasteboardNapi::HasDataType(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi HasDataType() is called!");
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    if ((!CheckExpression(env, argc >= ARGC_TYPE_SET1, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments must be grater than zero.")) ||
        (!CheckArgsType(env, argv[0], napi_string, "Parameter error. The type of mimeType must be string."))) {
        return nullptr;
    }

    std::string mimeType;
    if (!GetValue(env, argv[0], mimeType)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to GetValue!");
        return nullptr;
    }
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, mimeType]() {
        auto ret = PasteboardClient::GetInstance()->HasDataType(mimeType);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "ret = %{public}d", ret);
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "Request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, HasDataType failed.");
        return nullptr;
    }
    napi_value result = nullptr;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "value = %{public}d", *value);
    napi_get_boolean(env, *value, &result);
    return result;
}

napi_value SystemPasteboardNapi::ClearDataSync(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi ClearDataSync() is called!");
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block]() {
        PasteboardClient::GetInstance()->Clear();
        std::shared_ptr<int> value = std::make_shared<int>(0);
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "Request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, ClearDataSync failed.");
    }
    return nullptr;
}

napi_value SystemPasteboardNapi::GetDataSync(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi GetDataSync() is called!");
    napi_value instance = nullptr;
    NAPI_CALL(env, PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        return nullptr;
    }
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, &obj]() mutable {
        auto ret = PasteboardClient::GetInstance()->GetPasteData(*(obj->value_));
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, GetDataSync failed.");
        return nullptr;
    }

    if (*value != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "operate invalid, GetDataSync failed");
        return nullptr;
    }

    return instance;
}

napi_value SystemPasteboardNapi::SetDataSync(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi SetDataSync() is called!");
    size_t argc = 1;
    napi_value argv[1] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    if (!CheckExpression(env, argc > 0, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments must be one.") ||
        !CheckExpression(env, PasteDataNapi::IsPasteData(env, argv[0]), JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The Type of data must be pasteData.")) {
        return nullptr;
    }

    PasteDataNapi *pasteData = nullptr;
    napi_unwrap(env, argv[0], reinterpret_cast<void **>(&pasteData));
    if (pasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to GetValue!");
        return nullptr;
    }
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, pasteData]() {
        auto ret = PasteboardClient::GetInstance()->SetPasteData(*(pasteData->value_));
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, SetDataSync failed.");
        return nullptr;
    }

    if (*value != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "operate invalid, SetDataSync failed");
        return nullptr;
    }
    return nullptr;
}

napi_value SystemPasteboardNapi::HasDataSync(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "SystemPasteboardNapi HasDataSync() is called!");
    auto block = std::make_shared<BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block]() {
        auto ret = PasteboardClient::GetInstance()->HasPasteData();
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (!CheckExpression(env, value != nullptr, JSErrorCode::REQUEST_TIME_OUT, "request time out.")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "time out, HasDataSync failed.");
        return nullptr;
    }
    napi_value result = nullptr;
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "value=%{public}d", *value);
    napi_get_boolean(env, *value, &result);
    return result;
}

napi_value SystemPasteboardNapi::SystemPasteboardInit(napi_env env, napi_value exports)
{
    napi_status status = napi_ok;
    napi_property_descriptor descriptors[] = {
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("clear", Clear),
        DECLARE_NAPI_FUNCTION("getPasteData", GetPasteData),
        DECLARE_NAPI_FUNCTION("hasPasteData", HasPasteData),
        DECLARE_NAPI_FUNCTION("setPasteData", SetPasteData),
        DECLARE_NAPI_FUNCTION("clearData", ClearData),
        DECLARE_NAPI_FUNCTION("getData", GetData),
        DECLARE_NAPI_FUNCTION("hasData", HasData),
        DECLARE_NAPI_FUNCTION("setData", SetData),
        DECLARE_NAPI_FUNCTION("isRemoteData", IsRemoteData),
        DECLARE_NAPI_FUNCTION("getDataSource", GetDataSource),
        DECLARE_NAPI_FUNCTION("hasDataType", HasDataType),
        DECLARE_NAPI_FUNCTION("clearDataSync", ClearDataSync),
        DECLARE_NAPI_FUNCTION("getDataSync", GetDataSync),
        DECLARE_NAPI_FUNCTION("hasDataSync", HasDataSync),
        DECLARE_NAPI_FUNCTION("setDataSync", SetDataSync),
        DECLARE_NAPI_FUNCTION("setUnifiedData", SetUnifiedData),
        DECLARE_NAPI_FUNCTION("getUnifiedData", GetUnifiedData),
        DECLARE_NAPI_FUNCTION("setUnifiedDataSync", SetUnifiedDataSync),
        DECLARE_NAPI_FUNCTION("getUnifiedDataSync", GetUnifiedDataSync),
        DECLARE_NAPI_FUNCTION("setAppShareOptions", SetAppShareOptions),
        DECLARE_NAPI_FUNCTION("removeAppShareOptions", RemoveAppShareOptions),

    };
    napi_value constructor;
    napi_define_class(env, "SystemPasteboard", NAPI_AUTO_LENGTH, New, nullptr,
        sizeof(descriptors) / sizeof(napi_property_descriptor), descriptors, &constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to define class at SystemPasteboardInit");
        return nullptr;
    }
    napi_create_reference(env, constructor, 1, &g_systemPasteboard);
    status = napi_set_named_property(env, exports, "SystemPasteboard", constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Set property failed when SystemPasteboardInit");
        return nullptr;
    }
    return exports;
}

SystemPasteboardNapi::SystemPasteboardNapi() : env_(nullptr)
{
    value_ = std::make_shared<PasteDataNapi>();
}

SystemPasteboardNapi::~SystemPasteboardNapi()
{
}

void SystemPasteboardNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Destructor");
    SystemPasteboardNapi *obj = static_cast<SystemPasteboardNapi *>(nativeObject);
    delete obj;
}

napi_value SystemPasteboardNapi::New(napi_env env, napi_callback_info info)
{
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "proc.");
    // get native object
    SystemPasteboardNapi *obj = new (std::nothrow) SystemPasteboardNapi();
    if (!obj) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "New obj is null");
        return nullptr;
    }
    obj->env_ = env;
    NAPI_CALL(env, napi_wrap(env, thisVar, obj, SystemPasteboardNapi::Destructor,
                       nullptr, // finalize_hint
                       nullptr));
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return thisVar;
}

napi_status SystemPasteboardNapi::NewInstance(napi_env env, napi_value &instance)
{
    napi_status status;
    if (g_systemPasteboard_instance != nullptr) {
        status = napi_get_reference_value(env, g_systemPasteboard_instance, &instance);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "get instance failed");
            return status;
        }
        return napi_ok;
    }

    napi_value constructor;
    status = napi_get_reference_value(env, g_systemPasteboard, &constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "get reference failed");
        return status;
    }

    status = napi_new_instance(env, constructor, 0, nullptr, &instance);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "new instance failed");
        return status;
    }
    napi_create_reference(env, instance, 1, &g_systemPasteboard_instance);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "new instance ok");

    return napi_ok;
}

std::shared_ptr<PasteboardObserverInstance> SystemPasteboardNapi::GetObserver(napi_env env, napi_value observer)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetObserver start");
    for (auto &[refKey, observerValue] : observers_) {
        napi_value callback = nullptr;
        napi_get_reference_value(env, refKey, &callback);
        bool isEqual = false;
        napi_strict_equals(env, observer, callback, &isEqual);
        if (isEqual) {
            return observerValue;
        }
    }
    return nullptr;
}

void SystemPasteboardNapi::DeleteObserver(const std::shared_ptr<PasteboardObserverInstance> &observer)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "observer == null: %{public}d, size: %{public}zu",
        observer == nullptr, observers_.size());
    std::vector<std::shared_ptr<PasteboardObserverInstance>> observers;
    {
        for (auto it = observers_.begin(); it != observers_.end();) {
            if (it->second == observer) {
                observers.push_back(observer);
                observers_.erase(it++);
                break;
            }
            if (observer == nullptr) {
                observers.push_back(it->second);
                observers_.erase(it++);
            } else {
                it++;
            }
        }
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "delete observer size: %{public}zu", observers.size());
    for (auto &delObserver : observers) {
        PasteboardClient::GetInstance()->RemovePasteboardChangedObserver(delObserver->GetStub());
    }
}

void PasteboardObserverInstance::PasteboardObserverImpl::OnPasteboardChanged()
{
    std::shared_ptr<PasteboardObserverInstance> observerInstance(wrapper_.lock());
    if (observerInstance == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "expired callback");
        return;
    }
    observerInstance->OnPasteboardChanged();
}

void PasteboardObserverInstance::PasteboardObserverImpl::SetObserverWrapper(
    const std::shared_ptr<PasteboardObserverInstance> &observerInstance)
{
    wrapper_ = observerInstance;
}

void PasteboardDelayGetterInstance::PasteboardDelayGetterImpl::GetUnifiedData(
    const std::string &type, UDMF::UnifiedData &data)
{
    std::shared_ptr<PasteboardDelayGetterInstance> delayGetterInstance(wrapper_.lock());
    if (delayGetterInstance == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "no delay getter");
        return;
    }
    delayGetterInstance->GetUnifiedData(type, data);
}

void PasteboardDelayGetterInstance::PasteboardDelayGetterImpl::GetPasteData(
    const std::string &type, MiscServices::PasteData &data)
{
}

void PasteboardDelayGetterInstance::PasteboardDelayGetterImpl::SetDelayGetterWrapper(
    const std::shared_ptr<PasteboardDelayGetterInstance> delayGetterInstance)
{
    wrapper_ = delayGetterInstance;
}
} // namespace MiscServicesNapi
} // namespace OHOS