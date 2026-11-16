/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "Pasteboard_Capi_VirtualFile"

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "oh_pasteboard_common.h"
#include "pasteboard_client.h"
#include "pasteboard_fuse_getter_client.h"
#include "paste_data.h"
#include "paste_data_record.h"
#include "pasteboard_hilog.h"
#include "common/histogram_wrapper.h"
#include "udmf_capi_common.h"
#include "convert_utils.h"
#include "uds.h"

using namespace OHOS::MiscServices;
using OHOS::sptr;

namespace {
constexpr int NDK_CALLBACK_TIMEOUT_S = 5;

static bool IsPasteboardValid(OH_Pasteboard *pasteboard)
{
    return pasteboard != nullptr && pasteboard->cid == PASTEBOARD_STRUCT_ID
        && !pasteboard->destroyed.load(std::memory_order_acquire);
}

static PASTEBOARD_ErrCode GetMappedCode(int32_t code)
{
    auto iter = errCodeMap.find(static_cast<PasteboardError>(code));
    if (iter != errCodeMap.end()) {
        return iter->second;
    }
    return ERR_INNER_ERROR;
}

class CapiFuseGetter final : public PasteboardFuseGetterClient {
public:
    CapiFuseGetter(const OH_UDMF_VirtualFileCallbacks *callbacks, void *context,
        OH_UDMF_VirtualFileFinalize finalize)
        : PasteboardFuseGetterClient({}), context_(context), finalize_(finalize)
    {
        if (callbacks != nullptr) {
            onRead_ = callbacks->onRead;
            onStat_ = callbacks->onStat;
            onCancel_ = callbacks->onCancel;
        }
    }

    ~CapiFuseGetter() override
    {
        if (finalize_ != nullptr) {
            finalize_(context_);
        }
    }

    CapiFuseGetter(const CapiFuseGetter &) = delete;
    CapiFuseGetter &operator=(const CapiFuseGetter &) = delete;

    int32_t OnRead(uint32_t recordId, const std::string &id, int64_t offset, int32_t length,
        std::vector<uint8_t> &data) override
    {
        (void)recordId;
        if (onRead_ == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "OnRead callback is null");
            return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
        }
        struct ReadResult {
            int ret = -1;
            std::vector<uint8_t> buffer;
            uint32_t bytesRead = 0;
            bool done = false;
            std::mutex mtx;
            std::condition_variable cv;
        };
        auto result = std::make_shared<ReadResult>();
        result->buffer.resize(length);
        auto cb = onRead_;
        auto ctx = context_;
        std::thread([cb, ctx, result, id, offset, length]() {
            result->ret = cb(ctx, id.c_str(), offset, static_cast<uint32_t>(length),
                result->buffer.data(), &result->bytesRead);
            {
                std::lock_guard<std::mutex> lock(result->mtx);
                result->done = true;
            }
            result->cv.notify_one();
        }).detach();
        {
            std::unique_lock<std::mutex> lock(result->mtx);
            result->cv.wait_for(lock, std::chrono::seconds(NDK_CALLBACK_TIMEOUT_S),
                [&result]() { return result->done; });
        }
        if (!result->done) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "OnRead callback timeout, id:%{public}s", id.c_str());
            return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
        }
        if (result->ret != 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "OnRead callback failed, ret=%{public}d", result->ret);
            return result->ret;
        }
        if (result->bytesRead > 0) {
            data.assign(result->buffer.begin(), result->buffer.begin() + result->bytesRead);
        }
        return 0;
    }

    int32_t OnStat(uint32_t recordId, const std::string &id, FuseFileStat &stat) override
    {
        (void)recordId;
        if (onStat_ == nullptr) {
            return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
        }
        struct StatResult {
            OH_UDMF_VirtualItem *item = nullptr;
            bool done = false;
            std::mutex mtx;
            std::condition_variable cv;
        };
        auto result = std::make_shared<StatResult>();
        auto cb = onStat_;
        auto ctx = context_;
        std::thread([cb, ctx, result, id]() {
            result->item = cb(ctx, id.c_str());
            {
                std::lock_guard<std::mutex> lock(result->mtx);
                result->done = true;
            }
            result->cv.notify_one();
        }).detach();
        {
            std::unique_lock<std::mutex> lock(result->mtx);
            result->cv.wait_for(lock, std::chrono::seconds(NDK_CALLBACK_TIMEOUT_S),
                [&result]() { return result->done; });
        }
        if (!result->done) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "OnStat callback timeout, id:%{public}s", id.c_str());
            return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
        }
        if (result->item == nullptr) {
            return static_cast<int32_t>(PasteboardError::INVALID_RETURN_VALUE_ERROR);
        }
        stat.id = id;
        stat.name = "";
        return 0;
    }

    int32_t OnCancel(uint32_t recordId, const std::vector<std::string> &idList) override
    {
        (void)recordId;
        if (onCancel_ == nullptr) {
            return 0;
        }
        std::vector<const char *> cIds;
        cIds.reserve(idList.size());
        for (const auto &item : idList) {
            cIds.push_back(item.c_str());
        }
        onCancel_(context_, cIds.data(), static_cast<uint32_t>(cIds.size()));
        return 0;
    }

private:
    void *context_ = nullptr;
    OH_UDMF_VirtualFileOnRead onRead_ = nullptr;
    OH_UDMF_VirtualFileOnStat onStat_ = nullptr;
    OH_UDMF_VirtualFileOnCancel onCancel_ = nullptr;
    OH_UDMF_VirtualFileFinalize finalize_ = nullptr;
};
} // namespace

int OH_Pasteboard_SetVirtualFileData(OH_Pasteboard *pasteboard, OH_UdmfData *data,
    const OH_UDMF_VirtualFileCallbacks *callbacks, void *context,
    OH_UDMF_VirtualFileFinalize finalize)
{
    HISTOGRAM_BOOLEAN_SAMPLED("Pasteboard.APICall.OH_Pasteboard_SetVirtualFileData", true);
    if (!IsPasteboardValid(pasteboard) || data == nullptr || callbacks == nullptr ||
        callbacks->onRead == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "invalid parameters");
        return ERR_INVALID_PARAMETER;
    }

    auto capiGetter = sptr<CapiFuseGetter>::MakeSptr(callbacks, context, finalize);
    if (capiGetter == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "create CapiFuseGetter failed");
        return ERR_INNER_ERROR;
    }

    std::map<uint32_t, sptr<IPasteboardFuseGetter>> fuseGetters;
    auto &unifiedData = *(data->unifiedData_);
    auto records = unifiedData.GetRecords();
    for (const auto &record : records) {
        if (record == nullptr) {
            continue;
        }
        fuseGetters[record->GetRecordId()] = capiGetter;
    }
    if (fuseGetters.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "no records in data");
        return ERR_INVALID_PARAMETER;
    }

    auto pasteDataPtr = ConvertUtils::Convert(unifiedData);
    if (pasteDataPtr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "Convert unifiedData failed");
        return ERR_INNER_ERROR;
    }
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteDataPtr, nullptr, {}, fuseGetters);
    if (ret != static_cast<int32_t>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_CAPI, "SetPasteData return invalid, result is %{public}d", ret);
        return GetMappedCode(ret);
    }
    return ERR_OK;
}
