/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include "pasteboard_ffi.h"
#include "pasteboard_hilog.h"
#include "pasteboard_log.h"
#include "system_pasteboard_impl.h"

#ifndef CJ_EXPORT
#ifndef __WINDOWS__
#define CJ_EXPORT __attribute__((visibility("default")))
#else
#define CJ_EXPORT __declspec(dllexport)

#endif
#endif

using namespace OHOS::FFI;
using namespace OHOS::Media;
using namespace OHOS::MiscServices;
using namespace OHOS::MiscServicesCj;

namespace OHOS {
namespace CJSystemapi {
extern "C" {
RetDataI64 FfiOHOSCreateStringPasteData(const char *mimeType, const char *value)
{
    LOGI("[PasteData] FfiOHOSCreateStringPasteData");
    RetDataI64 ret;
    std::string mmType = mimeType;
    CJValueType valueType;
    valueType.stringValue = value;

    ret.data = CreateCjPasteDataObject(mmType, valueType);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSCreateStringPasteData success");

    return ret;
}

RetDataI64 FfiOHOSCreatePixelMapPasteData(const char *mimeType, int64_t pixelMapId)
{
    LOGI("[PasteData] FfiOHOSCreatePixelMapPasteData");
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    auto pixelMapImpl = FFIData::GetData<PixelMapImpl>(pixelMapId);
    if (pixelMapImpl == nullptr) {
        return ret;
    }
    std::string mmType = mimeType;
    CJValueType valueType;
    valueType.pixelMap = pixelMapImpl->GetRealPixelMap();

    ret.data = CreateCjPasteDataObject(mmType, valueType);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSCreatePixelMapPasteData success");

    return ret;
}

RetDataI64 FfiOHOSCreateArrayBufPasteData(const char *mimeType, uint8_t *buffPtr, int64_t bufferSize)
{
    LOGI("[PasteData] FfiOHOSCreateArrayBufPasteData");
    RetDataI64 ret;
    std::string mmType = mimeType;
    CJValueType valueType;
    valueType.arrayBufferData = buffPtr;
    valueType.arrayBufferSize = bufferSize;

    ret.data = CreateCjPasteDataObject(mmType, valueType);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSCreateArrayBufPasteData success");

    return ret;
}

char *PasteBoardMallocCString(const std::string &origin)
{
    auto len = origin.length() + 1;
    char *res = static_cast<char *>(malloc(sizeof(char) * len));
    if (res == nullptr) {
        return nullptr;
    }
    auto ret = strcpy_s(res, sizeof(char) * len, origin.c_str());
    if (ret != EOK) {
        LOGE("strcpy_s error");
        free(res);
        return nullptr;
    }
    return res;
}

void FillCPasteDataRecord(CPasteDataRecord *retPtr, std::shared_ptr<PasteDataRecord> record)
{
    if (retPtr == nullptr || record == nullptr) {
        return;
    }
    retPtr->htmlText = nullptr;
    retPtr->mimeType = nullptr;
    retPtr->plainText = nullptr;
    retPtr->uri = nullptr;
    retPtr->pixelMap = ERR_INVALID_INSTANCE_CODE;
    if (record->GetHtmlTextV0() != nullptr) {
        std::string resHtmlText = *(record->GetHtmlTextV0());
        retPtr->htmlText = PasteBoardMallocCString(resHtmlText);
    }
    if (!record->GetMimeType().empty()) {
        std::string resMimeType = record->GetMimeType();
        retPtr->mimeType = PasteBoardMallocCString(resMimeType);
    }
    if (record->GetPlainTextV0() != nullptr) {
        std::string resPlainText = *(record->GetPlainTextV0());
        retPtr->plainText = PasteBoardMallocCString(resPlainText);
    }
    if (record->GetUriV0() != nullptr) {
        std::string resUri = record->GetUriV0()->ToString();
        retPtr->uri = PasteBoardMallocCString(resUri);
    }
    std::shared_ptr<PixelMap> pixelMap = record->GetPixelMapV0();
    auto nativeImage = FFIData::Create<PixelMapImpl>(move(pixelMap));
    if (!nativeImage) {
        retPtr->pixelMap = 0;
        return;
    }
    retPtr->pixelMap = nativeImage->GetID();
}

RetDataI64 FfiOHOSCreateStringPasteDataRecord(const char *mimeType, const char *value, CPasteDataRecord *retPtr)
{
    LOGI("[PasteDataRecord] FfiOHOSCreateStringPasteDataRecord");
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    std::string mmType = mimeType;
    CJValueType valueType;
    valueType.stringValue = value;

    ret.data = CreateCjPasteDataRecordObject(mmType, valueType);
    if (ret.data == 0) {
        return ret;
    }
    auto recordInstance = FFIData::GetData<PasteDataRecordImpl>(ret.data);
    if (!recordInstance) {
        FFIData::Release(ret.data);
        return ret;
    }
    std::shared_ptr<PasteDataRecord> record = recordInstance->GetRealPasteDataRecord();
    FillCPasteDataRecord(retPtr, record);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteDataRecord] FfiOHOSCreateStringPasteDataRecord success");

    return ret;
}

RetDataI64 FfiOHOSCreatePixelMapPasteDataRecord(const char *mimeType, int64_t pixelMapId, CPasteDataRecord *retPtr)
{
    LOGI("[PasteDataRecord] FfiOHOSCreatePixelMapPasteDataRecord");
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    std::string mmType = mimeType;
    auto pixelMapImpl = FFIData::GetData<PixelMapImpl>(pixelMapId);
    if (pixelMapImpl == nullptr) {
        return ret;
    }

    auto pixelMap = pixelMapImpl->GetRealPixelMap();
    CJValueType valueType;
    valueType.pixelMap = pixelMap;
    ret.data = CreateCjPasteDataRecordObject(mmType, valueType);
    if (ret.data == 0) {
        return ret;
    }
    auto recordInstance = FFIData::GetData<PasteDataRecordImpl>(ret.data);
    if (!recordInstance) {
        FFIData::Release(ret.data);
        return ret;
    }
    std::shared_ptr<PasteDataRecord> record = recordInstance->GetRealPasteDataRecord();
    FillCPasteDataRecord(retPtr, record);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteDataRecord] FfiOHOSCreatePixelMapPasteDataRecord success");

    return ret;
}

RetDataI64 FfiOHOSCreateArrayBufPasteDataRecord(
    const char *mimeType, uint8_t *buffPtr, int64_t bufferSize, CPasteDataRecord *retPtr)
{
    LOGI("[PasteDataRecord] FfiOHOSCreateArrayBufPasteDataRecord");
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    std::string mmType = mimeType;
    CJValueType valueType;
    valueType.arrayBufferData = buffPtr;
    valueType.arrayBufferSize = bufferSize;

    ret.data = CreateCjPasteDataRecordObject(mmType, valueType);
    if (ret.data == 0) {
        return ret;
    }
    auto recordInstance = FFIData::GetData<PasteDataRecordImpl>(ret.data);
    if (!recordInstance) {
        FFIData::Release(ret.data);
        return ret;
    }
    std::shared_ptr<PasteDataRecord> record = recordInstance->GetRealPasteDataRecord();
    FillCPasteDataRecord(retPtr, record);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteDataRecord] FfiOHOSCreateArrayBufPasteDataRecord success");

    return ret;
}

RetDataCString FfiOHOSPasteDataGetPrimaryText(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryText start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetPrimaryText: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetPrimaryText: pasteData not exist");
        return ret;
    }

    std::shared_ptr<std::string> p = pasteData->GetPrimaryText();
    if (p != nullptr) {
        ret.data = PasteBoardMallocCString(*p);
        if (ret.data == nullptr) {
            ret.code = ERR_CODE_PARAM_INVALID;
            return ret;
        }
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryText success");

    return ret;
}

RetDataCString FfiOHOSPasteDataRecordToPlainText(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataRecordToPlainText start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<PasteDataRecordImpl>(id);
    if (!instance) {
        LOGE("[PasteRecord] ToPlainText: instance not exist %{public}" PRId64, id);
        ret.code = ERR_INVALID_INSTANCE_CODE;
        return ret;
    }

    auto realRecord = instance->GetRealPasteDataRecord();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(realRecord != nullptr, ret, PASTEBOARD_MODULE_SERVICE, "realRecord is null");
    std::string res = realRecord->ConvertToText();
    ret.data = PasteBoardMallocCString(res);
    if (ret.data == nullptr) {
        ret.code = ERR_CODE_PARAM_INVALID;
        return ret;
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteRecord] FfiOHOSPasteDataRecordToPlainText success");

    return ret;
}

RetDataCString FfiOHOSPasteDataGetPrimaryHtml(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryHtml start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetPrimaryHtml: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetPrimaryHtml: pasteData not exist");
        return ret;
    }

    std::shared_ptr<std::string> p = pasteData->GetPrimaryHtml();
    if (p != nullptr) {
        ret.data = PasteBoardMallocCString(*p);
        if (ret.data == nullptr) {
            ret.code = ERR_CODE_PARAM_INVALID;
            return ret;
        }
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryHtml success");

    return ret;
}

RetDataCString FfiOHOSPasteDataGetPrimaryUri(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryUri start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetPrimaryUri: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetPrimaryUri: pasteData not exist");
        return ret;
    }

    std::shared_ptr<OHOS::Uri> p = pasteData->GetPrimaryUri();
    if (p != nullptr) {
        std::string uri = p->ToString();
        ret.data = PasteBoardMallocCString(uri);
        if (ret.data == nullptr) {
            ret.code = ERR_CODE_PARAM_INVALID;
            return ret;
        }
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryUri success");

    return ret;
}

RetDataI64 FfiOHOSPasteDataGetPrimaryPixelMap(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryPixelMap start");
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetPrimaryPixelMap: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetPrimaryPixelMap: pasteData not exist");
        return ret;
    }

    std::shared_ptr<PixelMap> pixelMap = pasteData->GetPrimaryPixelMap();
    if (pixelMap != nullptr) {
        auto nativeImage = FFIData::Create<PixelMapImpl>(move(pixelMap));
        if (!nativeImage) {
            return ret;
        }
        ret.data = nativeImage->GetID();
        ret.code = SUCCESS_CODE;
        LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryPixelMap success");
    } else {
        LOGE("[PasteData] pixelMap not exist");
    }

    return ret;
}

RetDataCString FfiOHOSPasteDataGetPrimaryMimeType(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryMimeType start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetPrimaryMimeType: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetPrimaryMimeType: pasteData not exist");
        return ret;
    }

    std::shared_ptr<std::string> mimeType = pasteData->GetPrimaryMimeType();
    if (mimeType != nullptr) {
        ret.data = PasteBoardMallocCString(*mimeType);
        if (ret.data == nullptr) {
            ret.code = ERR_CODE_PARAM_INVALID;
            return ret;
        }
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetPrimaryMimeType success");

    return ret;
}

static char **VectorToCArrString(std::vector<std::string> &vec)
{
    char **result = new char *[vec.size()];
    if (result == nullptr) {
        return nullptr;
    }
    size_t temp = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        result[i] = new char[vec[i].length() + 1];
        if (result[i] == nullptr) {
            break;
        }
        if (strcpy_s(result[i], vec[i].length() + 1, vec[i].c_str()) != 0) {
            delete[] result[i];
            result[i] = nullptr;
            break;
        }
        temp++;
    }
    if (temp != vec.size()) {
        for (size_t j = temp; j > 0; j--) {
            delete[] result[j - 1];
            result[j - 1] = nullptr;
        }
        delete[] result;
        return nullptr;
    }
    return result;
}

int32_t FfiOHOSPasteDataGetProperty(int64_t id, CPasteDataProperty *retPtr)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetProperty start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetProperty: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetProperty: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }
    PasteDataProperty property = pasteData->GetProperty();
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(retPtr != nullptr, ERR_CODE_PARAM_INVALID,
        PASTEBOARD_MODULE_SERVICE, "retPtr is null");
    retPtr->tag = PasteBoardMallocCString(property.tag);
    if (retPtr->tag == nullptr) {
        return ERR_CODE_PARAM_INVALID;
    }
    retPtr->mimeTypes.head = VectorToCArrString(property.mimeTypes);
    if (retPtr->mimeTypes.head == nullptr) {
        free(retPtr->tag);
        retPtr->tag = nullptr;
        return ERR_CODE_PARAM_INVALID;
    }
    retPtr->mimeTypes.size = static_cast<int64_t>(property.mimeTypes.size());
    retPtr->timestamp = property.timestamp;
    retPtr->localOnly = property.localOnly;
    retPtr->shareOption = property.shareOption;
    LOGI("[PasteData] FfiOHOSPasteDataGetProperty success");

    return SUCCESS_CODE;
}

static std::vector<std::string> CArrStringToVector(CArrString src)
{
    LOGI("CArrStringToVector start");
    std::vector<std::string> res;
    for (int64_t i = 0; i < src.size; i++) {
        res.push_back(std::string(src.head[i]));
    }
    LOGI("CArrStringToVector end");
    return res;
}

int32_t FfiOHOSPasteDataSetProperty(
    int64_t id, CArrString mimeTypes, const char *tag, int64_t timestamp, bool localOnly, int32_t shareOption)
{
    LOGI("[PasteData] FfiOHOSPasteDataSetProperty start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] SetProperty: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }
    PasteDataProperty property;
    property.mimeTypes = CArrStringToVector(mimeTypes);
    property.tag = tag;
    property.timestamp = timestamp;
    property.localOnly = localOnly;
    property.shareOption = static_cast<ShareOption>(shareOption);

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] SetProperty: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    pasteData->SetProperty(property);
    LOGI("[PasteData] FfiOHOSPasteDataSetProperty success");

    return SUCCESS_CODE;
}

RetDataCString FfiOHOSPasteDataGetTag(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetTag start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetTag: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetTag: pasteData not exist");
        return ret;
    }

    std::string tag = pasteData->GetTag();
    if (!tag.empty()) {
        ret.data = PasteBoardMallocCString(tag);
        if (ret.data == nullptr) {
            ret.code = ERR_CODE_PARAM_INVALID;
            return ret;
        }
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetTag success");

    return ret;
}

RetDataBool FfiOHOSPasteDataHasType(int64_t id, const char *mimeTypes)
{
    LOGI("[PasteData] FfiOHOSPasteDataHasType start");
    RetDataBool ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = false };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] HasType: instance not exist %{public}" PRId64, id);
        return ret;
    }

    std::string types = mimeTypes;

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] HasType: pasteData not exist");
        return ret;
    }

    ret.data = pasteData->HasMimeType(types);
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataHasType success");

    return ret;
}

int32_t FfiOHOSPasteDataAddRecord(int64_t id, int64_t recordId)
{
    LOGI("[PasteData] FfiOHOSPasteDataAddRecord start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] AddRecord: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }

    PasteDataRecord rec;
    auto recordInstance = FFIData::GetData<PasteDataRecordImpl>(recordId);
    if (!recordInstance) {
        LOGE("[PasteData] AddRecord: instance not exist %{public}" PRId64, recordId);
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] AddRecord: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    pasteData->AddRecord(recordInstance->GetRealPasteDataRecord());
    LOGI("[PasteData] FfiOHOSPasteDataAddRecord success");

    return SUCCESS_CODE;
}

int32_t FfiOHOSPasteDataAddMimeTypeRecord(int64_t id, const char *mimeType, const char *value)
{
    LOGI("[PasteData] FfiOHOSPasteDataAddMimeTypeRecord start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] AddMimeTypeRecord: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }

    std::string types = mimeType;
    std::string realValue = value;

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] AddMimeTypeRecord: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    if (types == MIMETYPE_TEXT_HTML) {
        pasteData->AddHtmlRecord(realValue);
    } else if (types == MIMETYPE_TEXT_PLAIN) {
        pasteData->AddTextRecord(realValue);
    } else {
        pasteData->AddUriRecord(OHOS::Uri(realValue));
    }
    LOGI("[PasteData] FfiOHOSPasteDataAddMimeTypeRecord success");

    return SUCCESS_CODE;
}

int32_t FfiOHOSPasteDataAddPixelMapRecord(int64_t id, const char *mimeType, int64_t pixelMapId)
{
    LOGI("[PasteData] FfiOHOSPasteDataAddPixelMapRecord start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] AddPixelMapRecord: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }
    auto pixelMapImpl = FFIData::GetData<PixelMapImpl>(pixelMapId);
    if (pixelMapImpl == nullptr) {
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto pixelMap = pixelMapImpl->GetRealPixelMap();
    if (pixelMap == nullptr) {
        LOGE("[PasteData] AddPixelMapRecord: PixelMap not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] AddPixelMapRecord: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    pasteData->AddPixelMapRecord(pixelMap);
    LOGI("[PasteData] FfiOHOSPasteDataAddPixelMapRecord success");

    return SUCCESS_CODE;
}

int32_t FfiOHOSPasteDataAddArrayRecord(int64_t id, const char *mimeType, uint8_t *buffPtr, int64_t bufferSize)
{
    LOGI("[PasteData] FfiOHOSPasteDataAddArrayRecord start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] AddArrayRecord: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }

    std::string types = mimeType;
    void *data = buffPtr;
    size_t dataLen = static_cast<size_t>(bufferSize);

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] AddArrayRecord: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    pasteData->AddKvRecord(
        types, std::vector<uint8_t>(reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + dataLen));
    LOGI("[PasteData] FfiOHOSPasteDataAddArrayRecord success");

    return SUCCESS_CODE;
}

RetDataCArrString FfiOHOSPasteDataGetMimeTypes(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataAddArrayRecord start");
    RetDataCArrString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = { .head = nullptr, .size = 0 } };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetMimeTypes: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetMimeTypes: pasteData not exist");
        return ret;
    }

    std::vector<std::string> mimeTypes = pasteData->GetMimeTypes();
    ret.data.head = VectorToCArrString(mimeTypes);
    if (ret.data.head == nullptr) {
        ret.code = ERR_CODE_PARAM_INVALID;
        return ret;
    }
    ret.data.size = static_cast<int64_t>(mimeTypes.size());
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetMimeTypes success");

    return ret;
}

RetDataI64 FfiOHOSPasteDataGetRecord(int64_t id, int32_t index, CPasteDataRecord *retPtr)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetRecord start");
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetRecord: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetRecord: pasteData not exist");
        return ret;
    }

    if ((std::size_t)index >= pasteData->GetRecordCount()) {
        LOGI("[PasteData] FfiOHOSPasteDataRemoveRecord index out of range.");
        ret.code = OUT_OF_RANGE;
        return ret;
    }
    std::shared_ptr<PasteDataRecord> record = pasteData->GetRecordAt((std::size_t)index);
    if (record == nullptr) {
        LOGE("[PasteData] FfiOHOSPasteDataRemoveRecord index out of range.");
        ret.code = OUT_OF_RANGE;
        return ret;
    }

    FillCPasteDataRecord(retPtr, record);
    auto existedRecordImpl = getCjPasteDataRecordImpl(record);
    if (existedRecordImpl != nullptr) {
        ret.data = existedRecordImpl->GetID();
    } else {
        auto pasteDataRecordImpl = FFI::FFIData::Create<PasteDataRecordImpl>(record);
        if (!pasteDataRecordImpl) {
            return ret;
        }
        ret.data = pasteDataRecordImpl->GetID();
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetRecord success");

    return ret;
}

RetDataUI FfiOHOSPasteDataGetRecordCount(int64_t id)
{
    LOGI("[PasteData] FfiOHOSPasteDataGetRecordCount start");
    RetDataUI ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] GetRecordCount: instance not exist %{public}" PRId64, id);
        return ret;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] GetRecordCount: pasteData not exist");
        return ret;
    }

    ret.data = pasteData->GetRecordCount();
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSPasteDataGetRecordCount success");

    return ret;
}

int32_t FfiOHOSPasteDataRemoveRecord(int64_t id, int32_t index)
{
    LOGI("[PasteData] FfiOHOSPasteDataRemoveRecord start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] RemoveRecord: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] RemoveRecord: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    if ((std::size_t)index >= pasteData->GetRecordCount()) {
        LOGI("[PasteData] FfiOHOSPasteDataRemoveRecord index out of range.");
        return OUT_OF_RANGE;
    }
    std::shared_ptr<PasteDataRecord> recordImpl = pasteData->GetRecordAt((std::size_t)index);
    if (recordImpl != nullptr) {
        auto existedRecordImpl = getCjPasteDataRecordImpl(recordImpl);
        if (existedRecordImpl != nullptr) {
            FFIData::Release(existedRecordImpl->GetID());
            removeCjPasteDataRecordImpl(recordImpl);
        }
        pasteData->RemoveRecordAt(index);
    }

    LOGI("[PasteData] FfiOHOSPasteDataRemoveRecord success");

    return SUCCESS_CODE;
}

int32_t FfiOHOSPasteDataReplaceRecord(int64_t id, int64_t recordId, int32_t index)
{
    LOGI("[PasteData] FfiOHOSPasteDataReplaceRecord start");
    auto instance = FFIData::GetData<PasteDataImpl>(id);
    if (!instance) {
        LOGE("[PasteData] ReplaceRecord: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto recordInstance = FFIData::GetData<PasteDataRecordImpl>(recordId);
    if (!recordInstance) {
        LOGE("[PasteData] ReplaceRecord: instance not exist %{public}" PRId64, recordId);
        return ERR_INVALID_INSTANCE_CODE;
    }

    auto pasteData = instance->GetRealPasteData();
    if (pasteData == nullptr) {
        LOGE("[PasteData] ReplaceRecord: pasteData not exist");
        return ERR_INVALID_INSTANCE_CODE;
    }

    if ((std::size_t)index >= pasteData->GetRecordCount()) {
        LOGI("[PasteData] FfiOHOSPasteDataReplaceRecord index out of range.");
        return OUT_OF_RANGE;
    }
    std::shared_ptr<PasteDataRecord> oldRecord = pasteData->GetRecordAt((std::size_t)index);
    if (oldRecord != nullptr) {
        auto existedRecordImpl = getCjPasteDataRecordImpl(oldRecord);
        if (existedRecordImpl != nullptr) {
            FFIData::Release(existedRecordImpl->GetID());
            removeCjPasteDataRecordImpl(oldRecord);
        }
    }

    std::shared_ptr<PasteDataRecord> newRecord = recordInstance->GetRealPasteDataRecord();
    addCjPasteDataRecordImpl(newRecord, recordInstance);
    pasteData->ReplaceRecordAt(index, newRecord);
    LOGI("[PasteData] FfiOHOSPasteDataReplaceRecord success");

    return SUCCESS_CODE;
}

RetDataI64 FfiOHOSGetSystemPasteboard()
{
    RetDataI64 ret;
    LOGI("[SystemPasteboard] FfiOHOSGetSystemPasteboard start");
    ret.code = SystemPasteboardImpl::GetSystemPasteboardImpl(ret.data);
    LOGI("[SystemPasteboard] FfiOHOSGetSystemPasteboard success");
    return ret;
}

int32_t FfiOHOSSystemPasteboardSetData(int64_t id, int64_t pasteDataId)
{
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardSetData start");
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] SetData: instance not exist %{public}" PRId64, id);
        return ERR_INVALID_INSTANCE_CODE;
    }
    auto pasteDataInstance = FFIData::GetData<PasteDataImpl>(pasteDataId);
    if (!pasteDataInstance) {
        LOGE("[SystemPasteboard] SetData: instance not exist %{public}" PRId64, pasteDataId);
        return ERR_INVALID_INSTANCE_CODE;
    }
    auto ret = instance->SetData(pasteDataInstance, pasteDataInstance->GetRealPasteData());
    if (ret == SUCCESS_CODE) {
        LOGI("[PasteData] FfiOHOSSystemPasteboardSetData success");
    }

    return SUCCESS_CODE;
}

RetDataI64 FfiOHOSSystemPasteboardGetData(int64_t id)
{
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardGetData start");
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] GetData: instance not exist %{public}" PRId64, id);
        return ret;
    }
    std::shared_ptr<MiscServices::PasteData> pasteData = std::make_shared<PasteData>();
    int32_t res = instance->GetData(*pasteData);
    if (res != static_cast<int32_t>(PasteboardError::E_OK)) {
        ret.code = res;
        return ret;
    }
    auto pasteDataImpl = FFIData::Create<PasteDataImpl>(pasteData);
    if (!pasteDataImpl) {
        return ret;
    }
    ret.data = pasteDataImpl->GetID();
    ret.code = SUCCESS_CODE;
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardGetData success");
    return ret;
}

RetDataBool FfiOHOSSystemPasteboardHasData(int64_t id)
{
    RetDataBool ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = false };
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardHasData start");
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] HasData: instance not exist %{public}" PRId64, id);
        return ret;
    }

    ret.data = instance->HasData();
    ret.code = SUCCESS_CODE;
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardHasData success");
    return ret;
}

FFI_EXPORT void FfiOHOSSystemPasteboardClearData(int64_t id)
{
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardClearData start");
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] ClearData: instance not exist %{public}" PRId64, id);
        return;
    }
    instance->ClearData();

    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardClearData success");
}

RetDataBool FfiOHOSSystemPasteboardIsRemoteData(int64_t id)
{
    RetDataBool ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = false };
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardIsRemoteData start");
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] IsRemoteData: instance not exist %{public}" PRId64, id);
        return ret;
    }

    ret.data = instance->IsRemoteData();
    ret.code = SUCCESS_CODE;
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardIsRemoteData success");
    return ret;
}

RetDataBool FfiOHOSSystemPasteboardHasDataType(int64_t id, const char *mimeType)
{
    RetDataBool ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = false };
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardHasDataType start");
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] HasDataType: instance not exist %{public}" PRId64, id);
        return ret;
    }

    std::string types = mimeType;
    ret.data = instance->HasDataType(types);
    ret.code = SUCCESS_CODE;
    LOGI("[SystemPasteboard] FfiOHOSSystemPasteboardHasDataType success");
    return ret;
}

RetDataCString FfiOHOSSystemPasteboardGetDataSource(int64_t id)
{
    LOGI("[PasteData] FfiOHOSSystemPasteboardGetDataSource start");
    RetDataCString ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = nullptr };
    auto instance = FFIData::GetData<SystemPasteboardImpl>(id);
    if (!instance) {
        LOGE("[SystemPasteboard] GetDataSource: instance not exist %{public}" PRId64, id);
        return ret;
    }
    std::string res = instance->GetDataSource();
    ret.data = PasteBoardMallocCString(res);
    if (ret.data == nullptr) {
        ret.code = ERR_CODE_PARAM_INVALID;
        return ret;
    }
    ret.code = SUCCESS_CODE;
    LOGI("[PasteData] FfiOHOSSystemPasteboardGetDataSource success");

    return ret;
}

static std::shared_ptr<EntryValue> CreateEntryValue(const CPasteDataEntry &entry)
{
    constexpr int64_t MAX_BUFFER_SIZE = 100 * 1024 * 1024; // 100MB

    switch (entry.valueType) {
        case STRING_VALUE_TYPE: {
            if (entry.value == nullptr) {
                LOGE("[PasteData] CreateEntryValue: string value is null");
                return nullptr;
            }
            char *strPtr = static_cast<char *>(entry.value);
            return std::make_shared<EntryValue>(std::string(strPtr));
        }
        case PIXELMAP_VALUE_TYPE: {
            if (entry.value == nullptr) {
                LOGE("[PasteData] CreateEntryValue: pixelMap value is null");
                return nullptr;
            }
            int64_t pixelMapId = *static_cast<int64_t *>(entry.value);
            auto pixelMapImpl = FFIData::GetData<PixelMapImpl>(pixelMapId);
            if (pixelMapImpl == nullptr) {
                LOGE("[PasteData] CreateEntryValue: FFIData::GetData failed, pixelMapId=%{public}" PRId64, pixelMapId);
                return nullptr;
            }
            return std::make_shared<EntryValue>(pixelMapImpl->GetRealPixelMap());
        }
        case ARRAYBUF_VALUE_TYPE: {
            if (entry.value == nullptr || entry.size < 0 || entry.size > MAX_BUFFER_SIZE) {
                LOGE("[PasteData] CreateEntryValue: invalid arrayBuf, size=%{public}" PRId64, entry.size);
                return nullptr;
            }
            uint8_t *bufPtr = static_cast<uint8_t *>(entry.value);
            std::vector<uint8_t> arrayBuf(bufPtr, bufPtr + entry.size);
            return std::make_shared<EntryValue>(std::move(arrayBuf));
        }
        default:
            LOGE("[PasteData] CreateEntryValue: unknown valueType=%{public}d", entry.valueType);
            return nullptr;
    }
}

static bool ValidateEntriesAndMimeTypes(const CArrPasteDataEntry *entries)
{
    if (entries == nullptr) {
        LOGE("[PasteData] ValidateEntriesAndMimeTypes: entries is null");
        return false;
    }

    constexpr int64_t MAX_ENTRY_NUM = 512;
    if (entries->size <= 0 || entries->head == nullptr || entries->size > MAX_ENTRY_NUM) {
        LOGE("[PasteData] ValidateEntriesAndMimeTypes: invalid entries, size=%{public}" PRId64, entries->size);
        return false;
    }

    constexpr int32_t MIMETYPE_MAX_SIZE = 1024;
    for (int64_t i = 0; i < entries->size; i++) {
        CPasteDataEntry entry = entries->head[i];
        if (entry.mimeType == nullptr) {
            LOGE("[PasteData] ValidateEntriesAndMimeTypes: mimeType is null, index=%{public}" PRId64, i);
            return false;
        }
        std::string mimeTypeStr(entry.mimeType);
        if (mimeTypeStr.empty() || mimeTypeStr.size() > MIMETYPE_MAX_SIZE) {
            LOGE("[PasteData] ValidateEntriesAndMimeTypes: invalid mimeType, index=%{public}" PRId64
                ", mimeType=%{public}s",
                i, entry.mimeType);
            return false;
        }
    }
    return true;
}

static bool BuildTypeValueMap(const CArrPasteDataEntry &entries,
    std::map<std::string, std::shared_ptr<EntryValue>> &typeValueMap,
    std::string &primaryMimeType)
{
    for (int64_t i = 0; i < entries.size; i++) {
        CPasteDataEntry entry = entries.head[i];
        std::string mimeType(entry.mimeType);

        if (primaryMimeType.empty()) {
            primaryMimeType = mimeType;
        }

        auto entryValue = CreateEntryValue(entry);
        if (entryValue == nullptr) {
            LOGE("[PasteData] BuildTypeValueMap: CreateEntryValue failed, index=%{public}" PRId64
                ", valueType=%{public}d",
                i, entry.valueType);
            return false;
        }

        typeValueMap.emplace(mimeType, entryValue);
    }
    return true;
}

CJ_EXPORT RetDataI64 FfiOHOSCreateMultiTypePasteData(const CArrPasteDataEntry *entries)
{
    RetDataI64 ret = { .code = ERR_INVALID_INSTANCE_CODE, .data = 0 };

    if (!ValidateEntriesAndMimeTypes(entries)) {
        ret.code = PASTEBOARD_INVALID_PARAMETERS;
        return ret;
    }

    LOGD("[PasteData] FfiOHOSCreateMultiTypePasteData enter, entries.size=%{public}" PRId64, entries->size);

    std::map<std::string, std::shared_ptr<EntryValue>> typeValueMap;
    std::string primaryMimeType;

    if (!BuildTypeValueMap(*entries, typeValueMap, primaryMimeType)) {
        ret.code = ERR_INVALID_INSTANCE_CODE;
        return ret;
    }

    int32_t errCode = CreateCjMultiTypePasteDataObject(typeValueMap, primaryMimeType, ret.data);
    if (errCode != SUCCESS_CODE) {
        LOGE("[PasteData] FfiOHOSCreateMultiTypePasteData failed: errCode=%{public}d", errCode);
        ret.code = errCode;
        return ret;
    }
    ret.code = SUCCESS_CODE;
    LOGD("[PasteData] FfiOHOSCreateMultiTypePasteData success, id=%{public}" PRId64, ret.data);
    return ret;
}
}
} // namespace CJSystemapi
} // namespace OHOS
