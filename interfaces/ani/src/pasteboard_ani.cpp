/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <ani.h>
#include <array>
#include <iostream>
#include <map>
#include <unordered_map>
#include <memory>
#include <thread>
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_client.h"
#include "pasteboard_js_err.h"
#include "common/block_object.h"
#include "ani_common_want.h"
#include "image_ani_utils.h"
#include "pasteboard_ani_utils.h"
#include "unified_meta.h"
#include <ani_signature_builder.h>

using namespace OHOS::MiscServices;
using namespace arkts::ani_signature;

constexpr size_t SYNC_TIMEOUT = 3500;
constexpr size_t MIMETYPE_MAX_LEN = 1024;

static void ThrowBusinessError(ani_env *env, int errCode, std::string&& errMsg)
{
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "into ThrowBusinessError.");
    if (env == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "into ThrowBusinessError. env is null, return.");
        return;
    }
    static const char *errorClsName = "@ohos.base.BusinessError";
    ani_class cls {};
    if (env->FindClass(errorClsName, &cls) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "findClass BusinessError failed.");
        return;
    }
    ani_method ctor;
    if (env->Class_FindMethod(cls, "<ctor>", ":", &ctor) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "find method BusinessError.constructor failed.");
        return;
    }
    ani_object errorObject;
    if (env->Object_New(cls, ctor, &errorObject) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "create BusinessError object failed.");
        return;
    }
    ani_double aniErrCode = static_cast<ani_double>(errCode);
    ani_string errMsgStr;
    if (env->String_NewUTF8(errMsg.c_str(), errMsg.size(), &errMsgStr) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "convert errMsg to ani_string failed.");
        return;
    }
    if (env->Object_SetFieldByName_Double(errorObject, "code", aniErrCode) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "set error code failed.");
        return;
    }
    if (env->Object_SetPropertyByName_Ref(errorObject, "message", errMsgStr) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "set error message failed.");
        return;
    }
    ani_status flag = env->ThrowError(static_cast<ani_error>(errorObject));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "flag: %{public}d.", flag);
    return;
}

ani_object GetNullObject(ani_env *env)
{
    if (env == nullptr) {
        return nullptr;
    }
    ani_ref ref;
    if (env->GetNull(&ref) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "env->GetNull failed.");
        return NULL;
    }
    return static_cast<ani_object>(ref);
}

ani_object Create([[maybe_unused]] ani_env *env, std::shared_ptr<PasteData> &ptrPasteData)
{
    if (env == nullptr) {
        return nullptr;
    }
    static const char *className = "@ohos.pasteboard.PasteDataImpl";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "create failed.");
        return nullptr;
    }

    auto shareptrPasteData = new SharedPtrHolder<PasteData>(ptrPasteData);
    ani_object obj = NativeObjectWrapper<SharedPtrHolder<PasteData>>::Wrap(env, cls, shareptrPasteData);
    if (obj == nullptr) {
        delete shareptrPasteData;
        return nullptr;
    }
    return obj;
}

PasteData* UnwrapAndGetPasteDataPtr([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto shareptrPasteData = NativeObjectWrapper<SharedPtrHolder<PasteData>>::Unwrap(env, object);
    if (shareptrPasteData == nullptr) {
        return nullptr;
    }
    return shareptrPasteData->Get().get();
}

ani_object CreateObjectFromClass([[maybe_unused]] ani_env *env, const char* className)
{
    if (env == nullptr || className == nullptr) {
        return nullptr;
    }
    ani_object obj = nullptr;
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateObjectFromClass] Not found class.");
        return obj;
    }
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateObjectFromClass] get ctor failed.");
        return obj;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &obj)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateObjectFromClass] Create Object failed.");
        return obj;
    }

    return obj;
}

std::string GetStdStringFromUnion([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    if (env == nullptr) {
        return "";
    }
    UnionAccessor unionAccessor(env, union_obj);
    ani_string str;
    if (!unionAccessor.IsInstanceOf("std.core.String")) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetStdStringFromUnion] union_obj is not string!");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The type of mimeType must be string.");
        return "";
    }
    if (!unionAccessor.TryConvert<ani_string>(str)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[GetStdStringFromUnion] try to convert union object to ani_string failed!");
        return "";
    }

    return ANIUtils_ANIStringToStdString(env, str);
}

bool GetArrayBuffer([[maybe_unused]] ani_env *env, ani_object unionObj, std::vector<uint8_t> &vec)
{
    if (env == nullptr) {
        return false;
    }
    std::string classname("std.core.ArrayBuffer");
    bool isArrayBuffer = ANIUtils_UnionIsInstanceOf(env, unionObj, classname);
    if (!isArrayBuffer) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[getArrayBuffer] Failed: is not arraybuffer.");
        return false;
    }

    void* data;
    size_t length;
    if (ANI_OK != env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(unionObj), &data, &length)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[getArrayBuffer] Failed: env->ArrayBuffer_GetInfo().");
        return false;
    }

    auto pVal = static_cast<uint8_t*>(data);
    vec.assign(pVal, pVal + length);

    return true;
}

bool CheckMimeType(ani_env *env, std::string &mimeType)
{
    if (env == nullptr) {
        return false;
    }
    if (mimeType.size() == 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "The length of mimeType is 0.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The length of mimeType is 0.");
        return false;
    }

    if (mimeType.size() > MIMETYPE_MAX_LEN) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "The length of mimeType cannot be greater than 1024 bytes.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The length of mimeType cannot be greater than 1024 bytes.");
        return false;
    }

    return true;
}

static ani_enum_item GetEnumItem(ani_env *env, ani_int shareOption)
{
    if (env == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetEnumItem] env is null.");
        return nullptr;
    }

    ani_enum enumType;
    const char *enumName = "@ohos.pasteboard.pasteboard.ShareOption";
    if (ANI_OK != env->FindEnum(enumName, &enumType)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetEnumItem] Find Enum failed: %{public}s", enumName);
        return nullptr;
    }

    ani_enum_item enumItem;
    if (ANI_OK != env->Enum_GetEnumItemByIndex(enumType, shareOption, &enumItem)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetEnumItem] Enum_GetEnumItemByIndex failed.");
        return nullptr;
    }

    return enumItem;
}

static ani_double GetRecordCount([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    if (env == nullptr) {
        return 0;
    }
    PasteData* pPasteData = UnwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecordCount] pPasteData is null.");
        return 0;
    }

    return pPasteData->GetRecordCount();
}

static void AddRecordByPasteDataRecord([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_object record)
{
    if (env == nullptr) {
        return;
    }
    ani_ref uri;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(static_cast<ani_object>(record), "uri", &uri)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecord] Object_GetPropertyByName_Ref failed.");
        return;
    }

    auto uri_str = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(uri));

    PasteData* pPasteData = UnwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecord] pPasteData is null.");
        return;
    }

    PasteDataRecord::Builder builder("");
    builder.SetUri(std::make_shared<OHOS::Uri>(OHOS::Uri(uri_str)));
    std::shared_ptr<PasteDataRecord> result = builder.Build();
    if (result == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecord] result is null.");
        return;
    }
    pPasteData->AddRecord(*(result.get()));
}

static void ProcessStrValueOfRecord([[maybe_unused]] ani_env *env, ani_object union_obj, std::string mimeType,
    PasteData *pPasteData)
{
    if (env == nullptr) {
        return;
    }
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ProcessStrValueOfRecord] pPasteData is null.");
        return;
    }

    auto value_str = GetStdStringFromUnion(env, union_obj);
    if (mimeType == MIMETYPE_TEXT_HTML) {
        pPasteData->AddHtmlRecord(value_str);
    } else if (mimeType == MIMETYPE_TEXT_PLAIN) {
        pPasteData->AddTextRecord(value_str);
    } else {
        pPasteData->AddUriRecord(OHOS::Uri(value_str));
    }
}

static void AddRecordByTypeValue([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_string type, ani_object union_obj)
{
    if (env == nullptr) {
        return;
    }
    auto mimeType = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    if (!CheckMimeType(env, mimeType)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[AddRecordByTypeValue] minetype length is error. %{public}s", mimeType.c_str());
        return;
    }

    PasteData* pPasteData = UnwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] pPasteData is null.");
        return;
    }

    if (mimeType == MIMETYPE_PIXELMAP) {
        OHOS::Media::PixelMap* rawPixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnv(env, union_obj);
        if (rawPixelMap == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] GetPixelMapFromEnv failed.");
            return;
        }
        auto pixelMap = std::shared_ptr<OHOS::Media::PixelMap>(rawPixelMap);
        pPasteData->AddPixelMapRecord(pixelMap);
        return;
    } else if (mimeType == MIMETYPE_TEXT_WANT) {
        OHOS::AAFwk::Want want;
        if (!OHOS::AppExecFwk::UnwrapWant(env, union_obj, want)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] UnwrapWant failed.");
            return;
        }
        pPasteData->AddWantRecord(std::make_shared<OHOS::AAFwk::Want>(want));
        return;
    }

    if (mimeType == MIMETYPE_TEXT_HTML || mimeType == MIMETYPE_TEXT_PLAIN || mimeType == MIMETYPE_TEXT_URI) {
        ProcessStrValueOfRecord(env, union_obj, mimeType, pPasteData);
        return;
    }

    std::vector<uint8_t> vec;
    vec.clear();
    if (!GetArrayBuffer(env, union_obj, vec)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] getArrayBuffer failed.");
        return;
    }
    pPasteData->AddKvRecord(mimeType, vec);

    return;
}

static void SetNamedPropertyByStr(ani_env *env, ani_class cls, const char *propertyName, std::string propertyValue,
    ani_object &obj)
{
    if (env == nullptr || propertyName == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByStr] env or propertyName is null.");
        return;
    }
    ani_string propertyAniStr = {};
    env->String_NewUTF8(propertyValue.c_str(), propertyValue.length(), &propertyAniStr);

    ani_method propertySetter;
    if (ANI_OK != env->Class_FindMethod(cls, propertyName, nullptr, &propertySetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[SetNamedPropertyByStr] Class_FindMethod failed: %{public}s.", propertyName);
        return;
    }
    if (ANI_OK != env->Object_CallMethod_Void(obj, propertySetter, propertyAniStr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[SetNamedPropertyByStr] Object_CallMethod_Void failed: %{public}s.", propertyName);
        return;
    }
}

static void FillPasteDataRecordObject(ani_env *env, std::shared_ptr<PasteDataRecord> recordFromBottom, ani_object &obj)
{
    if (env == nullptr || recordFromBottom == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillPasteDataRecordObject] env or recordFromBottom is null.");
        return;
    }

    ani_class cls;
    if (ANI_OK != env->FindClass("@ohos.pasteboard.PasteDataRecordImpl", &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillPasteDataRecordObject] Not found class.");
        return;
    }

    auto mimeType = recordFromBottom->GetMimeType();
    SetNamedPropertyByStr(env, cls, Builder::BuildSetterName("mimeType").c_str(), mimeType, obj);

    auto plainTextPtr = recordFromBottom->GetPlainText();
    if (plainTextPtr != nullptr) {
        SetNamedPropertyByStr(env, cls, Builder::BuildSetterName("plainText").c_str(), *plainTextPtr.get(), obj);
    }

    auto uriPtr = recordFromBottom->GetUri();
    if (uriPtr != nullptr) {
        SetNamedPropertyByStr(env, cls, Builder::BuildSetterName("uri").c_str(), uriPtr->ToString(), obj);
    }
}

static ani_object GetRecord([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    [[maybe_unused]] ani_double index)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData* pPasteData = UnwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] pPasteData is null.");
        return GetNullObject(env);
    }

    if (index >= pPasteData->GetRecordCount() || index < 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] index is out of range.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::OUT_OF_RANGE),
            "index out of range.");
        return GetNullObject(env);
    }

    std::shared_ptr<PasteDataRecord> recordFromBottom = pPasteData->GetRecordAt(static_cast<std::size_t>(index));
    if (recordFromBottom == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] recordFromBottom is null.");
        return GetNullObject(env);
    }

    ani_object record = CreateObjectFromClass(env, "@ohos.pasteboard.PasteDataRecordImpl");
    if (record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] CreateObjectFromClass failed.");
        return GetNullObject(env);
    }
    FillPasteDataRecordObject(env, recordFromBottom, record);

    return record;
}

static void SetProperty([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object, ani_object property)
{
    if (env == nullptr) {
        return;
    }
    PasteData* pPasteData = UnwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetProperty] pPasteData is null.");
        return;
    }

    ani_ref shareOption;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(static_cast<ani_object>(property), "shareOption", &shareOption)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetProperty] Object_GetPropertyByName_Ref failed.");
        return;
    }
    ani_int shareOptionValue;
    if (ANI_OK != env->EnumItem_GetValue_Int(static_cast<ani_enum_item>(shareOption), &shareOptionValue)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetProperty] EnumItem_GetValue_Int failed.");
        return;
    }
    ani_int localOnlyValue = shareOptionValue == ShareOption::CrossDevice ? false : true;
    pPasteData->SetLocalOnly(localOnlyValue);
    pPasteData->SetShareOption(static_cast<ShareOption>(shareOptionValue));

    ani_ref tag;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(static_cast<ani_object>(property), "tag", &tag)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetProperty] Object_GetPropertyByName_Ref failed.");
        return;
    }
    auto tag_str = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(tag));
    pPasteData->SetTag(tag_str);

    return;
}

static void SetNamedPropertyByEnumInt(ani_env *env, ani_class cls, const char *propertyName, ani_int shareOpionValue,
    ani_object &obj)
{
    if (env == nullptr || propertyName == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByEnumInt] env or propertyName is null.");
        return;
    }
    ani_enum_item enumItem = GetEnumItem(env, shareOpionValue);
    if (enumItem == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByEnumInt] Class_FindMethod failed.");
        return;
    }
    ani_method shareOptionSetter;
    if (ANI_OK != env->Class_FindMethod(cls, propertyName, nullptr, &shareOptionSetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByEnumInt] Class_FindMethod failed.");
        return;
    }
    if (ANI_OK != env->Object_CallMethod_Void(obj, shareOptionSetter, enumItem)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByEnumInt] Object_CallMethod_Void failed.");
        return;
    }
}

static void FillPasteDataPropertyObject(ani_env *env, PasteDataProperty &property, ani_object &obj)
{
    if (env == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillPasteDataPropertyObject] env is null.");
        return;
    }

    ani_class cls;
    if (ANI_OK != env->FindClass("@ohos.pasteboard.PasteDataPropertyImpl", &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillPasteDataPropertyObject] Not found class.");
        return;
    }

    ani_int shareOpionValue = property.shareOption;
    SetNamedPropertyByEnumInt(env, cls, Builder::BuildSetterName("shareOption").c_str(), shareOpionValue, obj);
    SetNamedPropertyByStr(env, cls, Builder::BuildSetterName("tag").c_str(), property.tag, obj);

    ani_double timestampValue = property.timestamp;
    ani_method timestampSetter;
    if (ANI_OK != env->Class_FindMethod(cls, Builder::BuildSetterName("timestamp").c_str(),
        nullptr, &timestampSetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillPasteDataPropertyObject] Class_FindMethod failed.");
        return;
    }
    if (ANI_OK != env->Object_CallMethod_Void(obj, timestampSetter, timestampValue)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillPasteDataPropertyObject] Object_CallMethod_Void failed.");
        return;
    }
}

static ani_object GetProperty([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData* pPasteData = UnwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetProperty] pPasteData is null.");
        return GetNullObject(env);
    }

    PasteDataProperty property = pPasteData->GetProperty();

    ani_object propertyToAbove = CreateObjectFromClass(env, "@ohos.pasteboard.PasteDataPropertyImpl");
    if (propertyToAbove == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetProperty] CreateObjectFromClass failed.");
        return GetNullObject(env);
    }
    FillPasteDataPropertyObject(env, property, propertyToAbove);

    return propertyToAbove;
}

static ani_object CreateHtmlData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto value_str = GetStdStringFromUnion(env, union_obj);
    auto ptr = PasteboardClient::GetInstance()->CreateHtmlData(value_str);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateHtmlData] CreateHtmlData failed.");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreatePlainTextData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto value_str = GetStdStringFromUnion(env, union_obj);
    auto ptr = PasteboardClient::GetInstance()->CreatePlainTextData(value_str);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreatePlainTextData] CreatePlainTextData failed.");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreateUriData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto value_str = GetStdStringFromUnion(env, union_obj);
    auto ptr = PasteboardClient::GetInstance()->CreateUriData(OHOS::Uri(value_str));
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateUriData] CreateUriData failed.");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreatePixelMapData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    if (env == nullptr) {
        return nullptr;
    }
    OHOS::Media::PixelMap* rawPixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnv(env, union_obj);
    if (rawPixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreatePixelMapData] GetPixelMapFromEnv failed.");
        return GetNullObject(env);
    }

    auto pixelMap = std::shared_ptr<OHOS::Media::PixelMap>(rawPixelMap);
    auto ptr = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMap);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreatePixelMapData] CreatePixelMapData failed.");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreateWantData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    if (env == nullptr) {
        return nullptr;
    }
    OHOS::AAFwk::Want want;
    bool ret = OHOS::AppExecFwk::UnwrapWant(env, union_obj, want);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateWantData] UnwrapWant failed.");
        return GetNullObject(env);
    }

    auto ptr = PasteboardClient::GetInstance()->CreateWantData(std::make_shared<OHOS::AAFwk::Want>(want));
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateWantData] CreateWantData failed.");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

ani_object ProcessSpecialMimeType([[maybe_unused]] ani_env *env, ani_object union_obj, std::string type)
{
    if (env == nullptr) {
        return nullptr;
    }
    if (type == "text/html") {
        return CreateHtmlData(env, union_obj);
    } else if (type == "text/plain") {
        return CreatePlainTextData(env, union_obj);
    } else if (type == "text/uri") {
        return CreateUriData(env, union_obj);
    } else if (type == "pixelMap") {
        return CreatePixelMapData(env, union_obj);
    } else if (type == "text/want") {
        return CreateWantData(env, union_obj);
    }

    return nullptr;
}

static ani_object CreateDataTypeValue([[maybe_unused]] ani_env *env, ani_string type, ani_object union_obj)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto type_str = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    if (!CheckMimeType(env, type_str)) {
        return GetNullObject(env);
    }
    if (type_str == "text/html" || type_str == "text/plain" ||
        type_str == "text/uri" || type_str == "pixelMap" || type_str == "text/want") {
        return ProcessSpecialMimeType(env, union_obj, type_str);
    }

    std::string classname("C{std.core.ArrayBuffer}");
    bool isArrayBuffer = ANIUtils_UnionIsInstanceOf(env, union_obj, classname);
    if (!isArrayBuffer) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataTypeValue] not ArrayBuffer.");
        return GetNullObject(env);
    }

    void* data;
    size_t length;
    if (ANI_OK != env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(union_obj), &data, &length)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[CreateDataTypeValue] Failed: env->ArrayBuffer_GetInfo().");
        return GetNullObject(env);
    }

    auto pVal = static_cast<uint8_t*>(data);
    std::vector<uint8_t> vec(pVal, pVal + length);

    auto ptr = PasteboardClient::GetInstance()->CreateKvData(type_str, vec);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataTypeValue] CreateKvData failed.");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

bool ParsekeyValAndProcess([[maybe_unused]] ani_env *env, ani_ref key_value, ani_object value_obj,
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> result)
{
    if (env == nullptr) {
        return false;
    }
    std::string keyStr = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(key_value));
    if (!CheckMimeType(env, keyStr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ParsekeyValAndProcess] keyStr is empty or length is more than 1024 bytes.");
        return false;
    }

    std::shared_ptr<EntryValue> entryValue = std::make_shared<EntryValue>();
    std::vector<uint8_t> vec;
    vec.clear();
    if (GetArrayBuffer(env, value_obj, vec)) {
        *entryValue = std::vector<uint8_t>(vec.begin(), vec.end());
        result->emplace_back(std::make_pair(keyStr, entryValue));
        return true;
    }

    if (keyStr == "pixelMap") {
        OHOS::Media::PixelMap* rawPixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnv(env, value_obj);
        if (rawPixelMap == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParsekeyValAndProcess] GetPixelMapFromEnv failed.");
            return false;
        }
        *entryValue = std::shared_ptr<OHOS::Media::PixelMap>(rawPixelMap);
    } else if (keyStr == "text/want") {
        OHOS::AAFwk::Want want;
        if (!OHOS::AppExecFwk::UnwrapWant(env, value_obj, want)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParsekeyValAndProcess] UnwrapWant failed.");
            return false;
        }
        *entryValue = std::make_shared<OHOS::AAFwk::Want>(want);
    } else {
        auto value_str = GetStdStringFromUnion(env, value_obj);
        if (value_str.length() == 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParsekeyValAndProcess] value_str is null.");
            return false;
        }
        *entryValue = value_str;
    }

    result->emplace_back(std::make_pair(keyStr, entryValue));

    return true;
}

bool ForEachMapEntry(ani_env *env, ani_object map_object,
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> typeValueVector)
{
    if (env == nullptr) {
        return false;
    }
    ani_ref keys;
    if (ANI_OK != env->Object_CallMethodByName_Ref(map_object, "keys", ":C{escompat.IterableIterator}", &keys)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to get keys iterator.");
        return false;
    }

    bool success = true;
    while (success) {
        ani_ref next;
        ani_boolean done = false;
        if (ANI_OK != env->Object_CallMethodByName_Ref(
            static_cast<ani_object>(keys), "next", nullptr, &next)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to get next key.");
            success = false;
            break;
        }

        if (ANI_OK != env->Object_GetFieldByName_Boolean(
            static_cast<ani_object>(next), "done", &done)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to check iterator done.");
            success = false;
            break;
        }
        if (done) {
            PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] done break.");
            break;
        }

        ani_ref key_value;
        if (ANI_OK != env->Object_GetFieldByName_Ref(static_cast<ani_object>(next),
            "value", &key_value)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to get key value.");
            success = false;
            break;
        }

        ani_ref value_obj;
        if (ANI_OK != env->Object_CallMethodByName_Ref(map_object, "$_get", nullptr,
            &value_obj, key_value)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "[forEachMapEntry] Failed to get value for key.");
            success = false;
            break;
        }
        ParsekeyValAndProcess(env, key_value, static_cast<ani_object>(value_obj), typeValueVector);
    }
    return success;
}

static ani_object CreateDataRecord([[maybe_unused]] ani_env *env, ani_object map_object)
{
    if (env == nullptr) {
        return nullptr;
    }
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> typeValueVector =
        std::make_shared<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>>();
    ForEachMapEntry(env, map_object, typeValueVector);
    if (typeValueVector == nullptr || typeValueVector->empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataRecord] typeValueVector is null or empty.");
        return GetNullObject(env);
    }

    std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>> typeValueMap =
            std::make_shared<std::map<std::string, std::shared_ptr<EntryValue>>>();
    for (const auto &item : *typeValueVector) {
        typeValueMap->emplace(item.first, item.second);
    }
    auto ptr = PasteboardClient::GetInstance()->CreateMultiTypeData(std::move(typeValueMap),
        typeValueVector->begin()->first);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataRecord] CreateMultiTypeData failed.");
        return GetNullObject(env);
    }

    ani_object PasteDataImpl = Create(env, ptr);
    return PasteDataImpl;
}

static ani_object GetSystemPasteboard([[maybe_unused]] ani_env *env)
{
    if (env == nullptr) {
        return nullptr;
    }
    ani_object systemPasteboard = nullptr;
    static const char *className = "@ohos.pasteboard.SystemPasteboardImpl";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetSystemPasteboard] Not found classname.");
        return systemPasteboard;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetSystemPasteboard] get ctor failed.");
        return systemPasteboard;
    }

    if (ANI_OK != env->Object_New(cls, ctor, &systemPasteboard)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetSystemPasteboard] Create Object failed.");
        return systemPasteboard;
    }

    return systemPasteboard;
}

static ani_boolean HasDataType([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object, ani_string mimeType)
{
    if (env == nullptr) {
        return false;
    }
    auto mimeType_str = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(mimeType));

    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, mimeType_str]() {
        auto ret = PasteboardClient::GetInstance()->HasDataType(mimeType_str);
        std::shared_ptr<int> value = std::make_shared<int>(static_cast<int>(ret));
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[HasDataType] time out. ");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return false;
    }

    return *value;
}

static ani_int SetData([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object, ani_object pasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_ANI, "SetData is called!");
    int32_t ret = static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    auto data = UnwrapAndGetPasteDataPtr(env, pasteData);
    if (data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PasteData is null.");
        return ret;
    }

    std::map<uint32_t, std::shared_ptr<OHOS::UDMF::EntryGetter>> entryGetters;
    for (auto record : data->AllRecords()) {
        if (record != nullptr && record->GetEntryGetter() != nullptr) {
            entryGetters.emplace(record->GetRecordId(), record->GetEntryGetter());
        }
    }
    ret = PasteboardClient::GetInstance()->SetPasteData(*data, nullptr, entryGetters);
    if (ret == static_cast<int>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "SetPasteData successfully.");
    } else if (ret == static_cast<int>(PasteboardError::PROHIBIT_COPY)) {
        ThrowBusinessError(env, static_cast<int32_t>(PasteboardError::PROHIBIT_COPY),
            "The system prohibits copying.");
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "The system prohibits copying.");
    } else if (ret == static_cast<int>(PasteboardError::TASK_PROCESSING)) {
        ThrowBusinessError(env, static_cast<int32_t>(PasteboardError::TASK_PROCESSING),
            "Another setData is being processed.");
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Another setData is being processed.");
    }

    return ret;
}

static void ClearData([[maybe_unused]] ani_env *env)
{
    PasteboardClient::GetInstance()->Clear();
}

static ani_object GetDataSync([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    if (env == nullptr) {
        return nullptr;
    }
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_ANI, "GetDataSync called.");
    auto pasteData = std::make_shared<PasteData>();
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int32_t>>>(SYNC_TIMEOUT);
    std::thread thread([block, pasteData]() mutable {
        auto ret = PasteboardClient::GetInstance()->GetPasteData(*pasteData);
        std::shared_ptr<int32_t> value = std::make_shared<int32_t>(ret);
        block->SetValue(value);
    });
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "time out, GetDataSync failed.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return GetNullObject(env);
    }
    ani_object pasteDataObj = Create(env, pasteData);

    return pasteDataObj;
}

static ani_string GetDataSource([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    if (env == nullptr) {
        return nullptr;
    }
    std::string bundleName;
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int>>>(SYNC_TIMEOUT);
    std::thread thread([block, &bundleName]() mutable {
        auto ret = PasteboardClient::GetInstance()->GetDataSource(bundleName);
        std::shared_ptr<int> value = std::make_shared<int>(ret);
        block->SetValue(value);
    });
    thread.detach();

    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "time out, GetDataSource failed.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return nullptr;
    }

    if (*value != static_cast<int>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "GetDataSource, failed, ret = %{public}d.", *value);
        return nullptr;
    }

    ani_string aniStr = nullptr;
    if (ANI_OK != env->String_NewUTF8(bundleName.data(), bundleName.size(), &aniStr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Unsupported ANI_VERSION_1");
        return nullptr;
    }

    return aniStr;
}

ANI_EXPORT ani_status ANI_Constructor_Namespace(ani_env *env)
{
    if (env == nullptr) {
        return ANI_ERROR;
    }
    ani_namespace ns;
    static const char *nameSpaceName = "@ohos.pasteboard.pasteboard";
    if (ANI_OK != env->FindNamespace(nameSpaceName, &ns)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ANI_Constructor_Namespace] Not found namespace: %s", nameSpaceName);
        return ANI_NOT_FOUND;
    }

    std::array methods = {
        ani_native_function {"createDataTypeValue", nullptr, reinterpret_cast<void *>(CreateDataTypeValue)},
        ani_native_function {"createDataRecord", nullptr, reinterpret_cast<void *>(CreateDataRecord)},
        ani_native_function {"getSystemPasteboard", nullptr, reinterpret_cast<void *>(GetSystemPasteboard)},
    };

    if (ANI_OK != env->Namespace_BindNativeFunctions(ns, methods.data(), methods.size())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ANI_Constructor_Namespace] Cannot bind native methods to %s.", nameSpaceName);
        return ANI_ERROR;
    };

    return ANI_OK;
}

ANI_EXPORT ani_status ANI_Constructor_PasteData(ani_env *env)
{
    if (env == nullptr) {
        return ANI_ERROR;
    }
    static const char *className = "@ohos.pasteboard.PasteDataImpl";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ANI_Constructor_PasteData] Not found %s.", className);
        return ANI_NOT_FOUND;
    }

    std::array methods = {
        ani_native_function {"addRecordByPasteDataRecord",
            nullptr, reinterpret_cast<void *>(AddRecordByPasteDataRecord)},
        ani_native_function {"addRecordByTypeValue", nullptr, reinterpret_cast<void *>(AddRecordByTypeValue)},
        ani_native_function {"getRecordCount", nullptr, reinterpret_cast<void *>(GetRecordCount)},
        ani_native_function {"getRecord", nullptr, reinterpret_cast<void *>(GetRecord)},
        ani_native_function {"setProperty", nullptr, reinterpret_cast<void *>(SetProperty)},
        ani_native_function {"getProperty", nullptr, reinterpret_cast<void *>(GetProperty)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ANI_Constructor_PasteData] Cannot bind native methods to %s.", className);
        return ANI_ERROR;
    };

    return ANI_OK;
}

ANI_EXPORT ani_status ANI_Constructor_SystemPasteboard(ani_env *env)
{
    if (env == nullptr) {
        return ANI_ERROR;
    }
    static const char *className = "@ohos.pasteboard.SystemPasteboardImpl";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ANI_Constructor_SystemPasteboard] Not found %s.", className);
        return ANI_NOT_FOUND;
    }

    std::array methods = {
        ani_native_function {"hasDataType", nullptr, reinterpret_cast<void *>(HasDataType)},
        ani_native_function {"nativeSetData", nullptr, reinterpret_cast<void *>(SetData)},
        ani_native_function {"nativeClearData", nullptr, reinterpret_cast<void *>(ClearData)},
        ani_native_function {"getDataSync", nullptr, reinterpret_cast<void *>(GetDataSync)},
        ani_native_function {"getDataSource", nullptr, reinterpret_cast<void *>(GetDataSource)},
    };

    if (ANI_OK != env->Class_BindNativeMethods(cls, methods.data(), methods.size())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ANI_Constructor_SystemPasteboard] Cannot bind native methods to %s.", className);
        return ANI_ERROR;
    };

    return ANI_OK;
}

static ani_status BindCleanerclassMethods(ani_env *env)
{
    if (env == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[BindCleanerclassMethods] env is null.");
        return ANI_ERROR;
    }

    static const char *className = "@ohos.pasteboard.Cleaner";
    ani_class cleanerCls;
    ani_status status = env->FindClass(className, &cleanerCls);
    if (ANI_OK != status) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[BindCleanerclassMethods] Not found ohos/pasteboard/Cleaner. status:%{public}d.", status);
        return ANI_NOT_FOUND;
    }
    return NativePtrCleaner(env).Bind(cleanerCls);
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    if (vm == nullptr || result == nullptr) {
        return ANI_ERROR;
    }
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ANI_Constructor] Unsupported ANI_VERSION_1.");
        return ANI_ERROR;
    }

    ani_status namespaceStatus = ANI_Constructor_Namespace(env);
    if (namespaceStatus != ANI_OK) {
        return namespaceStatus;
    }

    ani_status pasteDataStatus = ANI_Constructor_PasteData(env);
    if (pasteDataStatus != ANI_OK) {
        return pasteDataStatus;
    }

    ani_status systemboardStatus = ANI_Constructor_SystemPasteboard(env);
    if (systemboardStatus != ANI_OK) {
        return systemboardStatus;
    }

    if (ANI_OK != BindCleanerclassMethods(env)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ANI_Constructor_PasteData]BindCleanerclassMethods failed.");
        return ANI_ERROR;
    }

    *result = ANI_VERSION_1;
    return ANI_OK;
}
