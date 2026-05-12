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
#include "paste_data_impl.h"
#include "pasteboard_hilog.h"
#include "pixel_map_impl.h"
#include "system_pasteboard_impl.h"

using namespace OHOS::MiscServices;
using namespace OHOS::FFI;
using OHOS::Media::PixelMapImpl;

namespace OHOS {
namespace MiscServicesCj {

OHOS::FFI::RuntimeType *PasteDataImpl::GetClassType()
{
    static OHOS::FFI::RuntimeType runtimeType = OHOS::FFI::RuntimeType::Create<OHOS::FFI::FFIData>("PasteDataImpl");
    return &runtimeType;
}

int64_t CreateCjPasteDataObject(std::string mimeType, CJValueType value)
{
    auto pasteDataImpl = FFI::FFIData::Create<PasteDataImpl>(mimeType, value);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(pasteDataImpl != nullptr, ERR_INVALID_VALUE, PASTEBOARD_MODULE_SERVICE,
        "create cj paste data failed");
    return pasteDataImpl->GetID();
}

PasteDataImpl::PasteDataImpl()
{
    value_ = std::make_shared<PasteData>();
}

PasteDataImpl::PasteDataImpl(std::shared_ptr<MiscServices::PasteData> pasteData)
{
    value_ = pasteData;
}

PasteDataImpl::PasteDataImpl(std::string mimeType, const CJValueType &value)
{
    if (mimeType == "text/html") {
        CreateHtmlData(mimeType, value);
    } else if (mimeType == "text/plain") {
        CreatePlainTextData(mimeType, value);
    } else if (mimeType == "text/uri") {
        CreateUriData(mimeType, value);
    } else if (mimeType == "pixelMap") {
        CreatePixelMapData(mimeType, value);
    } else if (mimeType == "text/want") {
        CreateWantData(mimeType, value);
    } else {
        std::vector<uint8_t> arrayBuf(reinterpret_cast<uint8_t *>(value.arrayBufferData),
            reinterpret_cast<uint8_t *>(value.arrayBufferData) + value.arrayBufferSize);
        value_ = PasteboardClient::GetInstance()->CreateKvData(mimeType, arrayBuf);
    }
}

std::shared_ptr<MiscServices::PasteData> PasteDataImpl::GetRealPasteData()
{
    return value_;
}

void PasteDataImpl::CreateHtmlData(std::string mimeType, const CJValueType &value)
{
    value_ = PasteboardClient::GetInstance()->CreateHtmlData(value.stringValue);
}

void PasteDataImpl::CreatePlainTextData(std::string mimeType, const CJValueType &value)
{
    value_ = PasteboardClient::GetInstance()->CreatePlainTextData(value.stringValue);
}

void PasteDataImpl::CreateUriData(std::string mimeType, const CJValueType &value)
{
    value_ = PasteboardClient::GetInstance()->CreateUriData(OHOS::Uri(value.stringValue));
}

void PasteDataImpl::CreatePixelMapData(std::string mimeType, const CJValueType &value)
{
    value_ = PasteboardClient::GetInstance()->CreatePixelMapData(value.pixelMap);
}

void PasteDataImpl::CreateWantData(std::string mimeType, CJValueType value) {}

int32_t CreateCjMultiTypePasteDataObject(
    const std::map<std::string, std::shared_ptr<MiscServices::EntryValue>> &typeValueMap,
    const std::string &primaryMimeType,
    int64_t &outId)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "CreateCjMultiTypePasteDataObject enter, primaryMimeType=%{public}s",
        primaryMimeType.c_str());

    auto pasteData = PasteboardClient::GetInstance()->CreateMultiTypeData(
        std::make_shared<std::map<std::string, std::shared_ptr<EntryValue>>>(typeValueMap), primaryMimeType);
    if (pasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "CreateCjMultiTypePasteDataObject: PasteboardClient::CreateMultiTypeData returned null");
        return ERR_INVALID_INSTANCE_CODE;
    }
    auto pasteDataImpl = FFI::FFIData::Create<PasteDataImpl>(pasteData);
    if (pasteDataImpl == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_SERVICE,
            "CreateCjMultiTypePasteDataObject: FFIData::Create returned null");
        return ERR_INVALID_INSTANCE_CODE;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE,
        "CreateCjMultiTypePasteDataObject success, id=%{public}" PRId64 ", primaryMimeType=%{public}s",
        pasteDataImpl->GetID(), primaryMimeType.c_str());
    outId = pasteDataImpl->GetID();
    return SUCCESS_CODE;
}

} // namespace MiscServicesCj
}