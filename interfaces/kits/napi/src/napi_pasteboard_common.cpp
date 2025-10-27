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
#include "napi_pasteboard_common.h"
#include "pasteboard_hilog.h"

namespace OHOS {
namespace MiscServicesNapi {
using namespace OHOS::MiscServices;
const size_t ARGC_TYPE_SET2 = 2;
constexpr size_t STR_TAIL_LENGTH = 1;
constexpr int32_t MIMETYPE_MAX_SIZE = 1024;

napi_value GetCallbackErrorValue(napi_env env, int32_t errorCode)
{
    napi_value result = nullptr;
    napi_value eCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, errorCode, &eCode));
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_set_named_property(env, result, "code", eCode));
    return result;
}

void SetCallback(const napi_env &env, const napi_ref &callbackIn, const napi_value *results)
{
    if (results == nullptr) {
        return;
    }
    napi_value callback = nullptr;
    napi_value resultOut = nullptr;
    napi_get_reference_value(env, callbackIn, &callback);
    napi_call_function(env, nullptr, callback, ARGC_TYPE_SET2, results, &resultOut);
}

napi_value NapiGetNull(napi_env env)
{
    napi_value result = nullptr;
    napi_get_null(env, &result);
    return result;
}

napi_value CreateNapiNumber(napi_env env, int32_t num)
{
    napi_value value = nullptr;
    napi_create_int32(env, num, &value);
    return value;
}

napi_value CreateNapiString(napi_env env, std::string str)
{
    napi_value value = nullptr;
    napi_create_string_utf8(env, str.c_str(), NAPI_AUTO_LENGTH, &value);
    return value;
}

/* napi_value <-> std::string */
bool GetValue(napi_env env, napi_value in, std::string &out)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, in, &type), false);
    NAPI_ASSERT_BASE(env, type == napi_string, "Wrong argument type. String expected.", false);

    size_t len = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, in, nullptr, 0, &len), false);
    if (len < 0) {
        return false;
    }

    size_t length = 0;
    out.resize(len + STR_TAIL_LENGTH, 0);
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, in, out.data(), len + STR_TAIL_LENGTH, &length), false);
    out.resize(len);

    return true;
}

/* napi_value <-> std::set<Pattern> */
bool GetValue(napi_env env, napi_value in, std::set<MiscServices::Pattern> &out)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, in, &isArray), false);
    if (!isArray) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Wrong argument type. pattern/uint32 array expected.");
        return false;
    }

    uint32_t len = 0;
    napi_status status = napi_get_array_length(env, in, &len);
    if (status != napi_ok || len == 0) {
        PASTEBOARD_HILOGE(
            PASTEBOARD_MODULE_JS_NAPI, "napi_get_array_length status = %{public}d, len = %{public}d", status, len);
        return false;
    }

    for (uint32_t i = 0; i < len; i++) {
        napi_value element;
        status = napi_get_element(env, in, i, &element);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(
                PASTEBOARD_MODULE_JS_NAPI, "napi_get_element%{public}d err status = %{public}d", i, status);
            return false;
        }
        uint32_t pattern;
        status = napi_get_value_uint32(env, element, &pattern);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_get_value_uint32 err status = %{public}d", status);
            return false;
        }
        if (pattern >= static_cast<uint32_t>(Pattern::COUNT)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Unsupported pattern value: %{public}d", pattern);
            return false;
        }
        out.insert(static_cast<Pattern>(pattern));
    }
    return true;
}

/* napi_value <-> std::set<Pattern> */
napi_status SetValue(napi_env env, std::set<Pattern> &in, napi_value &result)
{
    napi_status status = napi_create_array_with_length(env, in.size(), &result);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_create_array_with_length error status = %{public}d", status);
        return status;
    }
    int i = 0;
    for (auto &pattern : in) {
        napi_value element;
        status = napi_create_uint32(env, static_cast<uint32_t>(pattern), &element);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_create_uint32 error status = %{public}d", status);
            return status;
        }
        status = napi_set_element(env, result, i, element);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(
                PASTEBOARD_MODULE_JS_NAPI, "napi_set_element %{public}d err status = %{public}d", i, status);
            return status;
        }
        ++i;
    }
    return status;
}

bool CheckArgsType(napi_env env, napi_value in, napi_valuetype expectedType, const char *message)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, in, &type), false);
    int32_t errCode = static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS);
    if (type != expectedType) {
        napi_throw_error(env, std::to_string(errCode).c_str(), message);
        return false;
    }
    return true;
}

bool CheckExpression(napi_env env, bool flag, MiscServices::JSErrorCode errCode, const char *message)
{
    if (!flag) {
        NAPI_CALL_BASE(
            env, napi_throw_error(env, std::to_string(static_cast<int32_t>(errCode)).c_str(), message), false);
        return false;
    }
    return true;
}

// Check Parameters of CreateData, CreateRecord and AddRecord
bool CheckArgs(napi_env env, napi_value *argv, size_t argc, std::string &mimeType)
{
    // 2: CreateRecord, CreateRecord and AddRecord has 2 args.
    if (!CheckExpression(env, argc >= ARGC_TYPE_SET2, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The number of arguments cannot be less than two.")) {
        return false;
    }

    bool ret = CheckArgsMimeType(env, argv[0], mimeType);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "GetValue failed");
        return false;
    }
    if (!CheckExpression(
        env, mimeType != "", JSErrorCode::INVALID_PARAMETERS, "Parameter error. mimeType cannot be empty.") ||
        !CheckExpression(env, mimeType.size() <= MIMETYPE_MAX_SIZE, JSErrorCode::INVALID_PARAMETERS,
            "Parameter error. The length of mimeType cannot be greater than 1024 bytes.")) {
        return false;
    }

    if (mimeType == MIMETYPE_TEXT_URI || mimeType == MIMETYPE_TEXT_PLAIN || mimeType == MIMETYPE_TEXT_HTML) {
        if (!CheckArgsType(env, argv[1], napi_string, "Parameter error. The type of value must be string.")) {
            return false;
        }
    } else if (mimeType == MIMETYPE_PIXELMAP) {
        if (!CheckExpression(env, Media::PixelMapNapi::GetPixelMap(env, argv[1]) != nullptr,
                JSErrorCode::INVALID_PARAMETERS, "Parameter error. Actual mimeType is not mimetype_pixelmap.")) {
            return false;
        }
    } else if (mimeType == MIMETYPE_TEXT_WANT) {
        AAFwk::Want want;
        ret = OHOS::AppExecFwk::UnwrapWant(env, argv[1], want);
        if (!CheckExpression(env, ret, JSErrorCode::INVALID_PARAMETERS,
            "Parameter error. Actual mimeType is not mimetype_text_want.")) {
            return false;
        }
    } else {
        bool isArrayBuffer = false;
        NAPI_CALL_BASE(env, napi_is_arraybuffer(env, argv[1], &isArrayBuffer), false);
        if (!CheckExpression(env, isArrayBuffer, JSErrorCode::INVALID_PARAMETERS,
            "Parameter error. The mimeType is not an arraybuffer.")) {
            return false;
        }
    }
    return true;
}

bool CheckArgsMimeType(napi_env env, napi_value in, std::string &mimeType)
{
    bool ret = CheckArgsType(env, in, napi_string, "Parameter error. The type of mimeType must be string.");
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Wrong argument type. String expected.");
        return false;
    }
    ret = GetValue(env, in, mimeType);
    if (!ret) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "GetValue failed");
        return false;
    }
    if (!CheckExpression(
        env, mimeType != "", JSErrorCode::INVALID_PARAMETERS, "Parameter error. mimeType cannot be empty.") ||
        !CheckExpression(env, mimeType.size() <= MIMETYPE_MAX_SIZE, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. The length of mimeType cannot be greater than 1024 bytes.")) {
        return false;
    }
    return true;
}

bool CheckArgsArray(napi_env env, napi_value in, std::vector<std::string> &mimeTypes)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, in, &type), false);
    int32_t errCode = static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS);
    if (type != napi_object) {
        napi_throw_error(env, std::to_string(errCode).c_str(), "Wrong argument type. Object expected.");
        return false;
    }

    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, in, &isArray), false);
    if (!isArray) {
        return false;
    }

    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, in, &length), false);
    napi_value element;
    for (uint32_t i = 0; i < length; i++) {
        NAPI_CALL_BASE(env, napi_get_element(env, in, i, &element), false);
        std::string mimeType;
        if (!GetValue(env, element, mimeType)) {
            return false;
        }
        mimeTypes.emplace_back(mimeType);
    }

    return true;
}

bool CheckArgsFunc(napi_env env, napi_value in, napi_ref &provider)
{
    napi_valuetype type = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, in, &type), false);
    int32_t errCode = static_cast<int32_t>(JSErrorCode::INVALID_PARAMETERS);
    if (type != napi_function) {
        napi_throw_error(env, std::to_string(errCode).c_str(), "Wrong argument type. function expected.");
        return false;
    }

    NAPI_CALL_BASE(env, napi_create_reference(env, in, 1, &provider), false);

    return true;
}

bool CheckArgsVector(napi_env env, napi_value in,
    std::shared_ptr<std::vector<std::pair<std::string, std::shared_ptr<EntryValue>>>> result)
{
    napi_valuetype valueType = napi_undefined;
    NAPI_CALL_BASE(env, napi_typeof(env, in, &valueType), false);
    if (!CheckExpression(env, valueType == napi_object, JSErrorCode::INVALID_PARAMETERS,
        "Parameter error. When there is only one parameter, it must be a Record.")) {
        return false;
    }
    
    napi_value typeValueMap = nullptr;
    NAPI_CALL_BASE(env, napi_get_property_names(env, in, &typeValueMap), false);
    uint32_t length = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, typeValueMap, &length), false);

    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "length = %{public}u", length);
    for (uint32_t i = 0; i < length; i++) {
        napi_value mimeTypeNapi = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, typeValueMap, i, &mimeTypeNapi), false);
        std::string mimeType;
        bool ret = CheckArgsMimeType(env, mimeTypeNapi, mimeType);
        if (!ret) {
            return false;
        }
        napi_value value = nullptr;
        std::shared_ptr<EntryValue> entryValue = std::make_shared<EntryValue>();
        NAPI_CALL_BASE(env, napi_get_property(env, in, mimeTypeNapi, &value), false);
        if (!GetNativeValue(env, mimeType, value, *entryValue)) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "GetNativeValue failed");
            return false;
        }
        result->emplace_back(std::make_pair(mimeType, entryValue));
    }
    return true;
}

bool CheckRetCode(napi_env env, int32_t retCode, const std::vector<JSErrorCode> &focusErrCodes)
{
    auto errInfo = NapiDataUtils::GetErrInfo(static_cast<PasteboardError>(retCode));
    auto iter = std::find(focusErrCodes.begin(), focusErrCodes.end(), errInfo.first);
    if (iter != focusErrCodes.end()) {
        NAPI_CALL_BASE(env,
            napi_throw_error(env, std::to_string(static_cast<int32_t>(errInfo.first)).c_str(), errInfo.second.c_str()),
            false);
        return false;
    }
    return true;
}

bool GetContextSetErr(const std::shared_ptr<GetContextInfo> context, int32_t retCode,
    const std::vector<JSErrorCode> &focusErrCodes, std::string defaultMsg)
{
    auto errInfo = NapiDataUtils::GetErrInfo(static_cast<PasteboardError>(retCode));
    auto iter = std::find(focusErrCodes.begin(), focusErrCodes.end(), errInfo.first);
    if (iter != focusErrCodes.end()) {
        errInfo.second = defaultMsg.empty() ? errInfo.second : defaultMsg;
        context->SetErrInfo(static_cast<int32_t>(errInfo.first), errInfo.second);
        context->status = napi_generic_failure;
        return true;
    }
    return false;
}

bool UnifiedContextSetErr(const std::shared_ptr<GetUnifiedContextInfo> context, int32_t retCode,
    const std::vector<JSErrorCode> &focusErrCodes, std::string defaultMsg)
{
    auto errInfo = NapiDataUtils::GetErrInfo(static_cast<PasteboardError>(retCode));
    auto iter = std::find(focusErrCodes.begin(), focusErrCodes.end(), errInfo.first);
    if (iter != focusErrCodes.end()) {
        errInfo.second = defaultMsg.empty() ? errInfo.second : defaultMsg;
        context->SetErrInfo(static_cast<int32_t>(errInfo.first), errInfo.second);
        context->status = napi_generic_failure;
        return true;
    }
    return false;
}

napi_status ConvertEntryValue(napi_env env, napi_value *result, std::string &mimeType,
    std::shared_ptr<PasteDataEntry> value)
{
    if (value == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "failed to find dataEntry, type=%{public}s", mimeType.c_str());
        return napi_generic_failure;
    }
    if (mimeType == MIMETYPE_TEXT_URI) {
        std::shared_ptr<Uri> uri = value->ConvertToUri();
        if (uri == nullptr) {
            return napi_generic_failure;
        }
        std::string str = uri->ToString();
        return napi_create_string_utf8(env, str.c_str(), str.size(), result);
    } else if (mimeType == MIMETYPE_TEXT_PLAIN) {
        std::shared_ptr<std::string> str = value->ConvertToPlainText();
        if (str == nullptr) {
            return napi_generic_failure;
        }
        return napi_create_string_utf8(env, str->c_str(), str->size(), result);
    } else if (mimeType == MIMETYPE_TEXT_HTML) {
        std::shared_ptr<std::string> str = value->ConvertToHtml();
        if (str == nullptr) {
            return napi_generic_failure;
        }
        return napi_create_string_utf8(env, str->c_str(), str->size(), result);
    } else if (mimeType == MIMETYPE_PIXELMAP) {
        std::shared_ptr<Media::PixelMap> pixelMap = value->ConvertToPixelMap();
        if (!CheckExpression(env, pixelMap != nullptr,
            JSErrorCode::INVALID_PARAMETERS, "Parameter error. pixelMap get failed")) {
            return napi_generic_failure;
        }
        *result = Media::PixelMapNapi::CreatePixelMap(env, pixelMap);
        return napi_ok;
    } else if (mimeType == MIMETYPE_TEXT_WANT) {
        std::shared_ptr<AAFwk::Want> want = value->ConvertToWant();
        if (!CheckExpression(env, want != nullptr,
            JSErrorCode::INVALID_PARAMETERS, "Parameter error. want get failed")) {
            return napi_generic_failure;
        }
        *result = AppExecFwk::WrapWant(env, *want);
        return napi_ok;
    } else {
        std::shared_ptr<MineCustomData> customData = value->ConvertToCustomData();
        if (customData == nullptr) {
            return napi_generic_failure;
        }
        auto itemData = customData->GetItemData();
        auto item = itemData.find(mimeType);
        if (item == itemData.end()) {
            return napi_generic_failure;
        }
        std::vector<uint8_t> dataArray = item->second;
        void *data = nullptr;
        size_t len = dataArray.size();
        NAPI_CALL_BASE(env, napi_create_arraybuffer(env, len, &data, result), napi_generic_failure);
        if (memcpy_s(data, len, reinterpret_cast<const void *>(dataArray.data()), len) != 0) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "memcpy customData %{public}s failed, size=%{public}zu",
                mimeType.c_str(), len);
            return napi_generic_failure;
        }
        return napi_ok;
    }
}

bool GetNativeValue(napi_env env, const std::string &type, napi_value valueNapi, EntryValue &value)
{
    bool isArrayBuffer = false;
    NAPI_CALL_BASE(env, napi_is_arraybuffer(env, valueNapi, &isArrayBuffer), false);
    if (isArrayBuffer) {
        void *data = nullptr;
        size_t dataLen = 0;
        NAPI_CALL_BASE(env, napi_get_arraybuffer_info(env, valueNapi, &data, &dataLen), false);
        value = std::vector<uint8_t>(reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + dataLen);
        return true;
    }

    napi_status status;
    napi_valuetype valueType = napi_undefined;
    status = napi_typeof(env, valueNapi, &valueType);
    NAPI_ASSERT_BASE(env, status == napi_ok,
        "Parameter error: parameter value type must be ValueType", false);
    if (valueType == napi_object) {
        if (type == MIMETYPE_PIXELMAP) {
            value = std::shared_ptr<OHOS::Media::PixelMap>(nullptr);
        } else if (type == MIMETYPE_TEXT_WANT) {
            value = std::shared_ptr<OHOS::AAFwk::Want>(nullptr);
        } else {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "Parameter error: error ValueType");
            value = nullptr;
        }
    } else if (valueType == napi_string) {
        value = std::string();
    } else if (valueType == napi_number) {
        value = double();
    } else if (valueType == napi_boolean) {
        value = bool();
    } else if (valueType == napi_undefined) {
        value = std::monostate();
    } else if (valueType == napi_null) {
        value = nullptr;
    }
    std::visit([&](auto &value) { status = NapiDataUtils::GetValue(env, valueNapi, value); }, value);
    NAPI_ASSERT_BASE(env, status == napi_ok, "get unifiedRecord failed", false);
    return true;
}
} // namespace MiscServicesNapi
} // namespace OHOS