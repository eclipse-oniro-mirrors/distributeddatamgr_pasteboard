/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "system_pasteboard_impl.h"
#include "pasteboard_log.h"
#include "pasteboard_hilog.h"

using namespace OHOS::MiscServices;

namespace OHOS {
namespace MiscServicesCj {

static sptr<SystemPasteboardImpl> g_systemPasteboard_instance = nullptr;

SystemPasteboardImpl::SystemPasteboardImpl()
{
    value_ = nullptr;
    pasteData_ = nullptr;
}

int NewInstance(sptr<SystemPasteboardImpl> &instance)
{
    if (g_systemPasteboard_instance != nullptr) {
        instance = g_systemPasteboard_instance;
        return 0;
    }
    g_systemPasteboard_instance = FFI::FFIData::Create<SystemPasteboardImpl>();
    if (g_systemPasteboard_instance == nullptr) {
        LOGE("[SystemPasteboardImpl] NewInstance create instance failed");
        return ERR_INVALID_INSTANCE_CODE;
    }
    instance = g_systemPasteboard_instance;
    return 0;
}

int32_t SystemPasteboardImpl::GetSystemPasteboardImpl(int64_t &id)
{
    sptr<SystemPasteboardImpl> instance = nullptr;
    int32_t status = NewInstance(instance);
    if (status != 0) {
        LOGE("[SystemPasteboardImpl] GetSystemPasteboard create instance failed");
        return status;
    }
    id = instance->GetID();
    return 0;
}

int32_t SystemPasteboardImpl::SetData(sptr<PasteDataImpl> dataImpl, std::shared_ptr<MiscServices::PasteData> data)
{
    if (data == nullptr) {
        LOGE("[SystemPasteboardImpl] SetData data nullptr");
        return PASTEBOARD_INVALID_PARAMETERS;
    }
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    int32_t res = ERR_OK;
    if (ret == static_cast<int>(PasteboardError::E_OK)) {
        value_ = dataImpl;
        pasteData_ = data;
        LOGI("[SystemPasteboardImpl] SetData OK");
    } else if (ret == static_cast<int>(PasteboardError::PROHIBIT_COPY)) {
        res = PASTEBOARD_COPY_FORBIDDEN;
        LOGE("[SystemPasteboardImpl] SetData ERR PROHIBIT_COPY");
    } else if (ret == static_cast<int>(PasteboardError::TASK_PROCESSING)) {
        LOGE("[SystemPasteboardImpl] SetData ERR TASK_PROCESSING");
        res = PASTEBOARD_TASK_PROCESSING;
    }

    return res;
}

sptr<PasteDataImpl> SystemPasteboardImpl::GetSystemPasteboardPasteDataImpl()
{
    if (value_ == nullptr) {
        return nullptr;
    }

    return value_;
}

int32_t SystemPasteboardImpl::GetData(MiscServices::PasteData &pasteData)
{
    int32_t ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    return ret;
}

bool SystemPasteboardImpl::HasData()
{
    bool res = PasteboardClient::GetInstance()->HasPasteData();
    return res;
}

void SystemPasteboardImpl::ClearData()
{
    PasteboardClient::GetInstance()->Clear();
    value_ = nullptr;
    pasteData_ = nullptr;
}

bool SystemPasteboardImpl::IsRemoteData()
{
    bool res = PasteboardClient::GetInstance()->IsRemoteData();
    return res;
}

bool SystemPasteboardImpl::HasDataType(std::string mimeType)
{
    bool res = PasteboardClient::GetInstance()->HasDataType(mimeType);
    return res;
}

std::string SystemPasteboardImpl::GetDataSource()
{
    std::string res;
    PasteboardClient::GetInstance()->GetDataSource(res);
    return res;
}

} // namespace MiscServicesCj
}