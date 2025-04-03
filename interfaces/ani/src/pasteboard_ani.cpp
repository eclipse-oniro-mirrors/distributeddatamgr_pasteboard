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
#include "common/block_object.h"
#include "ani_common_want.h"
#include "image_ani_utils.h"
#include "pasteboard_ani_utils.h"
#include "unified_meta.h"

using namespace OHOS::MiscServices;

constexpr size_t SYNC_TIMEOUT = 3500;
constexpr size_t MIMETYPE_MAX_LEN = 1024;

ani_object GetNullObject(ani_env *env)
{
    ani_ref ref;
    if (env->GetNull(&ref) != ANI_OK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "env->GetNull failed.");
        return NULL;
    }
    return static_cast<ani_object>(ref);
}

ani_object Create([[maybe_unused]] ani_env *env, std::shared_ptr<PasteData> &ptrPasteData)
{
    static const char *className = "L@ohos/pasteboard/PasteDataImpl;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "create failed.");
        return nullptr;
    }

    auto shareptrPasteData = new SharedPtrHolder<PasteData>(ptrPasteData);
    return NativeObjectWrapper<SharedPtrHolder<PasteData>>::Wrap(env, cls, shareptrPasteData);
}

PasteData* unwrapAndGetPasteDataPtr([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    auto shareptrPasteData = NativeObjectWrapper<SharedPtrHolder<PasteData>>::Unwrap(env, object);
    return shareptrPasteData->Get().get();
}

ani_object CreateObjectFromClass([[maybe_unused]] ani_env *env, const char* className)
{
    ani_object obj = nullptr;
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateObjectFromClass] Not found class. ");
        return obj;
    }
    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateObjectFromClass] get ctor Failed. ");
        return obj;
    }
    if (ANI_OK != env->Object_New(cls, ctor, &obj)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateObjectFromClass] Create Object Failed. ");
        return obj;
    }

    return obj;
}

std::string GetStdStringFromUnion([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    UnionAccessor unionAccessor(env, union_obj);
    ani_string str;
    if (!unionAccessor.TryConvert<ani_string>(str)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[GetStdStringFromUnion] try to convert union object to ani_string failed! ");
        return nullptr;
    }

    return ANIUtils_ANIStringToStdString(env, str);
}

bool getArrayBuffer([[maybe_unused]] ani_env *env, ani_object unionObj, std::vector<uint8_t> &vec)
{
    std::string classname("Lescompat/ArrayBuffer;");
    bool isArrayBuffer = ANIUtils_UnionIsInstanceOf(env, unionObj, classname);
    if (!isArrayBuffer) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[getArrayBuffer] Failed: is not arraybuffer. ");
        return false;
    }

    void* data;
    size_t length;
    if (ANI_OK != env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(unionObj), &data, &length)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[getArrayBuffer] Failed: env->ArrayBuffer_GetInfo(). ");
        return false;
    }

    auto pVal = static_cast<uint8_t*>(data);
    vec.assign(pVal, pVal + length);

    return true;
}

bool CheckMimeType(ani_env *env, std::string &mimeType)
{
    if (mimeType.size() == 0 || mimeType.size() >= MIMETYPE_MAX_LEN) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Parameter error. \
            The length of mimeType cannot be greater than 1024 bytes, or The length of mimeType is 0.");
        // throw JSErrorCode::INVALID_PARAMETERS
        return false;
    }
    return true;
}

static ani_double GetRecordCount([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    PasteData* pPasteData = unwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecordCount] pPasteData is null");
        return 0;
    }

    return pPasteData->GetRecordCount();
}

static void AddRecordByPasteDataRecord([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    ani_object record)
{
    ani_ref uri;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(static_cast<ani_object>(record), "uri", &uri)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecord] Object_GetPropertyByName_Ref Faild");
        return;
    }

    auto uri_str = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(uri));

    PasteData* pPasteData = unwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecord] pPasteData is null");
        return;
    }

    PasteDataRecord::Builder builder("");
    builder.SetUri(std::make_shared<OHOS::Uri>(OHOS::Uri(uri_str)));
    std::shared_ptr<PasteDataRecord> result = builder.Build();
    if (result == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecord] result is null");
        return;
    }
    pPasteData->AddRecord(*(result.get()));
}

static void ProcessStrValueOfRecord([[maybe_unused]] ani_env *env, ani_object union_obj, std::string mimeType,
    PasteData *pPasteData)
{
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ProcessStrValueOfRecord] pPasteData is null. ");
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
    auto mimeType = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    if (!CheckMimeType(env, mimeType)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[AddRecordByTypeValue] minetype length is error. ");
        return;
    }

    PasteData* pPasteData = unwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] pPasteData is null");
        return;
    }

    if (mimeType == MIMETYPE_PIXELMAP) {
        OHOS::Media::PixelMap* rawPixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnv(env, union_obj);
        if (rawPixelMap == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] GetPixelMapFromEnv failed. ");
            return;
        }
        auto pixelMap = std::shared_ptr<OHOS::Media::PixelMap>(rawPixelMap);
        pPasteData->AddPixelMapRecord(pixelMap);
        return;
    } else if (mimeType == MIMETYPE_TEXT_WANT) {
        OHOS::AAFwk::Want want;
        if (!OHOS::AppExecFwk::UnwrapWant(env, union_obj, want)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] UnwrapWant failed. ");
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
    if (!getArrayBuffer(env, union_obj, vec)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[AddRecordByTypeValue] getArrayBuffer failed. ");
        return;
    }
    pPasteData->AddKvRecord(mimeType, vec);

    return;
}

static ani_object GetRecord([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object,
    [[maybe_unused]] ani_double index)
{
    PasteData* pPasteData = unwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr || index >= pPasteData->GetRecordCount()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] pPasteData is null or index is out of range.");
        return GetNullObject(env);
    }

    std::shared_ptr<PasteDataRecord> recordFromBottom = pPasteData->GetRecordAt(static_cast<std::size_t>(index));
    if (recordFromBottom == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] recordFromBottom is null.");
        return GetNullObject(env);
    }
    ani_string uri_string{};
    auto uriptr = recordFromBottom->GetUri();
    if (uriptr != nullptr) {
        env->String_NewUTF8(uriptr->ToString().c_str(), uriptr->ToString().length(), &uri_string);
    }

    ani_object record = CreateObjectFromClass(env, "L@ohos/pasteboard/PasteDataRecordImpl;");
    if (record == nullptr) {
        return GetNullObject(env);
    }

    ani_class cls;
    if (ANI_OK != env->FindClass("L@ohos/pasteboard/PasteDataRecordImpl;", &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] Not found. ");
        return record;
    }

    ani_method uriSetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<set>uri", nullptr, &uriSetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] Class_FindMethod Fail. ");
        return record;
    }
    if (ANI_OK != env->Object_CallMethod_Void(record, uriSetter, uri_string)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetRecord] Object_CallMethod_Void Fail. ");
        return record;
    }

    return record;
}

static void SetProperty([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object, ani_object property)
{
    PasteData* pPasteData = unwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetProperty] pPasteData is null. ");
        return;
    }

    ani_double shareOptionValue;
    if (ANI_OK != env->Object_GetPropertyByName_Double(static_cast<ani_object>(property),
        "shareOption", &shareOptionValue)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetProperty] Object_GetPropertyByName_Int Faild. ");
        return;
    }

    ani_int localOnlyValue = shareOptionValue == ShareOption::CrossDevice ? false : true;
    pPasteData->SetLocalOnly(localOnlyValue);
    pPasteData->SetShareOption(static_cast<ShareOption>(shareOptionValue));

    return;
}

static ani_object GetProperty([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
    PasteData* pPasteData = unwrapAndGetPasteDataPtr(env, object);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetProperty] pPasteData is null. ");
        return GetNullObject(env);
    }

    PasteDataProperty property = pPasteData->GetProperty();
    ani_double shareOpionValue = property.shareOption;

    ani_object propertyToAbove = CreateObjectFromClass(env, "L@ohos/pasteboard/PasteDataPropertyImpl;");
    if (propertyToAbove == nullptr) {
        return GetNullObject(env);
    }

    ani_class cls;
    if (ANI_OK != env->FindClass("L@ohos/pasteboard/PasteDataPropertyImpl;", &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetProperty] Not found class. ");
        return propertyToAbove;
    }

    ani_method shareOptionSetter;
    if (ANI_OK != env->Class_FindMethod(cls, "<set>shareOption", nullptr, &shareOptionSetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetProperty] Class_FindMethod Fail. ");
        return propertyToAbove;
    }
    if (ANI_OK != env->Object_CallMethod_Void(propertyToAbove, shareOptionSetter, shareOpionValue)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetProperty] Object_CallMethod_Void Fail. ");
        return propertyToAbove;
    }

    return propertyToAbove;
}

static ani_object CreateHtmlData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    auto value_str = GetStdStringFromUnion(env, union_obj);
    auto ptr = PasteboardClient::GetInstance()->CreateHtmlData(value_str);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateHtmlData] CreateHtmlData failed ");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreatePlainTextData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    auto value_str = GetStdStringFromUnion(env, union_obj);
    auto ptr = PasteboardClient::GetInstance()->CreatePlainTextData(value_str);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreatePlainTextData] CreatePlainTextData failed. ");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreateUriData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    auto value_str = GetStdStringFromUnion(env, union_obj);
    auto ptr = PasteboardClient::GetInstance()->CreateUriData(OHOS::Uri(value_str));
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateUriData] CreateUriData failed. ");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreatePixelMapData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    OHOS::Media::PixelMap* rawPixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnv(env, union_obj);
    if (rawPixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreatePixelMapData] GetPixelMapFromEnv failed. ");
        return GetNullObject(env);
    }

    auto pixelMap = std::shared_ptr<OHOS::Media::PixelMap>(rawPixelMap);
    auto ptr = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMap);
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

static ani_object CreateWantData([[maybe_unused]] ani_env *env, ani_object union_obj)
{
    OHOS::AAFwk::Want want;
    bool ret = OHOS::AppExecFwk::UnwrapWant(env, union_obj, want);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateWantData] UnwrapWant failed. ");
        return GetNullObject(env);
    }

    auto ptr = PasteboardClient::GetInstance()->CreateWantData(std::make_shared<OHOS::AAFwk::Want>(want));
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateWantData] CreateWantData failed. ");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

ani_object processSpecialMimeType([[maybe_unused]] ani_env *env, ani_object union_obj, std::string type)
{
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
    auto type_str = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    if (!CheckMimeType(env, type_str)) {
        return GetNullObject(env);
    }
    if (type_str == "text/html" || type_str == "text/plain" ||
        type_str == "text/uri" || type_str == "pixelMap" || type_str == "text/want") {
        return processSpecialMimeType(env, union_obj, type_str);
    }

    std::string classname("Lescompat/ArrayBuffer;");
    bool isArrayBuffer = ANIUtils_UnionIsInstanceOf(env, union_obj, classname);
    if (!isArrayBuffer) {
        return GetNullObject(env);
    }

    void* data;
    size_t length;
    if (ANI_OK != env->ArrayBuffer_GetInfo(static_cast<ani_arraybuffer>(union_obj), &data, &length)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[CreateDataTypeValue] Failed: env->ArrayBuffer_GetInfo(). ");
        return GetNullObject(env);
    }

    auto pVal = static_cast<uint8_t*>(data);
    std::vector<uint8_t> vec(pVal, pVal + length);

    auto ptr = PasteboardClient::GetInstance()->CreateKvData(type_str, vec);
    if (ptr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataTypeValue] CreateKvData failed. ");
        return GetNullObject(env);
    }
    ani_object PasteDataImpl = Create(env, ptr);

    return PasteDataImpl;
}

bool ParsekeyValAndProcess([[maybe_unused]] ani_env *env, ani_ref key_value, ani_object value_obj,
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> result)
{
    std::string keyStr = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(key_value));
    if (keyStr.length() == 0 || keyStr.length() > MIMETYPE_MAX_LEN) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParsekeyValAndProcess] keyStr is null or length is too big.");
        return false;
    }

    std::shared_ptr<EntryValue> entryValue = std::make_shared<EntryValue>();
    std::vector<uint8_t> vec;
    vec.clear();
    if (getArrayBuffer(env, value_obj, vec)) {
        *entryValue = std::vector<uint8_t>(vec.begin(), vec.end());
        result->emplace_back(std::make_pair(keyStr, entryValue));
        return true;
    }

    if (keyStr == "pixelMap") {
        OHOS::Media::PixelMap* rawPixelMap = OHOS::Media::ImageAniUtils::GetPixelMapFromEnv(env, value_obj);
        if (rawPixelMap == nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParsekeyValAndProcess] GetPixelMapFromEnv failed. ");
            return false;
        }
        *entryValue = std::shared_ptr<OHOS::Media::PixelMap>(rawPixelMap);
    } else if (keyStr == "text/want") {
        OHOS::AAFwk::Want want;
        if (!OHOS::AppExecFwk::UnwrapWant(env, value_obj, want)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParsekeyValAndProcess] UnwrapWant failed. ");
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

bool forEachMapEntry(ani_env *env, ani_object map_object,
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> typeValueVector)
{
    ani_ref keys;
    if (ANI_OK != env->Object_CallMethodByName_Ref(map_object, "keys", ":Lescompat/IterableIterator;", &keys)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to get keys iterator. ");
        return false;
    }

    bool success = true;
    while (success) {
        ani_ref next;
        ani_boolean done;

        if (ANI_OK != env->Object_CallMethodByName_Ref(
            static_cast<ani_object>(keys), "next", nullptr, &next)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to get next key. ");
            success = false;
            break;
        }

        if (ANI_OK != env->Object_GetFieldByName_Boolean(
            static_cast<ani_object>(next), "done", &done)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to check iterator done. ");
            success = false;
            break;
        }
        if (done) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] done break. ");
            break;
        }

        ani_ref key_value;
        if (ANI_OK != env->Object_GetFieldByName_Ref(static_cast<ani_object>(next),
            "value", &key_value)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[forEachMapEntry] Failed to get key value. ");
            success = false;
            break;
        }

        ani_ref value_obj;
        if (ANI_OK != env->Object_CallMethodByName_Ref(map_object, "$_get", nullptr,
            &value_obj, key_value)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "[forEachMapEntry] Failed to get value for key. ");
            success = false;
            break;
        }
        ParsekeyValAndProcess(env, key_value, static_cast<ani_object>(value_obj), typeValueVector);
    }
    return success;
}

static ani_object CreateDataRecord([[maybe_unused]] ani_env *env, ani_object map_object)
{
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> typeValueVector =
        std::make_shared<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>>();
    forEachMapEntry(env, map_object, typeValueVector);
    if (typeValueVector == nullptr || typeValueVector->empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataRecord] typeValueVector is null or empty. ");
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
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateDataRecord] CreateMultiTypeData failed. ");
        return GetNullObject(env);
    }

    ani_object PasteDataImpl = Create(env, ptr);
    return PasteDataImpl;
}

static ani_object GetSystemPasteboard([[maybe_unused]] ani_env *env)
{
    ani_object systemPasteboard = nullptr;
    static const char *className = "L@ohos/pasteboard/SystemPasteboardImpl;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetSystemPasteboard] Not found classname. ");
        return systemPasteboard;
    }

    ani_method ctor;
    if (ANI_OK != env->Class_FindMethod(cls, "<ctor>", nullptr, &ctor)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetSystemPasteboard] get ctor Failed. ");
        return systemPasteboard;
    }

    if (ANI_OK != env->Object_New(cls, ctor, &systemPasteboard)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetSystemPasteboard] Create Object Failed. ");
        return systemPasteboard;
    }

    return systemPasteboard;
}

static ani_boolean HasDataType([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object, ani_string mimeType)
{
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
        return false;
    }

    return *value;
}

static ani_int SetData([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object, ani_object pasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_ANI, "SetData is called!");
    int32_t ret = static_cast<int32_t>(PasteboardError::INVALID_DATA_ERROR);
    auto data = unwrapAndGetPasteDataPtr(env, pasteData);
    if (data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "PasteData is null");
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
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "SetPasteData successfully");
    } else if (ret == static_cast<int>(PasteboardError::PROHIBIT_COPY)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "The system prohibits copying");
    } else if (ret == static_cast<int>(PasteboardError::TASK_PROCESSING)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "Another setData is being processed");
    }

    return ret;
}

static void ClearData([[maybe_unused]] ani_env *env)
{
    PasteboardClient::GetInstance()->Clear();
}

static ani_object GetDataSync([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
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
        return GetNullObject(env);
    }
    ani_object pasteDataObj = Create(env, pasteData);

    return pasteDataObj;
}

static ani_string GetDataSource([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object)
{
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
        return nullptr;
    }

    if (*value != static_cast<int>(PasteboardError::E_OK)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "GetDataSource, failed, ret = %{public}d", *value);
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
    ani_namespace ns;
    static const char *nameSpaceName = "L@ohos/pasteboard/pasteboard;";
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
            "[ANI_Constructor_Namespace] Cannot bind native methods to %s", nameSpaceName);
        return ANI_ERROR;
    };

    return ANI_OK;
}

ANI_EXPORT ani_status ANI_Constructor_PasteData(ani_env *env)
{
    static const char *className = "L@ohos/pasteboard/PasteDataImpl;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ANI_Constructor_PasteData] Not found %s", className);
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
            "[ANI_Constructor_PasteData] Cannot bind native methods to %s", className);
        return ANI_ERROR;
    };

    return ANI_OK;
}

ANI_EXPORT ani_status ANI_Constructor_SystemPasteboard(ani_env *env)
{
    static const char *className = "L@ohos/pasteboard/SystemPasteboardImpl;";
    ani_class cls;
    if (ANI_OK != env->FindClass(className, &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ANI_Constructor_SystemPasteboard] Not found %s", className);
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
            "[ANI_Constructor_SystemPasteboard] Cannot bind native methods to %s", className);
        return ANI_ERROR;
    };

    return ANI_OK;
}

ANI_EXPORT ani_status ANI_Constructor(ani_vm *vm, uint32_t *result)
{
    ani_env *env;
    if (ANI_OK != vm->GetEnv(ANI_VERSION_1, &env)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ANI_Constructor] Unsupported ANI_VERSION_1");
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

    *result = ANI_VERSION_1;
    return ANI_OK;
}
