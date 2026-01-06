/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
using namespace OHOS::MiscServices;
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServicesNapi {
constexpr size_t MAX_ARGS = 6;
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

napi_value PasteboardNapi::JSCreateRecord(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "JSCreateRecord is called!");
    size_t argc = MAX_ARGS;
    napi_value argv[MAX_ARGS] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type));
    if (!CheckExpression(env, argc >= ARGC_TYPE_SET2, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments cannot be less than two.") ||
        !CheckExpression(env, type == napi_object || type == napi_string, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. the first param should be object or string.")) {
        return nullptr;
    }
    if (type == napi_string) {
        std::string mimeType;
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
    if (!CheckArgs(env, argv, argc, mimeType)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "CheckArgs failed!");
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