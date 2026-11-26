/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include "pasteboard_napi.h"
#include "pastedata_record_napi.h"
#include "pasteboard_fuse_getter_client.h"
#include "paste_data_record.h"
#include "pasteboard_virtual_entry.h"
#include "uri.h"
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <limits>
#include <memory>
#include <mutex>
using namespace OHOS::MiscServices;
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServicesNapi {

constexpr size_t MAX_VIRTUAL_FILE_ID_BYTES = 1024;
constexpr size_t MAX_VIRTUAL_FILE_NAME_BYTES = 255;
constexpr size_t MAX_CONCURRENT_FUSE_READS = 8;
constexpr size_t READ_ARG_COUNT = 3;
constexpr size_t PROMISE_HANDLER_COUNT = 2;

static bool IsValidFileId(const std::string &id)
{
    return !id.empty() && id.size() <= MAX_VIRTUAL_FILE_ID_BYTES && id.find('\0') == std::string::npos;
}

static bool IsValidFileName(const std::string &name)
{
    return !name.empty() && name.size() <= MAX_VIRTUAL_FILE_NAME_BYTES && name != "." && name != ".." &&
           name.find_first_of("/\\") == std::string::npos && name.find('\0') == std::string::npos;
}

static bool ParseVirtualFileSize(napi_env env, napi_value value, int64_t &size)
{
    napi_valuetype vt = napi_undefined;
    napi_typeof(env, value, &vt);
    if (vt == napi_undefined) {
        size = 0;
        return true;
    }
    double number = 0;
    const double upperBound = -static_cast<double>(std::numeric_limits<int64_t>::min());
    if (napi_get_value_double(env, value, &number) != napi_ok || !std::isfinite(number) || number < 0 ||
        number >= upperBound || std::trunc(number) != number) {
        return false;
    }
    size = static_cast<int64_t>(number);
    return true;
}

constexpr uint32_t MAX_VIRTUAL_FILE_DEPTH = 64;
constexpr size_t MAX_VIRTUAL_FILE_COUNT = 4096;

struct VirtualFileEntry {
    std::string id;
    std::string name;
    int64_t size = -1;
    double modifyTime = 0;
    int32_t type = 0;
    std::vector<VirtualFileEntry> subItems;

    bool IsFile() const { return type == 0; }
    bool IsDir() const { return type == 1; }

    const VirtualFileEntry *FindById(const std::string &targetId) const
    {
        if (IsFile() && id == targetId) {
            return this;
        }
        for (const auto &child : subItems) {
            if (auto *found = child.FindById(targetId); found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }

    bool HasValidFileSizes(uint32_t depth = 0) const
    {
        if (depth > MAX_VIRTUAL_FILE_DEPTH) {
            return false;
        }
        if (IsFile()) {
            return size >= 0;
        }
        for (const auto &child : subItems) {
            if (!child.HasValidFileSizes(depth + 1)) {
                return false;
            }
        }
        return true;
    }
};

static FuseNode ToFuseNode(const VirtualFileEntry &entry)
{
    FuseNode node;
    node.id = entry.id;
    node.name = entry.name;
    node.type = entry.type;
    node.size = entry.size;
    node.modifyTime = static_cast<int64_t>(entry.modifyTime);
    for (const auto &child : entry.subItems) {
        node.children.push_back(ToFuseNode(child));
    }
    return node;
}

struct FuseReadContext {
    std::string id;
    int64_t offset;
    int32_t length;
    std::vector<uint8_t> data;
    int32_t result = ERR_INVALID_VALUE;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
};

struct FuseStatContext {
    std::string id;
    FuseFileStat stat;
    int32_t result = ERR_INVALID_VALUE;
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
};

struct FuseCancelContext {
    std::vector<std::string> idList;
};

enum class FuseOperation { READ, STAT, CANCEL };

struct FuseCallData {
    FuseOperation op;
    std::weak_ptr<FuseReadContext> readCtx;
    std::weak_ptr<FuseStatContext> statCtx;
    FuseCancelContext *cancelCtx = nullptr;
};

struct FuseTsfnContext {
    napi_env env = nullptr;
    napi_ref onRead = nullptr;
    napi_ref onStat = nullptr;
    napi_ref onCancel = nullptr;
};

static std::atomic<size_t> g_activeFuseReads{0};

class FuseReadSlotGuard final {
public:
    FuseReadSlotGuard() = default;
    ~FuseReadSlotGuard()
    {
        g_activeFuseReads.fetch_sub(1, std::memory_order_release);
    }
    static bool TryAcquire()
    {
        size_t current = g_activeFuseReads.load(std::memory_order_acquire);
        while (current < MAX_CONCURRENT_FUSE_READS) {
            if (g_activeFuseReads.compare_exchange_weak(current, current + 1, std::memory_order_acq_rel)) {
                return true;
            }
        }
        return false;
    }
    FuseReadSlotGuard(const FuseReadSlotGuard &) = delete;
    FuseReadSlotGuard &operator=(const FuseReadSlotGuard &) = delete;
};

static void FuseTsfnFinalizer(napi_env env, void *finalizeData, void *finalizeHint)
{
    (void)env;
    (void)finalizeHint;
    auto *ctx = static_cast<FuseTsfnContext *>(finalizeData);
    if (ctx != nullptr) {
        if (ctx->onRead != nullptr) {
            napi_delete_reference(env, ctx->onRead);
        }
        if (ctx->onStat != nullptr) {
            napi_delete_reference(env, ctx->onStat);
        }
        if (ctx->onCancel != nullptr) {
            napi_delete_reference(env, ctx->onCancel);
        }
        delete ctx;
    }
}

class NapiFuseGetter final : public PasteboardFuseGetterClient {
public:
    NapiFuseGetter(napi_env env, napi_ref onRead, napi_ref onStat, napi_ref onCancel,
        const VirtualFileEntry &rootEntry)
        : PasteboardFuseGetterClient({}), env_(env), rootEntry_(rootEntry)
    {
        auto *tsfnCtx = new FuseTsfnContext{env, onRead, onStat, onCancel};
        napi_value name = nullptr;
        napi_create_string_utf8(env, "PasteboardFuseGetter", NAPI_AUTO_LENGTH, &name);
        napi_status status = napi_create_threadsafe_function(env, nullptr, nullptr, name, 0, 1, tsfnCtx,
            FuseTsfnFinalizer, tsfnCtx, CallJs, &tsfn_);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "create threadsafe function failed");
            delete tsfnCtx;
            tsfn_ = nullptr;
        }
    }
    ~NapiFuseGetter()
    {
        if (tsfn_ != nullptr) {
            napi_release_threadsafe_function(tsfn_, napi_tsfn_abort);
            tsfn_ = nullptr;
        }
    }
    NapiFuseGetter(const NapiFuseGetter &) = delete;
    NapiFuseGetter &operator=(const NapiFuseGetter &) = delete;
    NapiFuseGetter(NapiFuseGetter &&) = delete;
    NapiFuseGetter &operator=(NapiFuseGetter &&) = delete;

    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override
    {
        (void)recordId;
        if (tsfn_ == nullptr) {
            return ERR_INVALID_VALUE;
        }
        if (!FuseReadSlotGuard::TryAcquire()) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "OnRead concurrency limit reached");
            return ERR_INVALID_VALUE;
        }
        FuseReadSlotGuard slotGuard;
        auto ctx = std::make_shared<FuseReadContext>();
        ctx->id = id;
        ctx->offset = offset;
        ctx->length = length;
        auto *callData = new FuseCallData{FuseOperation::READ, std::weak_ptr<FuseReadContext>(ctx), {}, nullptr};
        napi_status status = napi_acquire_threadsafe_function(tsfn_);
        if (status != napi_ok) {
            delete callData;
            return ERR_INVALID_VALUE;
        }
        status = napi_call_threadsafe_function(tsfn_, callData, napi_tsfn_blocking);
        napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
        if (status != napi_ok) {
            delete callData;
            return ERR_INVALID_VALUE;
        }
        {
            std::unique_lock<std::mutex> lock(ctx->mtx);
            ctx->cv.wait_for(lock, fuseCallbackTimeout, [&ctx]() { return ctx->done; });
        }
        if (!ctx->done) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "OnRead timeout, id:%{public}s", id.c_str());
            return ERR_INVALID_VALUE;
        }
        data = std::move(ctx->data);
        return ctx->result;
    }
    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override
    {
        (void)recordId;
        const auto *entry = rootEntry_.FindById(id);
        if (entry != nullptr) {
            stat.id = entry->id;
            stat.name = entry->name;
            stat.size = entry->size;
            stat.modifyTime = static_cast<int64_t>(entry->modifyTime);
            stat.type = entry->type;
            return ERR_OK;
        }
        if (tsfn_ == nullptr) {
            return ERR_INVALID_VALUE;
        }
        auto ctx = std::make_shared<FuseStatContext>();
        ctx->id = id;
        auto *callData = new FuseCallData{FuseOperation::STAT, {}, std::weak_ptr<FuseStatContext>(ctx), nullptr};
        napi_status status = napi_acquire_threadsafe_function(tsfn_);
        if (status != napi_ok) {
            delete callData;
            return ERR_INVALID_VALUE;
        }
        status = napi_call_threadsafe_function(tsfn_, callData, napi_tsfn_blocking);
        napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
        if (status != napi_ok) {
            delete callData;
            return ERR_INVALID_VALUE;
        }
        {
            std::unique_lock<std::mutex> lock(ctx->mtx);
            ctx->cv.wait_for(lock, fuseCallbackTimeout, [&ctx]() { return ctx->done; });
        }
        if (!ctx->done) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "OnStat timeout, id:%{public}s", id.c_str());
            return ERR_INVALID_VALUE;
        }
        stat = std::move(ctx->stat);
        return ctx->result;
    }
    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override
    {
        (void)recordId;
        if (tsfn_ == nullptr) {
            return ERR_INVALID_VALUE;
        }
        auto *cancelCtx = new FuseCancelContext{idList};
        auto *callData = new FuseCallData{FuseOperation::CANCEL, {}, {}, cancelCtx};
        napi_status status = napi_acquire_threadsafe_function(tsfn_);
        if (status != napi_ok) {
            delete callData;
            delete cancelCtx;
            return ERR_INVALID_VALUE;
        }
        status = napi_call_threadsafe_function(tsfn_, callData, napi_tsfn_nonblocking);
        napi_release_threadsafe_function(tsfn_, napi_tsfn_release);
        if (status != napi_ok) {
            delete callData;
            delete cancelCtx;
            return ERR_INVALID_VALUE;
        }
        return ERR_OK;
    }
private:
    static constexpr auto fuseCallbackTimeout = std::chrono::seconds(5);

    static bool SignalDone(FuseReadContext *ctx)
    {
        {
            std::lock_guard<std::mutex> lock(ctx->mtx);
            if (ctx->done) {
                return false;
            }
            ctx->done = true;
        }
        ctx->cv.notify_one();
        return true;
    }

    static bool SignalDone(FuseStatContext *ctx)
    {
        {
            std::lock_guard<std::mutex> lock(ctx->mtx);
            if (ctx->done) {
                return false;
            }
            ctx->done = true;
        }
        ctx->cv.notify_one();
        return true;
    }

    static napi_value GetCallback(napi_env env, napi_ref ref)
    {
        if (ref == nullptr) {
            return nullptr;
        }
        napi_value cb = nullptr;
        napi_get_reference_value(env, ref, &cb);
        return cb;
    }

    static void CallJs(napi_env env, napi_value jsCallback, void *context, void *data)
    {
        (void)jsCallback;
        auto *tsfnCtx = static_cast<FuseTsfnContext *>(context);
        auto *callData = static_cast<FuseCallData *>(data);
        if (callData == nullptr) {
            return;
        }
        auto callDataGuard = std::unique_ptr<FuseCallData>(callData);
        if (env == nullptr || tsfnCtx == nullptr) {
            HandleTeardown(callData);
            return;
        }
        switch (callData->op) {
            case FuseOperation::READ:
                CallJsOnRead(env, tsfnCtx, callData);
                break;
            case FuseOperation::STAT:
                CallJsOnStat(env, tsfnCtx, callData);
                break;
            case FuseOperation::CANCEL:
                CallJsOnCancel(env, tsfnCtx, callData);
                break;
        }
    }

    static void HandleTeardown(FuseCallData *callData)
    {
        if (callData->op == FuseOperation::READ) {
            auto readCtx = callData->readCtx.lock();
            if (readCtx != nullptr) {
                SignalDone(readCtx.get());
            }
        } else if (callData->op == FuseOperation::STAT) {
            auto statCtx = callData->statCtx.lock();
            if (statCtx != nullptr) {
                SignalDone(statCtx.get());
            }
        } else if (callData->op == FuseOperation::CANCEL) {
            delete callData->cancelCtx;
            callData->cancelCtx = nullptr;
        }
    }

    static void CallJsOnRead(napi_env env, FuseTsfnContext *tsfnCtx, FuseCallData *callData)
    {
        auto ctx = callData->readCtx.lock();
        if (ctx == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CallJsOnRead: caller already timed out");
            return;
        }
        napi_value onRead = GetCallback(env, tsfnCtx->onRead);
        if (onRead == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CallJsOnRead: onRead callback null");
            SignalDone(ctx.get());
            return;
        }
        napi_value args[3] = { nullptr };
        napi_create_string_utf8(env, ctx->id.c_str(), NAPI_AUTO_LENGTH, &args[0]);
        napi_create_int64(env, ctx->offset, &args[1]);
        napi_create_int32(env, ctx->length, &args[READ_ARG_COUNT - 1]);
        napi_value jsResult = nullptr;
        napi_status status = napi_call_function(env, nullptr, onRead, 3, args, &jsResult);
        if (status != napi_ok || jsResult == nullptr) {
            SignalDone(ctx.get());
            return;
        }
        bool isPromise = false;
        napi_is_promise(env, jsResult, &isPromise);
        if (isPromise) {
            AttachPromiseHandlers(env, jsResult, ctx);
            return;
        }
        CompleteRead(env, jsResult, ctx.get());
    }

    static void CallJsOnStat(napi_env env, FuseTsfnContext *tsfnCtx, FuseCallData *callData)
    {
        auto ctx = callData->statCtx.lock();
        if (ctx == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CallJsOnStat: caller already timed out");
            return;
        }
        napi_value onStat = GetCallback(env, tsfnCtx->onStat);
        if (onStat == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CallJsOnStat: onStat callback null");
            SignalDone(ctx.get());
            return;
        }
        napi_value arg = nullptr;
        napi_create_string_utf8(env, ctx->id.c_str(), NAPI_AUTO_LENGTH, &arg);
        napi_value jsResult = nullptr;
        napi_status status = napi_call_function(env, nullptr, onStat, 1, &arg, &jsResult);
        if (status != napi_ok || jsResult == nullptr) {
            SignalDone(ctx.get());
            return;
        }
        bool isPromise = false;
        napi_is_promise(env, jsResult, &isPromise);
        if (isPromise) {
            AttachPromiseHandlers(env, jsResult, ctx);
            return;
        }
        CompleteStat(env, jsResult, ctx.get());
    }

    static void CallJsOnCancel(napi_env env, FuseTsfnContext *tsfnCtx, FuseCallData *callData)
    {
        auto *cancelCtx = callData->cancelCtx;
        if (cancelCtx == nullptr) {
            return;
        }
        napi_value onCancel = GetCallback(env, tsfnCtx->onCancel);
        if (onCancel == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CallJsOnCancel: onCancel callback null");
            delete cancelCtx;
            callData->cancelCtx = nullptr;
            return;
        }
        napi_value jsIdList = nullptr;
        napi_create_array_with_length(env, cancelCtx->idList.size(), &jsIdList);
        for (size_t i = 0; i < cancelCtx->idList.size(); i++) {
            napi_value elem = nullptr;
            napi_create_string_utf8(env, cancelCtx->idList[i].c_str(), NAPI_AUTO_LENGTH, &elem);
            napi_set_element(env, jsIdList, i, elem);
        }
        napi_value promise = nullptr;
        napi_call_function(env, nullptr, onCancel, 1, &jsIdList, &promise);
        if (promise != nullptr) {
            bool isPromise = false;
            napi_is_promise(env, promise, &isPromise);
            if (isPromise) {
                napi_value catchValue = nullptr;
                napi_get_named_property(env, promise, "catch", &catchValue);
                napi_value catchFunc = nullptr;
                napi_create_function(env, "onRejected", NAPI_AUTO_LENGTH,
                    [](napi_env env, napi_callback_info info) -> napi_value {
                        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "onCancel Promise rejected");
                        return nullptr;
                    }, nullptr, &catchFunc);
                napi_call_function(env, promise, catchValue, 1, &catchFunc, nullptr);
            }
        }
        delete cancelCtx;
        callData->cancelCtx = nullptr;
    }

    static void CompleteRead(napi_env env, napi_value value, FuseReadContext *ctx)
    {
        napi_typedarray_type type = napi_int8_array;
        size_t length = 0;
        void *bytes = nullptr;
        napi_value buffer = nullptr;
        size_t offset = 0;
        napi_status status = napi_get_typedarray_info(env, value, &type, &length, &bytes, &buffer, &offset);
        if (status == napi_ok && type == napi_uint8_array) {
            size_t maxLen = static_cast<size_t>(ctx->length);
            size_t copyLen = std::min(length, maxLen);
            if (copyLen > 0 && bytes != nullptr) {
                auto *begin = reinterpret_cast<uint8_t *>(bytes);
                ctx->data.assign(begin, begin + copyLen);
            }
            ctx->result = ERR_OK;
        } else {
            bool isArrayBuffer = false;
            napi_is_arraybuffer(env, value, &isArrayBuffer);
            if (isArrayBuffer) {
                void *bufData = nullptr;
                size_t bufLen = 0;
                napi_get_arraybuffer_info(env, value, &bufData, &bufLen);
                size_t maxLen = static_cast<size_t>(ctx->length);
                size_t copyLen = std::min(bufLen, maxLen);
                if (copyLen > 0 && bufData != nullptr) {
                    ctx->data.assign(reinterpret_cast<uint8_t *>(bufData),
                                     reinterpret_cast<uint8_t *>(bufData) + copyLen);
                }
                ctx->result = ERR_OK;
            }
        }
        SignalDone(ctx);
    }

    static void ParseStatType(napi_env env, napi_value jsType, int32_t &type)
    {
        napi_valuetype vt;
        napi_typeof(env, jsType, &vt);
        if (vt == napi_number) {
            int32_t typeVal = 0;
            napi_get_value_int32(env, jsType, &typeVal);
            type = typeVal;
        } else if (vt == napi_string) {
            size_t len = 0;
            constexpr size_t typeStrLen = 8;
            char typeStr[typeStrLen] = {0};
            napi_get_value_string_utf8(env, jsType, typeStr, typeStrLen - 1, &len);
            type = (strcmp(typeStr, "dir") == 0) ? 1 : 0;
        }
    }

    static void CompleteStat(napi_env env, napi_value jsResult, FuseStatContext *ctx)
    {
        napi_value jsId = nullptr;
        napi_value jsName = nullptr;
        napi_value jsSize = nullptr;
        napi_value jsModifyTime = nullptr;
        napi_value jsType = nullptr;
        napi_get_named_property(env, jsResult, "id", &jsId);
        napi_get_named_property(env, jsResult, "name", &jsName);
        napi_get_named_property(env, jsResult, "size", &jsSize);
        napi_get_named_property(env, jsResult, "modifyTime", &jsModifyTime);
        napi_get_named_property(env, jsResult, "type", &jsType);
        if (jsId != nullptr) {
            size_t len = 0;
            napi_get_value_string_utf8(env, jsId, nullptr, 0, &len);
            ctx->stat.id.resize(len);
            napi_get_value_string_utf8(env, jsId, ctx->stat.id.data(), len + 1, &len);
        }
        if (jsName != nullptr) {
            size_t len = 0;
            napi_get_value_string_utf8(env, jsName, nullptr, 0, &len);
            ctx->stat.name.resize(len);
            napi_get_value_string_utf8(env, jsName, ctx->stat.name.data(), len + 1, &len);
        }
        if (jsSize != nullptr) {
            int64_t size = 0;
            if (ParseVirtualFileSize(env, jsSize, size)) {
                ctx->stat.size = size;
            }
        }
        if (jsModifyTime != nullptr) {
            napi_get_value_int64(env, jsModifyTime, &ctx->stat.modifyTime);
        }
        if (jsType != nullptr) {
            ParseStatType(env, jsType, ctx->stat.type);
        }
        ctx->result = ERR_OK;
        SignalDone(ctx);
    }

    struct PromiseToken {
        std::shared_ptr<FuseReadContext> readCtx;
        std::shared_ptr<FuseStatContext> statCtx;
        bool isRead = false;
    };

    static void PromiseTokenFinalizer(napi_env env, void *data, void *hint)
    {
        (void)env;
        (void)hint;
        auto *token = static_cast<PromiseToken *>(data);
        delete token;
    }

    static void AttachPromiseHandlers(napi_env env, napi_value promise, std::shared_ptr<FuseReadContext> ctx)
    {
        auto *token = new PromiseToken{ctx, nullptr, true};
        napi_value thenFunc = nullptr;
        napi_create_function(env, "onFulfilled", NAPI_AUTO_LENGTH, OnReadPromiseResolved, token, &thenFunc);
        napi_add_finalizer(env, thenFunc, token, PromiseTokenFinalizer, nullptr, nullptr);
        napi_value catchFunc = nullptr;
        napi_create_function(env, "onRejected", NAPI_AUTO_LENGTH, OnPromiseRejected, token, &catchFunc);
        napi_value thenValue = nullptr;
        napi_get_named_property(env, promise, "then", &thenValue);
        napi_value handlers[] = { thenFunc, catchFunc };
        napi_call_function(env, promise, thenValue, PROMISE_HANDLER_COUNT, handlers, nullptr);
    }

    static void AttachPromiseHandlers(napi_env env, napi_value promise, std::shared_ptr<FuseStatContext> ctx)
    {
        auto *token = new PromiseToken{nullptr, ctx, false};
        napi_value thenFunc = nullptr;
        napi_create_function(env, "onFulfilled", NAPI_AUTO_LENGTH, OnStatPromiseResolved, token, &thenFunc);
        napi_add_finalizer(env, thenFunc, token, PromiseTokenFinalizer, nullptr, nullptr);
        napi_value catchFunc = nullptr;
        napi_create_function(env, "onRejected", NAPI_AUTO_LENGTH, OnPromiseRejected, token, &catchFunc);
        napi_value thenValue = nullptr;
        napi_get_named_property(env, promise, "then", &thenValue);
        napi_value handlers[] = { thenFunc, catchFunc };
        napi_call_function(env, promise, thenValue, PROMISE_HANDLER_COUNT, handlers, nullptr);
    }

    static napi_value OnReadPromiseResolved(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        napi_value thisVar = nullptr;
        void *data = nullptr;
        napi_get_cb_info(env, info, &argc, argv, &thisVar, &data);
        auto *token = static_cast<PromiseToken *>(data);
        if (token == nullptr || token->readCtx == nullptr) {
            return nullptr;
        }
        if (argv[0] != nullptr) {
            CompleteRead(env, argv[0], token->readCtx.get());
        } else {
            SignalDone(token->readCtx.get());
        }
        return nullptr;
    }

    static napi_value OnStatPromiseResolved(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        napi_value thisVar = nullptr;
        void *data = nullptr;
        napi_get_cb_info(env, info, &argc, argv, &thisVar, &data);
        auto *token = static_cast<PromiseToken *>(data);
        if (token == nullptr || token->statCtx == nullptr) {
            return nullptr;
        }
        if (argv[0] != nullptr) {
            CompleteStat(env, argv[0], token->statCtx.get());
        } else {
            SignalDone(token->statCtx.get());
        }
        return nullptr;
    }

    static napi_value OnPromiseRejected(napi_env env, napi_callback_info info)
    {
        size_t argc = 1;
        napi_value argv[1] = { nullptr };
        napi_value thisVar = nullptr;
        void *data = nullptr;
        napi_get_cb_info(env, info, &argc, argv, &thisVar, &data);
        auto *token = static_cast<PromiseToken *>(data);
        if (token == nullptr) {
            return nullptr;
        }
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Fuse callback promise rejected");
        if (token->isRead && token->readCtx != nullptr) {
            SignalDone(token->readCtx.get());
        } else if (!token->isRead && token->statCtx != nullptr) {
            SignalDone(token->statCtx.get());
        }
        return nullptr;
    }

    napi_env env_ = nullptr;
    napi_threadsafe_function tsfn_ = nullptr;
    VirtualFileEntry rootEntry_;
};

bool IsVirtualFileInfo(napi_env env, napi_value value)
{
    napi_valuetype type = napi_undefined;
    napi_typeof(env, value, &type);
    if (type != napi_object) {
        return false;
    }
    bool hasOnRead = false;
    bool hasItemInfo = false;
    napi_has_named_property(env, value, "onRead", &hasOnRead);
    napi_has_named_property(env, value, "itemInfo", &hasItemInfo);
    return hasOnRead && hasItemInfo;
}

static std::string ReadStringProperty(napi_env env, napi_value obj, const char *name)
{
    napi_value prop = nullptr;
    napi_get_named_property(env, obj, name, &prop);
    if (prop == nullptr) {
        return "";
    }
    size_t len = 0;
    napi_get_value_string_utf8(env, prop, nullptr, 0, &len);
    std::string value;
    value.resize(len);
    napi_get_value_string_utf8(env, prop, value.data(), len + 1, &len);
    return value;
}

static void ReadDateProperty(napi_env env, napi_value obj, const char *name, double &value)
{
    napi_value prop = nullptr;
    napi_get_named_property(env, obj, name, &prop);
    if (prop == nullptr) {
        return;
    }
    napi_valuetype vt;
    napi_typeof(env, prop, &vt);
    if (vt == napi_object) {
        bool isDate = false;
        napi_is_date(env, prop, &isDate);
        if (isDate) {
            napi_get_date_value(env, prop, &value);
        }
    }
}

static void ReadIntProperty(napi_env env, napi_value obj, const char *name, int32_t &value)
{
    napi_value prop = nullptr;
    napi_get_named_property(env, obj, name, &prop);
    if (prop == nullptr) {
        return;
    }
    napi_valuetype vt;
    napi_typeof(env, prop, &vt);
    if (vt == napi_number) {
        napi_get_value_int32(env, prop, &value);
    }
}

static bool ParseVirtualFile(napi_env env, napi_value obj, VirtualFileEntry &entry)
{
    entry.type = 0;
    entry.id = ReadStringProperty(env, obj, "id");
    if (!IsValidFileId(entry.id)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "fileId is invalid");
        napi_throw_error(env, "401", "Parameter error: fileId is invalid");
        return false;
    }
    napi_value jsSize = nullptr;
    napi_get_named_property(env, obj, "size", &jsSize);
    if (jsSize != nullptr) {
        napi_valuetype vt;
        napi_typeof(env, jsSize, &vt);
        if (vt == napi_number || vt == napi_bigint) {
            if (!ParseVirtualFileSize(env, jsSize, entry.size)) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "fileSize is invalid");
                napi_throw_error(env, "401", "Parameter error: fileSize is invalid");
                return false;
            }
        }
    }
    return true;
}

static bool ParseVirtualItem(napi_env env, napi_value obj, VirtualFileEntry &entry,
    uint32_t depth, uint32_t &itemCount);

static bool ParseVirtualDirectory(napi_env env, napi_value obj, VirtualFileEntry &entry,
    uint32_t depth, uint32_t &itemCount)
{
    entry.type = 1;
    napi_value subItems = nullptr;
    napi_get_named_property(env, obj, "subItems", &subItems);
    if (subItems == nullptr) {
        return true;
    }
    napi_valuetype vt;
    napi_typeof(env, subItems, &vt);
    if (vt == napi_undefined || vt == napi_null) {
        return true;
    }
    bool isArray = false;
    napi_is_array(env, subItems, &isArray);
    if (!isArray) {
        return false;
    }
    uint32_t length = 0;
    napi_get_array_length(env, subItems, &length);
    if (length > MAX_VIRTUAL_FILE_COUNT - itemCount) {
        return false;
    }
    entry.subItems.reserve(length);
    for (uint32_t i = 0; i < length; ++i) {
        napi_value item;
        napi_get_element(env, subItems, i, &item);
        VirtualFileEntry child;
        if (!ParseVirtualItem(env, item, child, depth + 1, itemCount)) {
            return false;
        }
        entry.subItems.emplace_back(std::move(child));
    }
    return true;
}

static bool ParseVirtualItem(napi_env env, napi_value obj, VirtualFileEntry &entry,
    uint32_t depth, uint32_t &itemCount)
{
    if (depth > MAX_VIRTUAL_FILE_DEPTH || itemCount >= MAX_VIRTUAL_FILE_COUNT) {
        return false;
    }
    ++itemCount;
    entry.name = ReadStringProperty(env, obj, "name");
    if (!IsValidFileName(entry.name)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "fileName is invalid");
        napi_throw_error(env, "401", "Parameter error: fileName is invalid");
        return false;
    }
    ReadDateProperty(env, obj, "modifyTime", entry.modifyTime);
    napi_value jsType = nullptr;
    napi_get_named_property(env, obj, "type", &jsType);
    if (jsType != nullptr) {
        napi_valuetype vt;
        napi_typeof(env, jsType, &vt);
        if (vt == napi_number) {
            napi_get_value_int32(env, jsType, &entry.type);
        } else if (vt == napi_string) {
            char typeStr[8] = {0};
            size_t len = 0;
            napi_get_value_string_utf8(env, jsType, typeStr, 7, &len);
            entry.type = (strcmp(typeStr, "dir") == 0) ? 1 : 0;
        }
    }
    if (entry.type == 0) {
        return ParseVirtualFile(env, obj, entry);
    }
    return entry.type == 1 && ParseVirtualDirectory(env, obj, entry, depth, itemCount);
}

static napi_value BuildVirtualFileResult(napi_env env, const std::string &mimeType,
    const VirtualFileEntry &info, napi_ref onReadRef, napi_ref onStatRef, napi_ref onCancelRef)
{
    auto provider = sptr<NapiFuseGetter>::MakeSptr(env, onReadRef, onStatRef, onCancelRef, info);
    if (provider == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "create NapiFuseGetter failed");
        return nullptr;
    }

    auto obj = std::make_shared<UDMF::Object>();
    obj->value_["id"] = info.id;
    obj->value_["name"] = info.name;
    obj->value_["size"] = info.size;
    obj->value_["modifyTime"] = static_cast<int64_t>(info.modifyTime);
    obj->value_["type"] = info.type;
    obj->value_["treeBlob"] = SerializeTree(ToFuseNode(info));

    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetUtdId("general.virtual-file");
    entry->SetMimeType(mimeType);
    entry->SetValue(std::shared_ptr<UDMF::Object>(obj));

    PasteDataRecord::Builder builder(mimeType);
    auto record = builder.Build();
    if (record == nullptr) {
        return nullptr;
    }
    record->AddEntry("general.virtual-file", entry);
    record->SetFuseGetter(provider);

    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddRecord(record);

    napi_value instance = nullptr;
    napi_status status = PasteDataNapi::NewInstance(env, instance);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "PasteDataNapi::NewInstance failed");
        return nullptr;
    }
    PasteDataNapi *objNapi = nullptr;
    napi_unwrap(env, instance, reinterpret_cast<void **>(&objNapi));
    if (objNapi == nullptr) {
        return nullptr;
    }
    objNapi->value_ = pasteData;
    return instance;
}

static void MakeVirtualFileRefs(napi_env env, napi_value onRead, napi_value onStat, napi_value onCancel,
    napi_ref &onReadRef, napi_ref &onStatRef, napi_ref &onCancelRef)
{
    napi_create_reference(env, onRead, 1, &onReadRef);
    if (onStat != nullptr) {
        napi_valuetype vt;
        napi_typeof(env, onStat, &vt);
        if (vt == napi_function) {
            napi_create_reference(env, onStat, 1, &onStatRef);
        }
    }
    napi_create_reference(env, onCancel, 1, &onCancelRef);
}

static bool BuildVirtualFileEntry(napi_env env, napi_value itemInfo, napi_value onRead, napi_value onStat,
    napi_value onCancel, const std::string &mimeType, std::shared_ptr<PasteDataEntry> &entry,
    sptr<NapiFuseGetter> &provider)
{
    VirtualFileEntry rootEntry;
    uint32_t itemCount = 0;
    if (!ParseVirtualItem(env, itemInfo, rootEntry, 0, itemCount)) {
        return false;
    }
    if (!rootEntry.HasValidFileSizes()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "fileSize is invalid");
        napi_throw_error(env, "401", "Parameter error: fileSize is invalid");
        return false;
    }
    napi_ref onReadRef = nullptr;
    napi_ref onStatRef = nullptr;
    napi_ref onCancelRef = nullptr;
    MakeVirtualFileRefs(env, onRead, onStat, onCancel, onReadRef, onStatRef, onCancelRef);
    provider = sptr<NapiFuseGetter>::MakeSptr(env, onReadRef, onStatRef, onCancelRef, rootEntry);
    if (provider == nullptr) {
        return false;
    }
    auto obj = std::make_shared<UDMF::Object>();
    obj->value_["id"] = rootEntry.id;
    obj->value_["name"] = rootEntry.name;
    obj->value_["size"] = rootEntry.size;
    obj->value_["modifyTime"] = static_cast<int64_t>(rootEntry.modifyTime);
    obj->value_["type"] = rootEntry.type;
    obj->value_["treeBlob"] = SerializeTree(ToFuseNode(rootEntry));
    entry = std::make_shared<PasteDataEntry>();
    entry->SetUtdId("general.virtual-file");
    entry->SetMimeType(mimeType);
    entry->SetValue(std::shared_ptr<UDMF::Object>(obj));
    return true;
}

static napi_value CreateVirtualFileData(napi_env env, const std::string &mimeType, napi_value vfiValue)
{
    napi_value itemInfo = nullptr;
    napi_get_named_property(env, vfiValue, "itemInfo", &itemInfo);
    napi_value onRead = nullptr;
    napi_value onStat = nullptr;
    napi_value onCancel = nullptr;
    napi_get_named_property(env, vfiValue, "onRead", &onRead);
    napi_get_named_property(env, vfiValue, "onStat", &onStat);
    napi_get_named_property(env, vfiValue, "onCancel", &onCancel);

    napi_valuetype onReadType = napi_undefined;
    napi_typeof(env, onRead, &onReadType);
    if (onReadType != napi_function) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "onRead is required and must be a function");
        napi_throw_error(env, "401", "Parameter error: onRead is required");
        return nullptr;
    }
    napi_valuetype itemInfoType = napi_undefined;
    napi_typeof(env, itemInfo, &itemInfoType);
    if (itemInfoType != napi_object) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "itemInfo is required and must be an object");
        napi_throw_error(env, "401", "Parameter error: itemInfo is required");
        return nullptr;
    }

    VirtualFileEntry rootEntry;
    uint32_t itemCount = 0;
    if (!ParseVirtualItem(env, itemInfo, rootEntry, 0, itemCount)) {
        return nullptr;
    }
    if (!rootEntry.HasValidFileSizes()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "fileSize is invalid");
        napi_throw_error(env, "401", "Parameter error: fileSize is invalid");
        return nullptr;
    }

    napi_ref onReadRef = nullptr;
    napi_ref onStatRef = nullptr;
    napi_ref onCancelRef = nullptr;
    MakeVirtualFileRefs(env, onRead, onStat, onCancel, onReadRef, onStatRef, onCancelRef);

    return BuildVirtualFileResult(env, mimeType, rootEntry, onReadRef, onStatRef, onCancelRef);
}
constexpr size_t MAX_ARGS = 6;
static napi_value CreateVirtualFileRecord(napi_env env, const std::string &mimeType, napi_value vfiValue)
{
    napi_value itemInfo = nullptr;
    napi_get_named_property(env, vfiValue, "itemInfo", &itemInfo);
    napi_value onRead = nullptr;
    napi_value onStat = nullptr;
    napi_value onCancel = nullptr;
    napi_get_named_property(env, vfiValue, "onRead", &onRead);
    napi_get_named_property(env, vfiValue, "onStat", &onStat);
    napi_get_named_property(env, vfiValue, "onCancel", &onCancel);
    napi_valuetype onReadType = napi_undefined;
    napi_typeof(env, onRead, &onReadType);
    if (onReadType != napi_function) {
        napi_throw_error(env, "401", "Parameter error: onRead is required");
        return nullptr;
    }
    napi_valuetype itemInfoType = napi_undefined;
    napi_typeof(env, itemInfo, &itemInfoType);
    if (itemInfoType != napi_object) {
        napi_throw_error(env, "401", "Parameter error: itemInfo is required");
        return nullptr;
    }
    std::shared_ptr<PasteDataEntry> entry;
    sptr<NapiFuseGetter> provider;
    if (!BuildVirtualFileEntry(env, itemInfo, onRead, onStat, onCancel, mimeType, entry, provider)) {
        return nullptr;
    }
    PasteDataRecord::Builder builder(mimeType);
    auto record = builder.Build();
    if (record == nullptr) {
        return nullptr;
    }
    record->AddEntry("general.virtual-file", entry);
    record->SetFuseGetter(provider);

    napi_value instance = nullptr;
    if (!PasteDataRecordNapi::NewInstanceByRecord(env, instance, record)) {
        return nullptr;
    }
    return instance;
}

bool AddVirtualFileEntry(napi_env env, std::shared_ptr<PasteDataRecord> record,
    const std::string &mimeType, napi_value vfiValue)
{
    if (record == nullptr) {
        return false;
    }
    napi_value itemInfo = nullptr;
    napi_get_named_property(env, vfiValue, "itemInfo", &itemInfo);
    napi_value onRead = nullptr;
    napi_value onStat = nullptr;
    napi_value onCancel = nullptr;
    napi_get_named_property(env, vfiValue, "onRead", &onRead);
    napi_get_named_property(env, vfiValue, "onStat", &onStat);
    napi_get_named_property(env, vfiValue, "onCancel", &onCancel);

    napi_valuetype onReadType = napi_undefined;
    napi_typeof(env, onRead, &onReadType);
    if (onReadType != napi_function) {
        napi_throw_error(env, "401", "Parameter error: onRead is required");
        return false;
    }
    napi_valuetype itemInfoType = napi_undefined;
    napi_typeof(env, itemInfo, &itemInfoType);
    if (itemInfoType != napi_object) {
        napi_throw_error(env, "401", "Parameter error: itemInfo is required");
        return false;
    }

    std::shared_ptr<PasteDataEntry> entry;
    sptr<NapiFuseGetter> provider;
    if (!BuildVirtualFileEntry(env, itemInfo, onRead, onStat, onCancel, mimeType, entry, provider)) {
        return false;
    }
    record->AddEntry("general.virtual-file", entry);
    record->SetFuseGetter(provider);
    return true;
}

constexpr size_t ARGC_TYPE_SET1 = 1;
constexpr size_t ARGC_TYPE_SET2 = 2;
napi_value PasteboardNapi::CreateHtmlRecord(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateHtmlRecord is called!");
    std::string value;
    bool ret = GetValue(env, in, value);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to GetValue!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PasteDataRecordNapi::NewHtmlTextRecordInstance(env, value, instance);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreatePlainTextRecord(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreatePlainTextRecord is called!");
    std::string value;
    bool ret = GetValue(env, in, value);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to GetValue!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PasteDataRecordNapi::NewPlainTextRecordInstance(env, value, instance);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreateUriRecord(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateUriRecord is called!");
    std::string value;
    bool ret = GetValue(env, in, value);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to GetValue!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PasteDataRecordNapi::NewUriRecordInstance(env, value, instance);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreatePixelMapRecord(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreatePixelMapRecord is called!");
    std::shared_ptr<PixelMap> pixelMap = PixelMapNapi::GetPixelMap(env, in);
    if (pixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to get pixelMap!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PasteDataRecordNapi::NewPixelMapRecordInstance(env, pixelMap, instance);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreateWantRecord(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateWantRecord is called!");
    AAFwk::Want want;
    bool ret = OHOS::AppExecFwk::UnwrapWant(env, in, want);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to unwrap want!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PasteDataRecordNapi::NewWantRecordInstance(env, std::make_shared<AAFwk::Want>(want), instance);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

// common function of CreateHtmlData, CreatePlainTextData, CreateUriData
PasteDataNapi *PasteboardNapi::CreateDataCommon(napi_env env, napi_value in, std::string &str, napi_value &instance)
{
    bool ret = GetValue(env, in, str);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "GetValue failed");
        return nullptr;
    }
    PASTEBOARD_CALL(PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_unwrap exec error: %{public}d", status);
        return nullptr;
    }
    if (obj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "obj is null!");
        return nullptr;
    }
    return obj;
}

napi_value PasteboardNapi::CreateHtmlData(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateHtmlData is called!");
    std::string str;
    napi_value instance = nullptr;
    PasteDataNapi *obj = CreateDataCommon(env, in, str, instance);
    if (obj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "obj is null!");
        return nullptr;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateHtmlData(str);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreatePlainTextData(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreatePlainTextData is called!");
    std::string str;
    napi_value instance = nullptr;
    PasteDataNapi *obj = CreateDataCommon(env, in, str, instance);
    if (obj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "obj is null!");
        return nullptr;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreatePlainTextData(str);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreateUriData(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateUriData is called!");
    std::string str;
    napi_value instance = nullptr;
    PasteDataNapi *obj = CreateDataCommon(env, in, str, instance);
    if (obj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "obj is null!");
        return nullptr;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateUriData(OHOS::Uri(str));
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreatePixelMapData(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreatePixelMapData is called!");
    std::shared_ptr<PixelMap> pixelMap = PixelMapNapi::GetPixelMap(env, in);
    if (pixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to get pixelMap!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PASTEBOARD_CALL(PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap Failed!");
        return nullptr;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMap);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreateWantData(napi_env env, napi_value in)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateWantData is called!");
    AAFwk::Want want;
    bool ret = OHOS::AppExecFwk::UnwrapWant(env, in, want);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to unwrap want!");
        return nullptr;
    }
    napi_value instance = nullptr;
    PASTEBOARD_CALL(PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed!");
        return nullptr;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateWantData(std::make_shared<AAFwk::Want>(want));
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreateMultiTypeData(napi_env env,
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> typeValueVector)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateMultiTypeData is called!");
    napi_value instance = nullptr;
    PASTEBOARD_CALL(PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed!");
        return nullptr;
    }
    if (typeValueVector == nullptr || typeValueVector->empty()) {
        return nullptr;
    }
    std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>> typeValueMap =
            std::make_shared<std::map<std::string, std::shared_ptr<EntryValue>>>();
    for (const auto &item : *typeValueVector) {
        typeValueMap->emplace(item.first, item.second);
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateMultiTypeData(std::move(typeValueMap),
        typeValueVector->begin()->first);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::CreateMultiTypeDelayData(napi_env env, std::vector<std::string> mimeTypes,
    std::shared_ptr<UDMF::EntryGetter> entryGetter)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "CreateMultiTypeDelayData is called!");
    napi_value instance = nullptr;
    PASTEBOARD_CALL(PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed!");
        return nullptr;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateMultiTypeDelayData(mimeTypes, entryGetter);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "end.");
    return instance;
}

napi_value PasteboardNapi::JScreateHtmlTextRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateHtmlTextRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    return CreateHtmlRecord(env, argv[0]);
}

napi_value PasteboardNapi::JScreateWantRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateWantRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "Wrong argument type. Object expected.");

    return CreateWantRecord(env, argv[0]);
}

napi_value PasteboardNapi::JScreateShareOption(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateShareOption is called!");

    napi_value jsShareOption = nullptr;
    napi_create_object(env, &jsShareOption);

    napi_value jsInApp = CreateNapiNumber(env, static_cast<int32_t>(ShareOption::InApp));
    NAPI_CALL(env, napi_set_named_property(env, jsShareOption, "InApp", jsInApp));
    NAPI_CALL(env, napi_set_named_property(env, jsShareOption, "INAPP", jsInApp));

    napi_value jsLocalDevice = CreateNapiNumber(env, static_cast<int32_t>(ShareOption::LocalDevice));
    NAPI_CALL(env, napi_set_named_property(env, jsShareOption, "LocalDevice", jsLocalDevice));
    NAPI_CALL(env, napi_set_named_property(env, jsShareOption, "LOCALDEVICE", jsLocalDevice));

    napi_value jsCrossDevice = CreateNapiNumber(env, static_cast<int32_t>(ShareOption::CrossDevice));
    NAPI_CALL(env, napi_set_named_property(env, jsShareOption, "CrossDevice", jsCrossDevice));
    NAPI_CALL(env, napi_set_named_property(env, jsShareOption, "CROSSDEVICE", jsCrossDevice));

    return jsShareOption;
}

napi_value PasteboardNapi::JScreatePattern(napi_env env, napi_callback_info info)
{
    napi_value jsPattern = nullptr;
    napi_create_object(env, &jsPattern);

    napi_value jsURL = CreateNapiNumber(env, static_cast<uint32_t>(Pattern::URL));
    NAPI_CALL(env, napi_set_named_property(env, jsPattern, "URL", jsURL));

    napi_value jsNumber = CreateNapiNumber(env, static_cast<uint32_t>(Pattern::NUMBER));
    NAPI_CALL(env, napi_set_named_property(env, jsPattern, "NUMBER", jsNumber));

    napi_value jsEmailAddress = CreateNapiNumber(env, static_cast<uint32_t>(Pattern::EMAIL_ADDRESS));
    NAPI_CALL(env, napi_set_named_property(env, jsPattern, "EMAIL_ADDRESS", jsEmailAddress));

    napi_value jsHttpLink = CreateNapiNumber(env, static_cast<uint32_t>(Pattern::HTTP_URL));
    NAPI_CALL(env, napi_set_named_property(env, jsPattern, "HTTP_URL", jsHttpLink));

    napi_value jsFlightNumber = CreateNapiNumber(env, static_cast<uint32_t>(Pattern::FLIGHT_NUMBER));
    NAPI_CALL(env, napi_set_named_property(env, jsPattern, "FLIGHT_NUMBER", jsFlightNumber));

    return jsPattern;
}

napi_value PasteboardNapi::JScreateFileConflictOptions(napi_env env, napi_callback_info info)
{
    napi_value jsFileConflictOptions = nullptr;
    napi_create_object(env, &jsFileConflictOptions);

    napi_value jsOVERWRITE = CreateNapiNumber(env, static_cast<uint32_t>(FileConflictOption::FILE_OVERWRITE));
    NAPI_CALL(env, napi_set_named_property(env, jsFileConflictOptions, "OVERWRITE", jsOVERWRITE));

    napi_value jsSKIP = CreateNapiNumber(env, static_cast<uint32_t>(FileConflictOption::FILE_SKIP));
    NAPI_CALL(env, napi_set_named_property(env, jsFileConflictOptions, "SKIP", jsSKIP));

    return jsFileConflictOptions;
}

napi_value PasteboardNapi::JScreateProgressIndicator(napi_env env, napi_callback_info info)
{
    napi_value jsProgressIndicator = nullptr;
    napi_create_object(env, &jsProgressIndicator);

    napi_value jsNONE = CreateNapiNumber(env, static_cast<uint32_t>(ProgressIndicator::NONE_PROGRESS_INDICATOR));
    NAPI_CALL(env, napi_set_named_property(env, jsProgressIndicator, "NONE", jsNONE));

    napi_value jsDEFAULT = CreateNapiNumber(env, static_cast<uint32_t>(ProgressIndicator::DEFAULT_PROGRESS_INDICATOR));
    NAPI_CALL(env, napi_set_named_property(env, jsProgressIndicator, "DEFAULT", jsDEFAULT));

    return jsProgressIndicator;
}

napi_value PasteboardNapi::JScreatePlainTextRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreatePlainTextRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    return CreatePlainTextRecord(env, argv[0]);
}

napi_value PasteboardNapi::JScreatePixelMapRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreatePixelMapRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "Wrong argument type. Object expected.");

    return CreatePixelMapRecord(env, argv[0]);
}

napi_value PasteboardNapi::JScreateUriRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateUriRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    return CreateUriRecord(env, argv[0]);
}

static bool CheckCreateRecordArgs(napi_env env, size_t argc, napi_valuetype type)
{
    if (!CheckExpression(env, argc >= ARGC_TYPE_SET2, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments cannot be less than two.") ||
        !CheckExpression(env, type == napi_object || type == napi_string, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. the first param should be object or string.")) {
        return false;
    }
    return true;
}

static std::string ReadStringValue(napi_env env, napi_value value)
{
    size_t len = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &len);
    std::string result;
    result.resize(len);
    napi_get_value_string_utf8(env, value, result.data(), len + 1, &len);
    return result;
}

napi_value PasteboardNapi::JSCreateRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JSCreateRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type));
    if (!CheckCreateRecordArgs(env, argc, type)) {
        return nullptr;
    }
    if (type == napi_string) {
        std::string mimeType = ReadStringValue(env, argv[0]);
        if (mimeType == MIMETYPE_TEXT_URI && IsVirtualFileInfo(env, argv[1])) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Creating VirtualFile record");
            return CreateVirtualFileRecord(env, mimeType, argv[1]);
        }
        if (!CheckArgs(env, argv, argc, mimeType)) {
            return nullptr;
        }
        auto it = createRecordMap_.find(mimeType);
        if (it != createRecordMap_.end()) {
            return (it->second)(env, argv[1]);
        }

        void *data = nullptr;
        size_t dataLen = 0;
        NAPI_CALL(env, napi_get_arraybuffer_info(env, argv[1], &data, &dataLen));
        std::vector<uint8_t> arrayBuf(reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + dataLen);
        napi_value instance = nullptr;
        PasteDataRecordNapi::NewKvRecordInstance(env, mimeType, arrayBuf, instance);
        return instance;
    } else {
        napi_ref provider = nullptr;
        std::vector<std::string> mimeTypes;
        if (!CheckArgsArray(env, argv[0], mimeTypes) ||
            !CheckArgsFunc(env, argv[1], provider)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CheckArgsArray or CheckArgsFunc failed!");
            return nullptr;
        }
        napi_value instance = nullptr;
        std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetter =
            std::make_shared<PastedataRecordEntryGetterInstance>(env, provider);
        entryGetter->GetStub()->SetEntryGetterWrapper(entryGetter);
        PasteDataRecordNapi::NewEntryGetterRecordInstance(mimeTypes, entryGetter, instance);
        return instance;
    }
}

napi_value PasteboardNapi::JScreateHtmlData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateHtmlData is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    return CreateHtmlData(env, argv[0]);
}

napi_value PasteboardNapi::JScreateWantData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateWantData is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "Wrong argument type. Object expected.");

    return CreateWantData(env, argv[0]);
}

napi_value PasteboardNapi::JScreatePlainTextData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreatePlainTextData is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    return CreatePlainTextData(env, argv[0]);
}

napi_value PasteboardNapi::JScreatePixelMapData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreatePixelMapData is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "Wrong argument type. Object expected.");

    return CreatePixelMapData(env, argv[0]);
}

napi_value PasteboardNapi::JScreateUriData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JScreateUriData is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");

    return CreateUriData(env, argv[0]);
}

napi_value PasteboardNapi::JSCreateKvData(
    napi_env env, const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JSCreateKvData is called!");

    napi_value instance = nullptr;
    PASTEBOARD_CALL(PasteDataNapi::NewInstance(env, instance));
    PasteDataNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed!");
        return nullptr;
    }

    obj->value_ = PasteboardClient::GetInstance()->CreateKvData(mimeType, arrayBuffer);
    return instance;
}

napi_value PasteboardNapi::CreateDelayDataFromArray(napi_env env, napi_value argv[], size_t argc)
{
    napi_ref provider = nullptr;
    std::vector<std::string> mimeTypes;
    if (!CheckArgsArray(env, argv[0], mimeTypes) || !CheckArgsFunc(env, argv[1], provider)) {
        return nullptr;
    }
    std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetter =
        std::make_shared<PastedataRecordEntryGetterInstance>(env, provider);
    entryGetter->GetStub()->SetEntryGetterWrapper(entryGetter);
    return CreateMultiTypeDelayData(env, mimeTypes, entryGetter->GetStub());
}

napi_value PasteboardNapi::JSCreateData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JSCreateData is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    std::string mimeType;
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> typeValueVector =
        std::make_shared<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>>();
    if (!CheckExpression(env, argc >= ARGC_TYPE_SET1, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments cannot be less than one.")) {
        return nullptr;
    }
    if (argc == ARGC_TYPE_SET1) {
        if (!CheckArgsVector(env, argv[0], typeValueVector)) {
            return nullptr;
        }
        return CreateMultiTypeData(env, typeValueVector);
    }
    bool isArray = false;
    NAPI_CALL(env, napi_is_array(env, argv[0], &isArray));
    if (isArray) {
        return CreateDelayDataFromArray(env, argv, argc);
    }
    size_t mimeTypeLen = 0;
    napi_get_value_string_utf8(env, argv[0], nullptr, 0, &mimeTypeLen);
    mimeType.resize(mimeTypeLen);
    napi_get_value_string_utf8(env, argv[0], mimeType.data(), mimeTypeLen + 1, &mimeTypeLen);
    if (mimeType == MIMETYPE_TEXT_URI && IsVirtualFileInfo(env, argv[1])) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Creating VirtualFile data");
        return CreateVirtualFileData(env, mimeType, argv[1]);
    }
    if (!CheckArgs(env, argv, argc, mimeType)) {
        return nullptr;
    }
    auto it = createDataMap_.find(mimeType);
    if (it != createDataMap_.end()) {
        return (it->second)(env, argv[1]);
    }
    void *data = nullptr;
    size_t dataLen = 0;
    NAPI_CALL(env, napi_get_arraybuffer_info(env, argv[1], &data, &dataLen));
    std::vector<uint8_t> arrayBuf(reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + dataLen);
    return JSCreateKvData(env, mimeType, arrayBuf);
}

napi_value PasteboardNapi::JSgetSystemPasteboard(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JSgetSystemPasteboard is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_value instance = nullptr;
    napi_status status = SystemPasteboardNapi::NewInstance(env, instance); // 0 arguments
    if (status != napi_ok) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JSgetSystemPasteboard create instance failed");
        return NapiGetNull(env);
    }

    return instance;
}

napi_value PasteboardNapi::PasteBoardInit(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_WRITABLE_FUNCTION("createHtmlData", JScreateHtmlData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createWantData", JScreateWantData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createPlainTextData", JScreatePlainTextData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createPixelMapData", JScreatePixelMapData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createUriData", JScreateUriData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createData", JSCreateData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createDelayData", JSCreateData),
        DECLARE_NAPI_WRITABLE_FUNCTION("createHtmlTextRecord", JScreateHtmlTextRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("createWantRecord", JScreateWantRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("createPlainTextRecord", JScreatePlainTextRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("createPixelMapRecord", JScreatePixelMapRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("createUriRecord", JScreateUriRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("createRecord", JSCreateRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("createDelayRecord", JSCreateRecord),
        DECLARE_NAPI_WRITABLE_FUNCTION("getSystemPasteboard", JSgetSystemPasteboard),
        DECLARE_NAPI_GETTER("ShareOption", JScreateShareOption),
        DECLARE_NAPI_GETTER("Pattern", JScreatePattern),
        DECLARE_NAPI_GETTER("FileConflictOptions", JScreateFileConflictOptions),
        DECLARE_NAPI_GETTER("ProgressIndicator", JScreateProgressIndicator),
        DECLARE_NAPI_PROPERTY("MAX_RECORD_NUM", CreateNapiNumber(env, PasteData::MAX_RECORD_NUM)),
        DECLARE_NAPI_PROPERTY("MIMETYPE_PIXELMAP", CreateNapiString(env, MIMETYPE_PIXELMAP)),
        DECLARE_NAPI_PROPERTY("MIMETYPE_TEXT_HTML", CreateNapiString(env, MIMETYPE_TEXT_HTML)),
        DECLARE_NAPI_PROPERTY("MIMETYPE_TEXT_WANT", CreateNapiString(env, MIMETYPE_TEXT_WANT)),
        DECLARE_NAPI_PROPERTY("MIMETYPE_TEXT_PLAIN", CreateNapiString(env, MIMETYPE_TEXT_PLAIN)),
        DECLARE_NAPI_PROPERTY("MIMETYPE_TEXT_URI", CreateNapiString(env, MIMETYPE_TEXT_URI)),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));

    return exports;
}

std::unordered_map<std::string, PasteboardNapi::FUNC> PasteboardNapi::createRecordMap_ = {
    { "text/html", &PasteboardNapi::CreateHtmlRecord }, { "text/plain", &PasteboardNapi::CreatePlainTextRecord },
    { "text/uri", &PasteboardNapi::CreateUriRecord }, { "pixelMap", &PasteboardNapi::CreatePixelMapRecord },
    { "text/want", &PasteboardNapi::CreateWantRecord }
};

std::unordered_map<std::string, PasteboardNapi::FUNC> PasteboardNapi::createDataMap_ = {
    { "text/html", &PasteboardNapi::CreateHtmlData }, { "text/plain", &PasteboardNapi::CreatePlainTextData },
    { "text/uri", &PasteboardNapi::CreateUriData }, { "pixelMap", &PasteboardNapi::CreatePixelMapData },
    { "text/want", &PasteboardNapi::CreateWantData }
};
} // namespace MiscServicesNapi
} // namespace OHOS