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

#include "entry_getter.h"
#include "pasteboard_hilog.h"
#include "pastedata_record_napi.h"

using namespace OHOS::MiscServices;
using namespace OHOS::Media;

namespace OHOS {
namespace MiscServicesNapi {
static thread_local napi_ref g_pasteDataRecord = nullptr;
const int ARGC_TYPE_SET0 = 0;
const int ARGC_TYPE_SET1 = 1;
const int ARGC_TYPE_SET2 = 2;
constexpr int32_t MIMETYPE_MAX_SIZE = 1024;
constexpr size_t ENTRY_GETTER_TIMEOUT = 2;

PasteDataRecordNapi::PasteDataRecordNapi() : env_(nullptr) {}

PasteDataRecordNapi::~PasteDataRecordNapi() {}

bool PasteDataRecordNapi::NewInstanceByRecord(
    napi_env env, napi_value &instance, const std::shared_ptr<MiscServices::PasteDataRecord> &record)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(record != nullptr, false, PASTEBOARD_MODULE_CLIENT,
        "invalid parameter record");
    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = record;
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewHtmlTextRecordInstance(napi_env env, const std::string &text, napi_value &instance)
{
    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateHtmlTextRecord(text);
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewPlainTextRecordInstance(napi_env env, const std::string &text, napi_value &instance)
{
    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreatePlainTextRecord(text);
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewPixelMapRecordInstance(
    napi_env env, const std::shared_ptr<PixelMap> pixelMap, napi_value &instance)
{
    if (pixelMap == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "pixelMap is nullptr.");
        return false;
    }

    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed.");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMap);
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewUriRecordInstance(napi_env env, const std::string &text, napi_value &instance)
{
    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateUriRecord(OHOS::Uri(text));
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewWantRecordInstance(
    napi_env env, const std::shared_ptr<OHOS::AAFwk::Want> want, napi_value &instance)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(want != nullptr, false,
        PASTEBOARD_MODULE_CLIENT, "invalid parameter want");
    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateWantRecord(want);
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewKvRecordInstance(
    napi_env env, const std::string &mimeType, const std::vector<uint8_t> &arrayBuffer, napi_value &instance)
{
    NAPI_CALL_BASE(env, PasteDataRecordNapi::NewInstance(env, instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    obj->JSFillInstance(env, instance);
    return true;
}

bool PasteDataRecordNapi::NewEntryGetterRecordInstance(const std::vector<std::string> &mimeTypes,
    std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetter, napi_value &instance)
{
    if (entryGetter == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "no entry getter");
        return false;
    }
    NAPI_CALL_BASE(entryGetter->GetEnv(), PasteDataRecordNapi::NewInstance(entryGetter->GetEnv(), instance), false);
    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(entryGetter->GetEnv(), instance, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
        return false;
    }
    obj->value_ = PasteboardClient::GetInstance()->CreateMultiDelayRecord(mimeTypes, entryGetter->GetStub());
    obj->JSFillInstance(entryGetter->GetEnv(), instance);
    return true;
}

void PasteDataRecordNapi::SetNamedPropertyByStr(
    napi_env env, napi_value &instance, const char *propName, const char *propValue)
{
    if (propName == nullptr || propValue == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "invalid parameter");
        return;
    }
    napi_value prop = nullptr;
    if (napi_create_string_utf8(env, propValue, NAPI_AUTO_LENGTH, &prop) == napi_ok) {
        napi_set_named_property(env, instance, propName, prop);
    }
}

napi_value PasteDataRecordNapi::SetNapiKvData(napi_env env, std::shared_ptr<MineCustomData> customData)
{
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(customData != nullptr, nullptr,
        PASTEBOARD_MODULE_CLIENT, "invalid parameter");
    napi_value jsCustomData = nullptr;
    napi_create_object(env, &jsCustomData);
    auto itemData = customData->GetItemData();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "size = %{public}zu.", itemData.size());
    for (const auto &item : itemData) {
        void *data = nullptr;
        napi_value arrayBuffer = nullptr;
        size_t len = item.second.size();
        NAPI_CALL(env, napi_create_arraybuffer(env, len, &data, &arrayBuffer));
        errno_t ret = (len == 0) ? EOK : memcpy_s(data, len, reinterpret_cast<const void *>(item.second.data()), len);
        if (ret != EOK) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "memcpy customData %{public}s failed, size=%{public}zu",
                item.first.c_str(), len);
        }
        NAPI_CALL(env, napi_set_named_property(env, jsCustomData, item.first.c_str(), arrayBuffer));
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "mimeType = %{public}s.", item.first.c_str());
    }
    return jsCustomData;
}

std::shared_ptr<MineCustomData> PasteDataRecordNapi::GetNativeKvData(napi_env env, napi_value napiValue)
{
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, napiValue, &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "Wrong argument type. Object expected.");

    napi_value mimeTypes = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, napiValue, &mimeTypes));
    uint32_t mimeTypesNum = 0;
    NAPI_CALL(env, napi_get_array_length(env, mimeTypes, &mimeTypesNum));

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "mimeTypesNum = %{public}u", mimeTypesNum);
    std::shared_ptr<MineCustomData> customData = std::make_shared<MineCustomData>();
    for (uint32_t i = 0; i < mimeTypesNum; i++) {
        napi_value mimeTypeNapi = nullptr;
        NAPI_CALL(env, napi_get_element(env, mimeTypes, i, &mimeTypeNapi));

        std::string mimeType;
        bool ret = GetValue(env, mimeTypeNapi, mimeType);
        if (!ret || (mimeType.size() > MIMETYPE_MAX_SIZE) || mimeType == "") {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "mimeType size or mimeType invalid");
            return nullptr;
        }
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_CLIENT, "mimeType = %{public}s,", mimeType.c_str());

        napi_value napiArrayBuffer = nullptr;
        NAPI_CALL(env, napi_get_property(env, napiValue, mimeTypeNapi, &napiArrayBuffer));
        void *data = nullptr;
        size_t dataLen;
        NAPI_CALL(env, napi_get_arraybuffer_info(env, napiArrayBuffer, &data, &dataLen));
        customData->AddItemData(mimeType,
            std::vector<uint8_t>(reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + dataLen));
    }
    return customData;
}

void PasteDataRecordNapi::JSFillInstance(napi_env env, napi_value &instance)
{
    if (value_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "invalid parameter");
        return;
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "mimeType = %{public}s.", value_->GetMimeType().c_str());

    auto mimeType = value_->GetMimeType();
    SetNamedPropertyByStr(env, instance, "mimeType", mimeType.c_str());

    auto plainText = value_->GetPlainTextV0();
    if (plainText != nullptr) {
        SetNamedPropertyByStr(env, instance, "plainText", plainText->c_str());
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "fill plainText.");
    }

    auto htmlText = value_->GetHtmlTextV0();
    if (htmlText != nullptr) {
        SetNamedPropertyByStr(env, instance, "htmlText", htmlText->c_str());
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "fill htmlText.");
    }

    auto uri = value_->GetUriV0();
    if (uri != nullptr) {
        SetNamedPropertyByStr(env, instance, "uri", uri->ToString().c_str());
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "fill uri.");
    }

    auto want = value_->GetWant();
    if (want != nullptr) {
        napi_value jsWant = OHOS::AppExecFwk::WrapWant(env, *want);
        napi_set_named_property(env, instance, "want", jsWant);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "fill want.");
    }

    auto pixelMap = value_->GetPixelMapV0();
    if (pixelMap != nullptr) {
        napi_value jsPixelMap = OHOS::Media::PixelMapNapi::CreatePixelMap(env, pixelMap);
        napi_set_named_property(env, instance, "pixelMap", jsPixelMap);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "fill pixelMap.");
    }

    auto customData = value_->GetCustomData();
    if (customData != nullptr) {
        napi_value jsCustomData = PasteDataRecordNapi::SetNapiKvData(env, customData);
        napi_set_named_property(env, instance, "data", jsCustomData);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "fill data.");
    }
}

napi_value PasteDataRecordNapi::ConvertToText(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "ConvertToText is called!");

    struct ExeContext {
        std::string text;
        PasteDataRecordNapi *obj = nullptr;
    };
    auto exeContext = std::make_shared<ExeContext>();
    auto input = [exeContext](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        // convertToText(callback: AsyncCallback<string>) has 1 args
        if (argc > 0 &&
            !CheckArgsType(env, argv[0], napi_function, "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        PasteDataRecordNapi *obj = nullptr;
        napi_status status = napi_unwrap(env, self, reinterpret_cast<void **>(&obj));
        if (status == napi_ok && obj != nullptr) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
            exeContext->obj = obj;
        }
        return napi_ok;
    };

    auto exec = [exeContext](AsyncCall::Context *ctx) {
        if ((exeContext->obj != nullptr) && (exeContext->obj->value_ != nullptr)) {
            exeContext->text = exeContext->obj->value_->ConvertToText();
        }
    };
    auto output = [exeContext](napi_env env, napi_value *result) -> napi_status {
        napi_status status =
            napi_create_string_utf8(env, (exeContext->text).c_str(), (exeContext->text).length(), result);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_create_string_utf8 status = %{public}d", status);
        return status;
    };

    // 0: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, std::make_shared<AsyncCall::Context>(std::move(input), std::move(output)), 0);
    return asyncCall.Call(env, exec);
}

napi_value PasteDataRecordNapi::ConvertToTextV9(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "ConvertToTextV9 is called!");
    return ConvertToText(env, info);
}

napi_value PasteDataRecordNapi::ToPlainText(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_JS_NAPI, "ToPlainText is called!");
    size_t argc = ARGC_TYPE_SET1;
    napi_value argv[ARGC_TYPE_SET1] = {0};
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_ASSERT(env, argc >= ARGC_TYPE_SET0, "Wrong number of arguments");

    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr) || (obj->value_ == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Get ToPlainText object failed");
        return nullptr;
    }

    std::string str = obj->value_->ConvertToText();
    napi_value result = nullptr;
    napi_create_string_utf8(env, str.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value PasteDataRecordNapi::AddEntry(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "AddEntry is called!");
    size_t argc = ARGC_TYPE_SET2;
    napi_value argv[ARGC_TYPE_SET2] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    std::string mimeType;
    if (!CheckArgs(env, argv, argc, mimeType)) {
        return nullptr;
    }

    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr) || (obj->value_ == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Get AddEntry object failed");
        return nullptr;
    }
    
    EntryValue entryValue;
    if (!GetNativeValue(env, mimeType, argv[1], entryValue)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "GetNativeValue failed");
        return nullptr;
    }

    auto utdType = CommonUtils::Convert2UtdId(UDMF::UD_BUTT, mimeType);
    std::shared_ptr<PasteDataEntry> pasteDataEntry = std::make_shared<PasteDataEntry>(utdType, entryValue);
    obj->value_->AddEntry(utdType, pasteDataEntry);
    return nullptr;
}

napi_value PasteDataRecordNapi::GetValidTypes(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetValidType is called!");
    size_t argc = ARGC_TYPE_SET1;
    napi_value argv[ARGC_TYPE_SET1] = { 0 };
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    std::vector<std::string> mimeTypes;
    
    if (!CheckExpression(env, argc >= ARGC_TYPE_SET1, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments cannot be less than one.") ||
        !CheckArgsArray(env, argv[0], mimeTypes)) {
        return nullptr;
    }

    PasteDataRecordNapi *obj = nullptr;
    napi_status status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&obj));
    if ((status != napi_ok) || (obj == nullptr) || (obj->value_ == nullptr)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Get GetValidType object failed");
        return nullptr;
    }
    std::vector<std::string> validTypes = obj->value_->GetValidMimeTypes(mimeTypes);
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_array(env, &result));
    for (uint32_t i = 0; i < validTypes.size(); i++) {
        napi_value element;
        NAPI_CALL(env, napi_create_string_utf8(env, validTypes[i].c_str(), NAPI_AUTO_LENGTH, &element));
        NAPI_CALL(env, napi_set_element(env, result, i, element));
    }
    return result;
}

napi_value PasteDataRecordNapi::GetRecordData(napi_env env, napi_callback_info info)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetValidType is called!");

    struct ExeContext {
        std::string mimeType;
        std::shared_ptr<PasteDataEntry> entryValue;
        PasteDataRecordNapi *obj = nullptr;
    };
    auto exeContext = std::make_shared<ExeContext>();
    auto input = [exeContext](napi_env env, size_t argc, napi_value *argv, napi_value self) -> napi_status {
        if (!CheckExpression(env, argc >= ARGC_TYPE_SET1, JSErrorCode::INVALID_PARAMETERS,
            "Parameter error. The number of arguments cannot be less than one.") ||
            !CheckArgsMimeType(env, argv[0], exeContext->mimeType)) {
            return napi_invalid_arg;
        }
        if (argc > ARGC_TYPE_SET1 &&
            !CheckArgsType(env, argv[ARGC_TYPE_SET1], napi_function,
            "Parameter error. The type of callback must be function.")) {
            return napi_invalid_arg;
        }
        napi_status status = napi_unwrap(env, self, reinterpret_cast<void **>(&(exeContext->obj)));
        if ((status != napi_ok) || (exeContext->obj == nullptr)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "unwrap failed");
            return napi_object_expected;
        }
        return napi_ok;
    };

    auto exec = [exeContext](AsyncCall::Context *ctx) {
        if ((exeContext->obj != nullptr) && (exeContext->obj->value_ != nullptr)) {
            exeContext->entryValue = exeContext->obj->value_->GetEntryByMimeType(exeContext->mimeType);
        }
    };
    auto output = [exeContext](napi_env env, napi_value *result) -> napi_status {
        napi_status status = ConvertEntryValue(env, result, exeContext->mimeType, exeContext->entryValue);
        return status;
    };

    // 1: the AsyncCall at the first position;
    AsyncCall asyncCall(env, info, std::make_shared<AsyncCall::Context>(std::move(input),
        std::move(output)), ARGC_TYPE_SET1);
    return asyncCall.Call(env, exec);
}

napi_value PasteDataRecordNapi::PasteDataRecordInit(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_WRITABLE_FUNCTION("convertToText", ConvertToText),
        DECLARE_NAPI_WRITABLE_FUNCTION("convertToTextV9", ConvertToTextV9),
        DECLARE_NAPI_WRITABLE_FUNCTION("toPlainText", ToPlainText),
        DECLARE_NAPI_WRITABLE_FUNCTION("addEntry", AddEntry),
        DECLARE_NAPI_WRITABLE_FUNCTION("getValidTypes", GetValidTypes),
        DECLARE_NAPI_WRITABLE_FUNCTION("getData", GetRecordData)
    };

    napi_status status = napi_ok;

    napi_value constructor;
    status = napi_define_class(env, "PasteDataRecord", NAPI_AUTO_LENGTH, New, nullptr,
        sizeof(properties) / sizeof(napi_property_descriptor), properties, &constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Failed to define class at PasteDataRecordInit");
        return nullptr;
    }

    status = napi_create_reference(env, constructor, 1, &g_pasteDataRecord);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "PasteDataRecordInit create reference failed");
        return nullptr;
    }
    napi_set_named_property(env, exports, "PasteDataRecord", constructor);
    return exports;
}

void PasteDataRecordNapi::Destructor(napi_env env, void *nativeObject, void *finalize_hint)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "Destructor");
    PasteDataRecordNapi *obj = static_cast<PasteDataRecordNapi *>(nativeObject);
    delete obj;
}

napi_value PasteDataRecordNapi::New(napi_env env, napi_callback_info info)
{
    size_t argc = ARGC_TYPE_SET1;
    napi_value argv[ARGC_TYPE_SET1] = {0};
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);

    // get native object
    PasteDataRecordNapi *obj = new PasteDataRecordNapi();
    obj->env_ = env;
    ASSERT_CALL(env, napi_wrap(env, thisVar, obj, PasteDataRecordNapi::Destructor, nullptr, nullptr), obj);
    return thisVar;
}

napi_status PasteDataRecordNapi::NewInstance(napi_env env, napi_value &instance)
{
    napi_status status = napi_invalid_arg;

    napi_value constructor;
    status = napi_get_reference_value(env, g_pasteDataRecord, &constructor);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "get reference failed");
        return status;
    }

    status = napi_new_instance(env, constructor, 0, nullptr, &instance);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "new instance failed");
        return status;
    }

    return napi_ok;
}

PastedataRecordEntryGetterInstance::PastedataRecordEntryGetterInstance(const napi_env &env, const napi_ref &ref)
    : env_(env), ref_(ref)
{
    stub_ = std::make_shared<PastedataRecordEntryGetterInstance::PastedataRecordEntryGetterImpl>();
}

PastedataRecordEntryGetterInstance::~PastedataRecordEntryGetterInstance()
{
    napi_delete_reference(env_, ref_);
}

void UvWorkGetRecordByEntryGetter(uv_work_t *work, int status)
{
    if (UV_ECANCELED == status || work == nullptr || work->data == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "invalid parameter");
        return;
    }
    PasteboardEntryGetterWorker *entryGetterWork = reinterpret_cast<PasteboardEntryGetterWorker *>(work->data);
    if (entryGetterWork == nullptr || entryGetterWork->entryGetter == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "pasteboardDataWorker or delayGetter is null");
        delete work;
        work = nullptr;
        return;
    }
    auto env = entryGetterWork->entryGetter->GetEnv();
    auto ref = entryGetterWork->entryGetter->GetRef();
    napi_handle_scope scope = nullptr;
    napi_status napiStatus = napi_open_handle_scope(env, &scope);
    if (napiStatus != napi_ok || scope == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "open handle scope failed, status = %{public}d", napiStatus);
        return;
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    auto mimeType = CommonUtils::Convert2MimeType(entryGetterWork->utdId);
    napi_value argv[1] = { CreateNapiString(env, mimeType) };
    napi_value callback = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(env, ref, &callback);
    {
        std::unique_lock<std::mutex> lock(entryGetterWork->mutex);
        auto ret = napi_call_function(env, undefined, callback, 1, argv, &resultOut);
        if (ret == napi_ok) {
            EntryValue entryValue;
            if (GetNativeValue(env, mimeType, resultOut, entryValue)) {
                entryGetterWork->entryValue = std::make_shared<UDMF::ValueType>(entryValue);
            }
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "call napi_call_function is not ok, ret: %{public}d", ret);
        }
        napi_close_handle_scope(env, scope);
        entryGetterWork->complete = true;
        if (!entryGetterWork->clean) {
            entryGetterWork->cv.notify_all();
            return;
        }
    }
    delete entryGetterWork;
    entryGetterWork = nullptr;
    delete work;
    work = nullptr;
}

UDMF::ValueType PastedataRecordEntryGetterInstance::GetValueByType(const std::string &utdId)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(loop != nullptr, std::monostate{},
        PASTEBOARD_MODULE_JS_NAPI, "loop instance is nullptr");
    uv_work_t *work = new (std::nothrow) uv_work_t;
    PASTEBOARD_CHECK_AND_RETURN_RET_LOGE(work != nullptr, std::monostate{},
        PASTEBOARD_MODULE_JS_NAPI, "work is null");
    PasteboardEntryGetterWorker *entryGetterWork = new (std::nothrow) PasteboardEntryGetterWorker();
    if (entryGetterWork == nullptr) {
        PASTEBOARD_HILOGW(PASTEBOARD_MODULE_JS_NAPI, "PasteboardEntryGetterWorker is null");
        delete work;
        work = nullptr;
        return std::monostate{};
    }
    entryGetterWork->entryGetter = shared_from_this();
    entryGetterWork->utdId = utdId;
    work->data = reinterpret_cast<void *>(entryGetterWork);
    bool noNeedClean = false;
    {
        std::unique_lock<std::mutex> lock(entryGetterWork->mutex);
        int ret = uv_queue_work(loop, work, [](uv_work_t *work) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "GetValueByType callback");
        }, UvWorkGetRecordByEntryGetter);
        if (ret != 0) {
            delete entryGetterWork;
            entryGetterWork = nullptr;
            delete work;
            work = nullptr;
            return std::monostate{};
        }
        if (entryGetterWork->cv.wait_for(lock, std::chrono::seconds(ENTRY_GETTER_TIMEOUT),
            [entryGetterWork] { return entryGetterWork->complete; }) && entryGetterWork->entryValue != nullptr) {
            return *(entryGetterWork->entryValue);
        }
        if (!entryGetterWork->complete && uv_cancel((uv_req_t*)work) != 0) {
            entryGetterWork->clean = true;
            noNeedClean = true;
        }
    }
    if (!noNeedClean) {
        delete entryGetterWork;
        entryGetterWork = nullptr;
        delete work;
        work = nullptr;
    }
    return std::monostate{};
}

UDMF::ValueType PastedataRecordEntryGetterInstance::PastedataRecordEntryGetterImpl::GetValueByType(
    const std::string &utdId)
{
    if (wrapper_ == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "PastedataRecordEntryGetterImpl::GetValueByType no entry getter");
        return std::monostate{};
    }
    return wrapper_->GetValueByType(utdId);
}

void PastedataRecordEntryGetterInstance::PastedataRecordEntryGetterImpl::SetEntryGetterWrapper(
    const std::shared_ptr<PastedataRecordEntryGetterInstance> entryGetterInstance)
{
    wrapper_ = entryGetterInstance;
}

napi_value PasteDataRecordNapi::CreateInstance(napi_env env, std::shared_ptr<MiscServices::PasteDataRecord> record)
{
    napi_value instance = nullptr;
    napi_status status = NewInstance(env, instance);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "new instance failed");
        return instance;
    }
    PasteDataRecordNapi *obj = new (std::nothrow) PasteDataRecordNapi();
    if (obj == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "create PasteDataRecordNapi failed");
        return instance;
    }
    obj->value_ = record;
    status = napi_wrap(
        env,
        instance,
        obj,
        PasteDataRecordNapi::Destructor,
        nullptr,
        nullptr);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_wrap failed, status=%{public}d",
            static_cast<int32_t>(status));
        delete obj;
    }
    return instance;
}

extern "C" {
napi_value GetEtsPasteDataRecord(napi_env env, std::shared_ptr<MiscServices::PasteDataRecord> record)
{
    return PasteDataRecordNapi::CreateInstance(env, record);
}
}
} // namespace MiscServicesNapi
} // namespace OHOS
