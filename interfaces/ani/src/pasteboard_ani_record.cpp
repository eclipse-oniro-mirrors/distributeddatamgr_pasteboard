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

#include "pasteboard_ani_record.h"

#include "ani_common_want.h"
#include "common/block_object.h"
#include "common/pasteboard_common_utils.h"
#include "image_ani_utils.h"
#include "pasteboard_ani_utils.h"
#include "pasteboard_hilog.h"
#include "pasteboard_js_err.h"
#include "unified_meta.h"

using namespace OHOS::MiscServices;

constexpr size_t RECORD_SYNC_TIMEOUT = 3500;

extern ani_object Create([[maybe_unused]] ani_env *env, std::shared_ptr<PasteData> &ptrPasteData);
extern PasteData *UnwrapAndGetPasteDataPtr([[maybe_unused]] ani_env *env, [[maybe_unused]] ani_object object);
extern ani_object CreateObjectFromClass([[maybe_unused]] ani_env *env, const char *className);
extern void ThrowBusinessError(ani_env *env, int errCode, std::string &&errMsg);
extern ani_object GetNullObject(ani_env *env);
extern std::string GetStdStringFromUnion([[maybe_unused]] ani_env *env, ani_object union_obj);
extern bool GetArrayBuffer([[maybe_unused]] ani_env *env, ani_object unionObj, std::vector<uint8_t> &vec);
extern bool CheckMimeType(ani_env *env, std::string &mimeType);

static void SetNamedPropertyByStrEx(ani_env *env, ani_class cls, const char *propertyName,
    std::string propertyValue, ani_object &obj)
{
    if (env == nullptr || propertyName == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByStrEx] env or propertyName is null.");
        return;
    }
    ani_string propertyAniStr = {};
    env->String_NewUTF8(propertyValue.c_str(), propertyValue.length(), &propertyAniStr);
    ani_method propertySetter;
    if (ANI_OK != env->Class_FindMethod(cls, propertyName, nullptr, &propertySetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[SetNamedPropertyByStrEx] Class_FindMethod failed: %{public}s.", propertyName);
        return;
    }
    if (ANI_OK != env->Object_CallMethod_Void(obj, propertySetter, propertyAniStr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[SetNamedPropertyByStrEx] Object_CallMethod_Void failed: %{public}s.", propertyName);
        return;
    }
}

static void SetNamedPropertyByLongEx(ani_env *env, ani_class cls, const char *propertyName,
    ani_long propertyValue, ani_object &obj)
{
    if (env == nullptr || propertyName == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetNamedPropertyByLongEx] env or propertyName is null.");
        return;
    }
    ani_method propertySetter;
    if (ANI_OK != env->Class_FindMethod(cls, propertyName, nullptr, &propertySetter)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[SetNamedPropertyByLongEx] Class_FindMethod failed: %{public}s.", propertyName);
        return;
    }
    if (ANI_OK != env->Object_CallMethod_Void(obj, propertySetter, propertyValue)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[SetNamedPropertyByLongEx] Object_CallMethod_Void failed: %{public}s.", propertyName);
        return;
    }
}

ani_object PasteboardAniRecordUtils::CreateRecordObject(ani_env *env)
{
    if (env == nullptr) {
        return nullptr;
    }
    return CreateObjectFromClass(env, "@ohos.pasteboard.PasteDataRecordImpl");
}

bool PasteboardAniRecordUtils::FillRecordMimeType(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordMimeType] env or record is null.");
        return false;
    }
    auto mimeType = record->GetMimeType();
    SetNamedPropertyByStrEx(env, cls, "<set>mimeType", mimeType, obj);
    return true;
}

bool PasteboardAniRecordUtils::FillRecordPlainText(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordPlainText] env or record is null.");
        return false;
    }
    auto plainTextPtr = record->GetPlainText();
    if (plainTextPtr != nullptr) {
        SetNamedPropertyByStrEx(env, cls, "<set>plainText", *plainTextPtr.get(), obj);
    }
    return true;
}

bool PasteboardAniRecordUtils::FillRecordUri(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordUri] env or record is null.");
        return false;
    }
    auto uriPtr = record->GetUri();
    if (uriPtr != nullptr) {
        SetNamedPropertyByStrEx(env, cls, "<set>uri", uriPtr->ToString(), obj);
    }
    return true;
}

bool PasteboardAniRecordUtils::FillRecordHtmlText(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordHtmlText] env or record is null.");
        return false;
    }
    auto htmlTextPtr = record->GetHtmlText();
    if (htmlTextPtr != nullptr) {
        SetNamedPropertyByStrEx(env, cls, "<set>htmlText", *htmlTextPtr.get(), obj);
    }
    return true;
}

bool PasteboardAniRecordUtils::FillRecordWant(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordWant] env or record is null.");
        return false;
    }
    auto wantPtr = record->GetWant();
    if (wantPtr != nullptr) {
        ani_object wantObj = OHOS::AppExecFwk::WrapWant(env, *wantPtr);
        if (wantObj != nullptr) {
            ani_method wantSetter;
            if (ANI_OK == env->Class_FindMethod(cls, "<set>want", nullptr, &wantSetter)) {
                env->Object_CallMethod_Void(obj, wantSetter, wantObj);
            }
        }
    }
    return true;
}

bool PasteboardAniRecordUtils::FillRecordPixelMap(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordPixelMap] env or record is null.");
        return false;
    }
    auto pixelMapPtr = record->GetPixelMap();
    if (pixelMapPtr != nullptr) {
        ani_object pixelMapObj = OHOS::Media::ImageAniUtils::GetAniPixelMapFromPixelMap(env, pixelMapPtr);
        if (pixelMapObj != nullptr) {
            ani_method pixelMapSetter;
            if (ANI_OK == env->Class_FindMethod(cls, "<set>pixelMap", nullptr, &pixelMapSetter)) {
                env->Object_CallMethod_Void(obj, pixelMapSetter, pixelMapObj);
            }
        }
    }
    return true;
}

bool PasteboardAniRecordUtils::FillRecordData(ani_env *env, ani_class cls, ani_object &obj,
    std::shared_ptr<PasteDataRecord> record)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordData] env or record is null.");
        return false;
    }
    auto customData = record->GetCustomData();
    if (customData == nullptr) {
        return true;
    }
    auto dataMap = customData->GetItemData();
    for (auto &item : dataMap) {
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_ANI, "[FillRecordData] key=%{public}s.", item.first.c_str());
    }
    return true;
}

bool PasteboardAniRecordUtils::FillRecordObject(ani_env *env, std::shared_ptr<PasteDataRecord> record,
    ani_object &obj)
{
    if (env == nullptr || record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordObject] env or record is null.");
        return false;
    }
    ani_class cls;
    if (ANI_OK != env->FindClass("@ohos.pasteboard.PasteDataRecordImpl", &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[FillRecordObject] Not found class.");
        return false;
    }
    FillRecordMimeType(env, cls, obj, record);
    FillRecordPlainText(env, cls, obj, record);
    FillRecordUri(env, cls, obj, record);
    FillRecordHtmlText(env, cls, obj, record);
    FillRecordWant(env, cls, obj, record);
    FillRecordPixelMap(env, cls, obj, record);
    FillRecordData(env, cls, obj, record);
    return true;
}

std::string PasteboardAniRecordUtils::GetMimeTypeFromRecord(ani_env *env, ani_object recordObj)
{
    if (env == nullptr) {
        return "";
    }
    ani_ref mimeTypeRef;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(recordObj, "mimeType", &mimeTypeRef)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetMimeTypeFromRecord] get mimeType failed.");
        return "";
    }
    return ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(mimeTypeRef));
}

std::string PasteboardAniRecordUtils::GetPlainTextFromRecord(ani_env *env, ani_object recordObj)
{
    if (env == nullptr) {
        return "";
    }
    ani_ref plainTextRef;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(recordObj, "plainText", &plainTextRef)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPlainTextFromRecord] get plainText failed.");
        return "";
    }
    return ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(plainTextRef));
}

std::string PasteboardAniRecordUtils::GetUriFromRecord(ani_env *env, ani_object recordObj)
{
    if (env == nullptr) {
        return "";
    }
    ani_ref uriRef;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(recordObj, "uri", &uriRef)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetUriFromRecord] get uri failed.");
        return "";
    }
    return ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(uriRef));
}

std::string PasteboardAniRecordUtils::GetHtmlTextFromRecord(ani_env *env, ani_object recordObj)
{
    if (env == nullptr) {
        return "";
    }
    ani_ref htmlTextRef;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(recordObj, "htmlText", &htmlTextRef)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetHtmlTextFromRecord] get htmlText failed.");
        return "";
    }
    return ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(htmlTextRef));
}

std::vector<uint8_t> PasteboardAniRecordUtils::GetDataFromRecord(ani_env *env, ani_object recordObj)
{
    std::vector<uint8_t> result;
    if (env == nullptr) {
        return result;
    }
    ani_ref dataRef;
    if (ANI_OK != env->Object_GetPropertyByName_Ref(recordObj, "data", &dataRef)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetDataFromRecord] get data failed.");
        return result;
    }
    GetArrayBuffer(env, static_cast<ani_object>(dataRef), result);
    return result;
}

std::shared_ptr<PasteDataRecord> PasteboardAniRecordUtils::ParseRecordFromAni(ani_env *env, ani_object recordObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    std::string mimeType = GetMimeTypeFromRecord(env, recordObj);
    if (mimeType.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ParseRecordFromAni] mimeType is empty.");
        return nullptr;
    }
    PasteDataRecord::Builder builder(mimeType);
    if (mimeType == MIMETYPE_TEXT_HTML) {
        std::string htmlText = GetHtmlTextFromRecord(env, recordObj);
        builder.SetHtmlText(std::make_shared<std::string>(htmlText));
    } else if (mimeType == MIMETYPE_TEXT_PLAIN) {
        std::string plainText = GetPlainTextFromRecord(env, recordObj);
        builder.SetText(std::make_shared<std::string>(plainText));
    } else if (mimeType == MIMETYPE_TEXT_URI) {
        std::string uri = GetUriFromRecord(env, recordObj);
        builder.SetUri(std::make_shared<OHOS::Uri>(uri));
    } else {
        std::vector<uint8_t> data = GetDataFromRecord(env, recordObj);
        if (!data.empty()) {
            builder.SetCustomData(std::make_shared<PasteDataRecord::CustomData>(mimeType, data));
        }
    }
    return builder.Build();
}

bool PasteboardAniRecordUtils::ValidateMimeType(ani_env *env, const std::string &mimeType)
{
    if (env == nullptr) {
        return false;
    }
    if (mimeType.empty()) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ValidateMimeType] mimeType is empty.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The length of mimeType is 0.");
        return false;
    }
    if (mimeType.size() > ANI_MIMETYPE_MAX_LEN) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ValidateMimeType] mimeType length exceeds 1024 bytes.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The length of mimeType cannot be greater than 1024 bytes.");
        return false;
    }
    return true;
}

bool PasteboardAniRecordUtils::ValidateRecordCount(ani_env *env, int32_t count)
{
    if (env == nullptr) {
        return false;
    }
    if (count >= ANI_MAX_RECORD_NUM) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ValidateRecordCount] record count exceeds limit: %{public}d.", count);
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::RECORD_EXCEEDS_LIMIT),
            "The number of records exceeds the maximum limit.");
        return false;
    }
    return true;
}

bool PasteboardAniRecordUtils::ValidateTextLength(ani_env *env, const std::string &text)
{
    if (env == nullptr) {
        return false;
    }
    if (text.size() > ANI_MAX_TEXT_LENGTH) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ValidateTextLength] text length exceeds limit: %{public}zu.", text.size());
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The text length exceeds the maximum limit.");
        return false;
    }
    return true;
}

bool PasteboardAniRecordUtils::ValidateHtmlLength(ani_env *env, const std::string &html)
{
    if (env == nullptr) {
        return false;
    }
    if (html.size() > ANI_MAX_HTML_LENGTH) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[ValidateHtmlLength] html length exceeds limit: %{public}zu.", html.size());
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "The html length exceeds the maximum limit.");
        return false;
    }
    return true;
}

ani_object PasteboardAniDataAdapter::CreatePasteDataFromRecord(ani_env *env, ani_object recordObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto record = PasteboardAniRecordUtils::ParseRecordFromAni(env, recordObj);
    if (record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreatePasteDataFromRecord] parse record failed.");
        return GetNullObject(env);
    }
    auto ptr = std::make_shared<PasteData>();
    ptr->AddRecord(*record.get());
    return Create(env, ptr);
}

ani_object PasteboardAniDataAdapter::CreatePasteDataFromMimeTypeValue(ani_env *env, ani_string type,
    ani_object value)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto mimeType = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    if (!PasteboardAniRecordUtils::ValidateMimeType(env, mimeType)) {
        return GetNullObject(env);
    }
    if (mimeType == MIMETYPE_TEXT_HTML || mimeType == MIMETYPE_TEXT_PLAIN ||
        mimeType == MIMETYPE_TEXT_URI || mimeType == MIMETYPE_PIXELMAP || mimeType == MIMETYPE_TEXT_WANT) {
        auto valueStr = GetStdStringFromUnion(env, value);
        if (mimeType == MIMETYPE_TEXT_HTML) {
            auto ptr = PasteboardClient::GetInstance()->CreateHtmlData(valueStr);
            return Create(env, ptr);
        } else if (mimeType == MIMETYPE_TEXT_PLAIN) {
            auto ptr = PasteboardClient::GetInstance()->CreatePlainTextData(valueStr);
            return Create(env, ptr);
        } else if (mimeType == MIMETYPE_TEXT_URI) {
            auto ptr = PasteboardClient::GetInstance()->CreateUriData(OHOS::Uri(valueStr));
            return Create(env, ptr);
        }
    }
    return GetNullObject(env);
}

ani_object PasteboardAniDataAdapter::CreateRecordFromMimeTypeValue(ani_env *env, ani_string type,
    ani_object value)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto mimeType = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    if (!PasteboardAniRecordUtils::ValidateMimeType(env, mimeType)) {
        return GetNullObject(env);
    }
    auto recordObj = PasteboardAniRecordUtils::CreateRecordObject(env);
    if (recordObj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[CreateRecordFromMimeTypeValue] create record failed.");
        return GetNullObject(env);
    }
    ani_class cls;
    if (ANI_OK != env->FindClass("@ohos.pasteboard.PasteDataRecordImpl", &cls)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI,
            "[CreateRecordFromMimeTypeValue] Not found class.");
        return GetNullObject(env);
    }
    SetNamedPropertyByStrEx(env, cls, "<set>mimeType", mimeType, recordObj);
    if (mimeType == MIMETYPE_TEXT_HTML) {
        auto htmlStr = GetStdStringFromUnion(env, value);
        SetNamedPropertyByStrEx(env, cls, "<set>htmlText", htmlStr, recordObj);
    } else if (mimeType == MIMETYPE_TEXT_PLAIN) {
        auto textStr = GetStdStringFromUnion(env, value);
        SetNamedPropertyByStrEx(env, cls, "<set>plainText", textStr, recordObj);
    } else if (mimeType == MIMETYPE_TEXT_URI) {
        auto uriStr = GetStdStringFromUnion(env, value);
        SetNamedPropertyByStrEx(env, cls, "<set>uri", uriStr, recordObj);
    }
    return recordObj;
}

ani_object PasteboardAniDataAdapter::GetPrimaryHtml(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryHtml] pPasteData is null.");
        return GetNullObject(env);
    }
    auto htmlPtr = pPasteData->GetPrimaryHtml();
    if (htmlPtr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryHtml] htmlPtr is null.");
        return GetNullObject(env);
    }
    ani_string aniStr = nullptr;
    env->String_NewUTF8(htmlPtr->c_str(), htmlPtr->size(), &aniStr);
    return static_cast<ani_object>(aniStr);
}

ani_object PasteboardAniDataAdapter::GetPrimaryText(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryText] pPasteData is null.");
        return GetNullObject(env);
    }
    auto textPtr = pPasteData->GetPrimaryText();
    if (textPtr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryText] textPtr is null.");
        return GetNullObject(env);
    }
    ani_string aniStr = nullptr;
    env->String_NewUTF8(textPtr->c_str(), textPtr->size(), &aniStr);
    return static_cast<ani_object>(aniStr);
}

ani_object PasteboardAniDataAdapter::GetPrimaryUri(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryUri] pPasteData is null.");
        return GetNullObject(env);
    }
    auto uriPtr = pPasteData->GetPrimaryUri();
    if (uriPtr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryUri] uriPtr is null.");
        return GetNullObject(env);
    }
    auto uriStr = uriPtr->ToString();
    ani_string aniStr = nullptr;
    env->String_NewUTF8(uriStr.c_str(), uriStr.size(), &aniStr);
    return static_cast<ani_object>(aniStr);
}

ani_object PasteboardAniDataAdapter::GetPrimaryMimeType(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryMimeType] pPasteData is null.");
        return GetNullObject(env);
    }
    auto mimeType = pPasteData->GetPrimaryMimeType();
    ani_string aniStr = nullptr;
    env->String_NewUTF8(mimeType.c_str(), mimeType.size(), &aniStr);
    return static_cast<ani_object>(aniStr);
}

ani_object PasteboardAniDataAdapter::GetPrimaryWant(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryWant] pPasteData is null.");
        return GetNullObject(env);
    }
    auto wantPtr = pPasteData->GetPrimaryWant();
    if (wantPtr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryWant] wantPtr is null.");
        return GetNullObject(env);
    }
    ani_object wantObj = OHOS::AppExecFwk::WrapWant(env, *wantPtr);
    if (wantObj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryWant] WrapWant failed.");
        return GetNullObject(env);
    }
    return wantObj;
}

ani_object PasteboardAniDataAdapter::GetPrimaryPixelMap(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryPixelMap] pPasteData is null.");
        return GetNullObject(env);
    }
    auto pixelMapPtr = pPasteData->GetPrimaryPixelMap();
    if (pixelMapPtr == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetPrimaryPixelMap] pixelMapPtr is null.");
        return GetNullObject(env);
    }
    ani_object pixelMapObj = OHOS::Media::ImageAniUtils::GetAniPixelMapFromPixelMap(env, pixelMapPtr);
    return pixelMapObj;
}

ani_array PasteboardAniDataAdapter::GetMimeTypes(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetMimeTypes] pPasteData is null.");
        return nullptr;
    }
    auto mimeTypes = pPasteData->GetMimeTypes();
    ani_array array = nullptr;
    ani_size size = static_cast<ani_size>(mimeTypes.size());
    if (ANI_OK != env->Array_New_String(size, &array)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetMimeTypes] Array_New_String failed.");
        return nullptr;
    }
    for (ani_size i = 0; i < size; ++i) {
        ani_string aniStr = nullptr;
        env->String_NewUTF8(mimeTypes[i].c_str(), mimeTypes[i].size(), &aniStr);
        env->Array_Set_String(array, i, aniStr);
    }
    return array;
}

ani_string PasteboardAniDataAdapter::GetTag(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return nullptr;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetTag] pPasteData is null.");
        return nullptr;
    }
    auto tag = pPasteData->GetTag();
    ani_string aniStr = nullptr;
    env->String_NewUTF8(tag.c_str(), tag.size(), &aniStr);
    return aniStr;
}

ani_boolean PasteboardAniDataAdapter::HasType(ani_env *env, ani_object pasteDataObj, ani_string type)
{
    if (env == nullptr) {
        return false;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[HasType] pPasteData is null.");
        return false;
    }
    auto mimeType = ANIUtils_ANIStringToStdString(env, static_cast<ani_string>(type));
    return pPasteData->HasMimeType(mimeType);
}

void PasteboardAniDataAdapter::RemoveRecord(ani_env *env, ani_object pasteDataObj, ani_int index)
{
    if (env == nullptr) {
        return;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[RemoveRecord] pPasteData is null.");
        return;
    }
    if (index < 0 || index >= static_cast<ani_int>(pPasteData->GetRecordCount())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[RemoveRecord] index out of range.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::OUT_OF_RANGE), "index out of range.");
        return;
    }
    pPasteData->RemoveRecordAt(static_cast<std::size_t>(index));
}

void PasteboardAniDataAdapter::ReplaceRecord(ani_env *env, ani_object pasteDataObj, ani_int index,
    ani_object recordObj)
{
    if (env == nullptr) {
        return;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ReplaceRecord] pPasteData is null.");
        return;
    }
    auto record = PasteboardAniRecordUtils::ParseRecordFromAni(env, recordObj);
    if (record == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ReplaceRecord] parse record failed.");
        return;
    }
    if (index < 0 || index >= static_cast<ani_int>(pPasteData->GetRecordCount())) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ReplaceRecord] index out of range.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::OUT_OF_RANGE), "index out of range.");
        return;
    }
    pPasteData->ReplaceRecordAt(static_cast<std::size_t>(index), record);
}

void PasteboardAniDataAdapter::PasteStart(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[PasteStart] pPasteData is null.");
        return;
    }
    pPasteData->PasteStart();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "PasteStart called.");
}

void PasteboardAniDataAdapter::PasteComplete(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return;
    }
    PasteData *pPasteData = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (pPasteData == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[PasteComplete] pPasteData is null.");
        return;
    }
    pPasteData->PasteComplete();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_ANI, "PasteComplete called.");
}

ani_boolean PasteboardAniSystemAdapter::HasData(ani_env *env)
{
    if (env == nullptr) {
        return false;
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<bool>>>(RECORD_SYNC_TIMEOUT);
    std::thread thread([block]() {
        bool ret = PasteboardClient::GetInstance()->HasPasteData();
        auto value = std::make_shared<bool>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniHasData");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[HasData] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return false;
    }
    return *value;
}

ani_boolean PasteboardAniSystemAdapter::HasDataSync(ani_env *env)
{
    return HasData(env);
}

ani_boolean PasteboardAniSystemAdapter::HasRemoteData(ani_env *env)
{
    if (env == nullptr) {
        return false;
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<bool>>>(RECORD_SYNC_TIMEOUT);
    std::thread thread([block]() {
        bool ret = PasteboardClient::GetInstance()->HasRemoteData();
        auto value = std::make_shared<bool>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniHasRemoteData");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[HasRemoteData] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return false;
    }
    return *value;
}

ani_boolean PasteboardAniSystemAdapter::IsRemoteData(ani_env *env)
{
    if (env == nullptr) {
        return false;
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<bool>>>(RECORD_SYNC_TIMEOUT);
    std::thread thread([block]() {
        bool ret = PasteboardClient::GetInstance()->IsRemoteData();
        auto value = std::make_shared<bool>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniIsRemoteData");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[IsRemoteData] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return false;
    }
    return *value;
}

void PasteboardAniSystemAdapter::ClearDataSync(ani_env *env)
{
    if (env == nullptr) {
        return;
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int32_t>>>(RECORD_SYNC_TIMEOUT);
    std::thread thread([block]() {
        PasteboardClient::GetInstance()->Clear();
        auto value = std::make_shared<int32_t>(0);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniClearDataSync");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[ClearDataSync] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
    }
}

void PasteboardAniSystemAdapter::SetDataSync(ani_env *env, ani_object pasteDataObj)
{
    if (env == nullptr) {
        return;
    }
    auto data = UnwrapAndGetPasteDataPtr(env, pasteDataObj);
    if (data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetDataSync] data is null.");
        return;
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int32_t>>>(RECORD_SYNC_TIMEOUT);
    std::thread thread([block, data]() {
        std::map<uint32_t, std::shared_ptr<OHOS::UDMF::EntryGetter>> entryGetters;
        for (auto record : data->AllRecords()) {
            if (record != nullptr && record->GetEntryGetter() != nullptr) {
                entryGetters.emplace(record->GetRecordId(), record->GetEntryGetter());
            }
        }
        auto ret = PasteboardClient::GetInstance()->SetPasteData(*data, nullptr, entryGetters);
        auto value = std::make_shared<int32_t>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniSetDataSync");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[SetDataSync] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return;
    }
    if (*value == static_cast<int32_t>(PasteboardError::PROHIBIT_COPY)) {
        ThrowBusinessError(env, static_cast<int32_t>(PasteboardError::PROHIBIT_COPY),
            "The system prohibits copying.");
    } else if (*value == static_cast<int32_t>(PasteboardError::TASK_PROCESSING)) {
        ThrowBusinessError(env, static_cast<int32_t>(PasteboardError::TASK_PROCESSING),
            "Another setData is being processed.");
    }
}

ani_long PasteboardAniSystemAdapter::GetChangeCount(ani_env *env)
{
    if (env == nullptr) {
        return 0;
    }
    uint32_t changeCount = 0;
    PasteboardClient::GetInstance()->GetChangeCount(changeCount);
    return static_cast<ani_long>(changeCount);
}

ani_array PasteboardAniSystemAdapter::GetMimeTypes(ani_env *env)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<std::vector<std::string>>>>(
        RECORD_SYNC_TIMEOUT);
    std::thread thread([block]() {
        auto ret = PasteboardClient::GetInstance()->GetMimeTypes();
        auto value = std::make_shared<std::vector<std::string>>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniGetMimeTypes");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetMimeTypes] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return nullptr;
    }
    ani_array array = nullptr;
    ani_size size = static_cast<ani_size>(value->size());
    if (ANI_OK != env->Array_New_String(size, &array)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetMimeTypes] Array_New_String failed.");
        return nullptr;
    }
    for (ani_size i = 0; i < size; ++i) {
        ani_string aniStr = nullptr;
        env->String_NewUTF8((*value)[i].c_str(), (*value)[i].size(), &aniStr);
        env->Array_Set_String(array, i, aniStr);
    }
    return array;
}

ani_array PasteboardAniSystemAdapter::DetectPatterns(ani_env *env, ani_array patterns)
{
    if (env == nullptr) {
        return nullptr;
    }
    ani_size size = 0;
    if (ANI_OK != env->Array_Length(patterns, &size)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[DetectPatterns] Array_Length failed.");
        return nullptr;
    }
    std::set<Pattern> patternsToCheck;
    for (ani_size i = 0; i < size; ++i) {
        ani_int patternValue;
        if (ANI_OK != env->Array_Get_Int(patterns, i, &patternValue)) {
            continue;
        }
        patternsToCheck.insert(static_cast<Pattern>(patternValue));
    }
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<std::set<Pattern>>>>(
        RECORD_SYNC_TIMEOUT);
    std::thread thread([block, patternsToCheck]() {
        auto ret = PasteboardClient::GetInstance()->DetectPatterns(patternsToCheck);
        auto value = std::make_shared<std::set<Pattern>>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniDetectPatterns");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[DetectPatterns] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return nullptr;
    }
    ani_array resultArray = nullptr;
    ani_size resultSize = static_cast<ani_size>(value->size());
    if (ANI_OK != env->Array_New_Int(resultSize, &resultArray)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[DetectPatterns] Array_New_Int failed.");
        return nullptr;
    }
    ani_size index = 0;
    for (auto &pattern : *value) {
        env->Array_Set_Int(resultArray, index, static_cast<ani_int>(pattern));
        ++index;
    }
    return resultArray;
}

void PasteboardAniSystemAdapter::SetAppShareOptions(ani_env *env, ani_int shareOption)
{
    if (env == nullptr) {
        return;
    }
    auto result = PasteboardClient::GetInstance()->SetAppShareOptions(
        static_cast<ShareOption>(shareOption));
    if (result == static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR)) {
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS),
            "Parameter error. Parameter verification failed.");
    } else if (result == static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR)) {
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::NO_PERMISSION),
            "Permission verification failed. A non-permission application calls a API.");
    } else if (result == static_cast<int32_t>(PasteboardError::INVALID_OPERATION_ERROR)) {
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::SETTINGS_ALREADY_EXIST),
            "Settings already exist.");
    }
}

void PasteboardAniSystemAdapter::RemoveAppShareOptions(ani_env *env)
{
    if (env == nullptr) {
        return;
    }
    auto result = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    if (result == static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR)) {
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::NO_PERMISSION),
            "Permission verification failed. A non-permission application calls a API.");
    }
}

ani_object PasteboardAniSystemAdapter::GetDataWithProgress(ani_env *env, ani_object params)
{
    if (env == nullptr) {
        return nullptr;
    }
    auto pasteData = std::make_shared<PasteData>();
    auto block = std::make_shared<OHOS::BlockObject<std::shared_ptr<int32_t>>>(RECORD_SYNC_TIMEOUT);
    std::thread thread([block, pasteData]() mutable {
        auto ret = PasteboardClient::GetInstance()->GetPasteData(*pasteData);
        auto value = std::make_shared<int32_t>(ret);
        block->SetValue(value);
    });
    PasteBoardCommonUtils::SetThreadTaskName(thread, "AniGetDataWithProgress");
    thread.detach();
    auto value = block->GetValue();
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_ANI, "[GetDataWithProgress] time out.");
        ThrowBusinessError(env, static_cast<int32_t>(JSErrorCode::REQUEST_TIME_OUT), "request timed out.");
        return GetNullObject(env);
    }
    return Create(env, pasteData);
}
