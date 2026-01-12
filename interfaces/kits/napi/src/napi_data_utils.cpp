/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "napi_data_utils.h"
#include "pasteboard_hilog.h"
#include "napi_pasteboard_assert.h"

using namespace OHOS::MiscServices;
namespace OHOS {
namespace MiscServicesNapi {
constexpr int32_t STR_MAX_LENGTH = 4096;
constexpr size_t STR_TAIL_LENGTH = 1;

const std::map<PasteboardError, std::pair<JSErrorCode, std::string>> errInfoMap = {
    {PasteboardError::PERMISSION_VERIFICATION_ERROR,
        {JSErrorCode::NO_PERMISSION, "Permission verification failed. A non-permission application calls a API."}},
    {PasteboardError::TASK_PROCESSING,
        {JSErrorCode::OTHER_COPY_OR_PASTE_IN_PROCESSING, "Another calling is being processed."}}
};

std::pair<JSErrorCode, std::string> NapiDataUtils::GetErrInfo(PasteboardError retCode)
{
    auto iter = errInfoMap.find(retCode);
    if (iter != errInfoMap.end()) {
        return iter->second;
    }
    return {static_cast<JSErrorCode>(-1), "Unknown error."};
}

/* napi_value <-> bool */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, bool &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- bool");
    return napi_get_value_bool(env, in, &out);
}

napi_status NapiDataUtils::SetValue(napi_env env, const bool &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> bool");
    return napi_get_boolean(env, in, &out);
}

/* napi_value <-> int32_t */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, int32_t &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> int32_t");
    return napi_get_value_int32(env, in, &out);
}

napi_status NapiDataUtils::SetValue(napi_env env, const int32_t &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- int32_t");
    return napi_create_int32(env, in, &out);
}

/* napi_value <-> int64_t */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, int64_t &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> int64_t");
    return napi_get_value_int64(env, in, &out);
}

napi_status NapiDataUtils::SetValue(napi_env env, const int64_t &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- int64_t");
    return napi_create_int64(env, in, &out);
}

/* napi_value <-> float */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, float &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> float");
    double tmp = 0;
    napi_status status = napi_get_value_double(env, in, &tmp);
    out = tmp;
    return status;
}

napi_status NapiDataUtils::SetValue(napi_env env, const float &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- float");
    double tmp = in;
    return napi_create_double(env, tmp, &out);
}

/* napi_value <-> double */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, double &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> double");
    return napi_get_value_double(env, in, &out);
}

napi_status NapiDataUtils::SetValue(napi_env env, const double &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- double");
    return napi_create_double(env, in, &out);
}

/* napi_value <-> std::string */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::string &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- string");
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, in, &type);
    if (!((status == napi_ok) && (type == napi_string))) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_typeof failed, type=%{public}d status=%{public}d",
            static_cast<int32_t>(type), status);
        return napi_invalid_arg;
    }

    size_t maxLen = STR_MAX_LENGTH;
    status = napi_get_value_string_utf8(env, in, NULL, 0, &maxLen);
    if (maxLen == 0) {
        return status;
    }
    char *buf = new (std::nothrow) char[maxLen + STR_TAIL_LENGTH];
    if (buf != nullptr) {
        size_t len = 0;
        status = napi_get_value_string_utf8(env, in, buf, maxLen + STR_TAIL_LENGTH, &len);
        if (status == napi_ok) {
            buf[len] = 0;
            out = std::string(buf);
        }
        delete[] buf;
    } else {
        status = napi_generic_failure;
    }
    return status;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::string &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::string %{public}d", (int)in.length());
    return napi_create_string_utf8(env, in.c_str(), in.size(), &out);
}

/* napi_value <-> std::vector<std::string> */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::vector<std::string> &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::vector<std::string>");
    bool isArray = false;
    napi_is_array(env, in, &isArray);
    if (!isArray) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_value is not an array");
        return napi_invalid_arg;
    }

    uint32_t length = 0;
    napi_status status = napi_get_array_length(env, in, &length);
    if (!((status == napi_ok) && (length > 0))) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "get_array failed, status=%{public}d length=%{public}u",
            status, length);
        return napi_invalid_arg;
    }
    for (uint32_t i = 0; i < length; ++i) {
        napi_value item = nullptr;
        status = napi_get_element(env, in, i, &item);
        if (!((item != nullptr) && (status == napi_ok))) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_get_element failed, status=%{public}d", status);
            return napi_invalid_arg;
        }
        std::string value;
        status = GetValue(env, item, value);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_value is nota string, status=%{public}d", status);
            return napi_invalid_arg;
        }
        out.push_back(value);
    }
    return status;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::vector<std::string> &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::vector<std::string>");
    napi_status status = napi_create_array_with_length(env, in.size(), &out);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "create array failed, status=%{public}d", status);
        return status;
    }
    int index = 0;
    for (auto &item : in) {
        napi_value element = nullptr;
        SetValue(env, item, element);
        status = napi_set_element(env, out, index++, element);
        if (status != napi_ok) {
            PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_set_element failed, status=%{public}d", status);
            return status;
        }
    }
    return status;
}

/* napi_value <-> std::vector<uint8_t> */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::vector<uint8_t> &out)
{
    out.clear();
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::vector<uint8_t> ");
    napi_typedarray_type type = napi_biguint64_array;
    size_t length = 0;
    napi_value buffer = nullptr;
    size_t offset = 0;
    void *data = nullptr;
    napi_status status = napi_get_typedarray_info(env, in, &type, &length, &data, &buffer, &offset);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI,
        "array type=%{public}d length=%{public}d offset=%{public}d  status=%{public}d",
        (int)type, (int)length, (int)offset, status);
    if (!((status == napi_ok) && (length > 0) && (type == napi_uint8_array) && (data != nullptr))) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI,
            "array type=%{public}d length=%{public}d status=%{public}d",
            static_cast<int32_t>(type), static_cast<int32_t>(length), status);
        return napi_invalid_arg;
    }
    out.assign(reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + length);
    return status;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::vector<uint8_t> &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::vector<uint8_t> ");
    if (in.size() <= 0) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "invalid std::vector<uint8_t>");
        return napi_invalid_arg;
    }
    void *data = nullptr;
    napi_value buffer = nullptr;
    napi_status status = napi_create_arraybuffer(env, in.size(), &data, &buffer);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "create array buffer failed, status=%{public}d", status);
        return status;
    }

    if (memcpy_s(data, in.size(), in.data(), in.size()) != EOK) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "memcpy uint8 vector failed, size=%{public}zu", in.size());
        return napi_invalid_arg;
    }
    status = napi_create_typedarray(env, napi_uint8_array, in.size(), buffer, 0, &out);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI,
            "napi_value <- std::vector<uint8_t> invalid value, status=%{public}d", status);
        return status;
    }
    return status;
}

/* napi_value <-> std::map<std::string, int32_t> */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::map<std::string, int32_t> &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::map<std::string, int32_t> ");
    (void)(env);
    (void)(in);
    (void)(out);
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "std::map<std::string, uint32_t> from napi_value, unsupported!");
    return napi_invalid_arg;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::map<std::string, int32_t> &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::map<std::string, int32_t> ");
    napi_status status = napi_create_array_with_length(env, in.size(), &out);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "invalid object, status=%{public}d", status);
        return status;
    }
    int index = 0;
    for (const auto &[key, value] : in) {
        napi_value element = nullptr;
        napi_create_array_with_length(env, TUPLE_SIZE, &element);
        napi_value jsKey = nullptr;
        napi_create_string_utf8(env, key.c_str(), key.size(), &jsKey);
        napi_set_element(env, element, TUPLE_KEY, jsKey);
        napi_value jsValue = nullptr;
        napi_create_int32(env, static_cast<int32_t>(value), &jsValue);
        napi_set_element(env, element, TUPLE_VALUE, jsValue);
        napi_set_element(env, out, index++, element);
    }
    return status;
}

/* napi_value <-> std::map<std::string, int64_t> */
napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::map<std::string, int64_t> &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::map<std::string, int64_t> ");
    (void)(env);
    (void)(in);
    (void)(out);
    PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "std::map<std::string, int64_t> from napi_value, unsupported!");
    return napi_invalid_arg;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::map<std::string, int64_t> &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::map<std::string, int64_t> ");
    napi_status status = napi_create_array_with_length(env, in.size(), &out);
    if (status != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "invalid object, status=%{public}d", status);
        return status;
    }
    int index = 0;
    for (const auto &[key, value] : in) {
        napi_value element = nullptr;
        napi_create_array_with_length(env, TUPLE_SIZE, &element);
        napi_value jsKey = nullptr;
        napi_create_string_utf8(env, key.c_str(), key.size(), &jsKey);
        napi_set_element(env, element, TUPLE_KEY, jsKey);
        napi_value jsValue = nullptr;
        napi_create_int64(env, static_cast<int64_t>(value), &jsValue);
        napi_set_element(env, element, TUPLE_VALUE, jsValue);
        napi_set_element(env, out, index++, element);
    }
    return status;
}

napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::shared_ptr<OHOS::Media::PixelMap> &pixelMap)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::shared_ptr<OHOS::Media::PixelMap>");
    pixelMap = OHOS::Media::PixelMapNapi::GetPixelMap(env, in);
    return napi_ok;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::shared_ptr<OHOS::Media::PixelMap> &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::shared_ptr<OHOS::Media::PixelMap>");
    out = OHOS::Media::PixelMapNapi::CreatePixelMap(env, in);
    return napi_ok;
}

napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::shared_ptr<OHOS::AAFwk::Want> &wantPtr)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::shared_ptr<OHOS::AAFwk::Want>");
    OHOS::AAFwk::Want want;
    AppExecFwk::UnwrapWant(env, in, want);
    wantPtr = std::make_shared<OHOS::AAFwk::Want>(want);
    return napi_ok;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::shared_ptr<OHOS::AAFwk::Want> &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::shared_ptr<OHOS::AAFwk::Want>");
    if (in == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "SetValue in is nullptr");
        return napi_invalid_arg;
    }
    out = OHOS::AppExecFwk::WrapWant(env, *in);
    return napi_ok;
}

napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::shared_ptr<UDMF::Object> &object)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::GetValue Object");
    napi_value attributeNames = nullptr;
    NAPI_CALL_BASE(env, napi_get_property_names(env, in, &attributeNames), napi_invalid_arg);
    uint32_t attributesNum = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, attributeNames, &attributesNum), napi_invalid_arg);
    for (uint32_t i = 0; i < attributesNum; i++) {
        napi_value attributeNameNapi = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, attributeNames, i, &attributeNameNapi), napi_invalid_arg);
        size_t len = 0;
        char str[STR_MAX_SIZE] = { 0 };
        NAPI_CALL_BASE(env, napi_get_value_string_utf8(
            env, attributeNameNapi, str, STR_MAX_SIZE, &len), napi_invalid_arg);
        std::string attributeName = str;
        napi_value attributeValueNapi = nullptr;
        NAPI_CALL_BASE(env, napi_get_named_property(env, in, str, &attributeValueNapi), napi_invalid_arg);

        bool isArrayBuffer = false;
        NAPI_CALL_BASE(env, napi_is_arraybuffer(env, attributeValueNapi, &isArrayBuffer), napi_invalid_arg);
        if (isArrayBuffer) {
            void *data = nullptr;
            size_t dataLen = 0;
            NAPI_CALL_BASE(env, napi_get_arraybuffer_info(env, attributeValueNapi, &data, &dataLen), napi_invalid_arg);
            object->value_[attributeName] = std::vector<uint8_t>(
                reinterpret_cast<uint8_t *>(data), reinterpret_cast<uint8_t *>(data) + dataLen);
            continue;
        }
        napi_valuetype valueType = napi_undefined;
        NAPI_CALL_BASE(env, napi_typeof(env, attributeValueNapi, &valueType), napi_invalid_arg);
        switch (valueType) {
            case napi_valuetype::napi_object:
                object->value_[attributeName] = std::make_shared<UDMF::Object>();
                break;
            case napi_valuetype::napi_number:
                object->value_[attributeName] = double();
                break;
            case napi_valuetype::napi_string:
                object->value_[attributeName] = std::string();
                break;
            case napi_valuetype::napi_boolean:
                object->value_[attributeName] = bool();
                break;
            case napi_valuetype::napi_undefined:
                object->value_[attributeName] = std::monostate();
                break;
            case napi_valuetype::napi_null:
                object->value_[attributeName] = nullptr;
                break;
            default:
                return napi_invalid_arg;
        }
        napi_status status = napi_ok;
        std::visit([&](auto &value) {status = NapiDataUtils::GetValue(env, attributeValueNapi, value);},
            object->value_[attributeName]);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::shared_ptr<UDMF::Object> &object, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::GetValue Object");
    napi_create_object(env, &out);
    for (auto &[key, value] : object->value_) {
        napi_value valueNapi = nullptr;
        if (std::holds_alternative<std::vector<uint8_t>>(value)) {
            auto array = std::get<std::vector<uint8_t>>(value);
            void *data = nullptr;
            size_t len = array.size();
            PASTEBOARD_CALL_BASE(napi_create_arraybuffer(env, len, &data, &valueNapi), napi_generic_failure);
            if (memcpy_s(data, len, reinterpret_cast<const void *>(array.data()), len) != 0) {
                PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "memcpy udmf %{public}s failed, size=%{public}zu",
                    key.c_str(), len);
                return napi_generic_failure;
            }
        } else {
            std::visit([&](const auto &value) {NapiDataUtils::SetValue(env, value, valueNapi);}, value);
        }
        napi_set_named_property(env, out, key.c_str(), valueNapi);
    }
    return napi_ok;
}

napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, std::monostate &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> std::monostate");
    out = std::monostate{};
    return napi_ok;
}

napi_status NapiDataUtils::SetValue(napi_env env, const std::monostate &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- std::monostate");
    return napi_get_undefined(env, &out);
}

napi_status NapiDataUtils::GetValue(napi_env env, napi_value in, nullptr_t &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value -> null");
    out = nullptr;
    return napi_ok;
}

napi_status NapiDataUtils::SetValue(napi_env env, const nullptr_t &in, napi_value &out)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI, "napi_value <- null");
    return napi_get_null(env, &out);
}

bool NapiDataUtils::IsTypeForNapiValue(napi_env env, napi_value param, napi_valuetype expectType)
{
    napi_valuetype valueType = napi_undefined;

    if (param == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "param is null");
        return false;
    }

    if (napi_typeof(env, param, &valueType) != napi_ok) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "get value type failed");
        return false;
    }

    return valueType == expectType;
}

bool NapiDataUtils::IsNull(napi_env env, napi_value value)
{
    napi_valuetype type = napi_undefined;
    napi_status status = napi_typeof(env, value, &type);
    if (status == napi_ok && (type == napi_undefined || type == napi_null)) {
        return true;
    }
    if (type == napi_string) {
        size_t len;
        napi_get_value_string_utf8(env, value, NULL, 0, &len);
        return len == 0;
    }
    return false;
}

napi_value NapiDataUtils::DefineClass(napi_env env, const std::string &name,
    const napi_property_descriptor *properties, size_t count, napi_callback newCb)
{
    // base64("data.udmf") as rootPropName, i.e. global.<root>
    const std::string rootPropName = "ZGF0YS51ZG1m";
    napi_value root = nullptr;
    bool hasRoot = false;
    napi_value global = nullptr;
    napi_get_global(env, &global);
    napi_has_named_property(env, global, rootPropName.c_str(), &hasRoot);
    if (hasRoot) {
        napi_get_named_property(env, global, rootPropName.c_str(), &root);
    } else {
        napi_create_object(env, &root);
        napi_set_named_property(env, global, rootPropName.c_str(), root);
    }

    std::string propName = "constructor_of_" + name;
    napi_value constructor = nullptr;
    bool hasProp = false;
    napi_has_named_property(env, root, propName.c_str(), &hasProp);
    if (hasProp) {
        napi_get_named_property(env, root, propName.c_str(), &constructor);
        if (constructor != nullptr) {
            PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI,
                "got data.distributeddata.%{public}s as constructor", propName.c_str());
            return constructor;
        }
        hasProp = false; // no constructor.
    }

    PASTEBOARD_CALL_BASE(
        napi_define_class(env, name.c_str(), name.size(), newCb, nullptr, count, properties, &constructor),
        nullptr);

    if (constructor == nullptr) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_JS_NAPI, "napi_define_class failed!");
        return nullptr;
    }

    if (!hasProp) {
        napi_set_named_property(env, root, propName.c_str(), constructor);
        PASTEBOARD_HILOGD(PASTEBOARD_MODULE_JS_NAPI,
            "save constructor to data.distributeddata.%{public}s", propName.c_str());
    }
    return constructor;
}
} // namespace MiscServicesNapi
} // namespace OHOS
