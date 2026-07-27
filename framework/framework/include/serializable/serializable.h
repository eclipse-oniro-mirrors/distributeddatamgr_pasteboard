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

#ifndef OHOS_DISTRIBUTED_SERIALIZABLE_SERIALIZABLE_H
#define OHOS_DISTRIBUTED_SERIALIZABLE_SERIALIZABLE_H
#include <cstdint>
#include <string>
#include <vector>

#include "api/visibility.h"
#ifndef JSON_NOEXCEPTION
#define JSON_NOEXCEPTION
#endif
#include "cJSON.h"

namespace OHOS {
namespace DistributedData {
#ifndef GET_NAME
#define GET_NAME(value) #value
#endif
struct Serializable {
public:
    using json = cJSON *;
    API_EXPORT bool Unmarshall(const std::string &jsonStr);
    API_EXPORT std::string Marshall() const;
    virtual bool Marshal(json &node) const = 0;
    virtual bool Unmarshal(const json &node) = 0;
    API_EXPORT static bool GetValue(const json node, const std::string &name, std::string &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, uint8_t &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, uint16_t &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, uint32_t &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, uint64_t &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, int32_t &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, int64_t &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, bool &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, std::vector<uint8_t> &value);
    API_EXPORT static bool GetValue(const json node, const std::string &name, Serializable &value);
    API_EXPORT static bool SetValue(json &node, const std::string &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const uint8_t &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const uint16_t &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const uint32_t &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const uint64_t &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const int32_t &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const int64_t &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const bool &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const std::vector<uint8_t> &value, const std::string &name = "");
    API_EXPORT static bool SetValue(json &node, const Serializable &value, const std::string &name = "");

    template<typename T>
    static bool SetValue(json &node, const std::vector<T> &values, const std::string &name = "")
    {
        bool result = true;
        auto subNode = cJSON_CreateArray();
        for (const auto &value : values) {
            json item = nullptr;
            result = SetValue(item, value, "") && result;
            cJSON_AddItemToArray(subNode, item);
        }

        if (name.empty()) {
            node = subNode;
            return result;
        }
        SetValueToObject(node, subNode, name);
        return result;
    }

protected:
    API_EXPORT ~Serializable() = default;

    template<typename T>
    static bool GetValue(const json node, const std::string &name, std::vector<T> &values)
    {
        auto subNode = GetSubNode(node, name);
        if (!cJSON_IsArray(subNode)) {
            return false;
        }
        bool result = true;
        auto arraySize = cJSON_GetArraySize(subNode);
        values.resize(arraySize);
        for (int i = 0; i < arraySize; ++i) {
            result = GetValue(cJSON_GetArrayItem(subNode, i), "", values[i]) && result;
        }
        return result;
    }

    template<typename T>
    static bool SetNumber(json &node, const T &value, const std::string &name)
    {
        json subNode = cJSON_CreateNumber(value);
        if (name.empty()) {
            node = subNode;
            return true;
        }
        SetValueToObject(node, subNode, name);
        return true;
    }

    template<typename T>
    static bool GetNumber(const json node, const std::string &name, T &value)
    {
        auto subNode = GetSubNode(node, name);
        if (!cJSON_IsNumber(subNode)) {
            return false;
        }
        value = cJSON_GetNumberValue(subNode);
        return true;
    }

    API_EXPORT static json GetSubNode(const json node, const std::string &name);
    static void SetValueToObject(json &node, json subNode, const std::string &name)
    {
        if (!cJSON_IsObject(node)) {
            node = cJSON_CreateObject();
        }
        if (cJSON_HasObjectItem(node, name.c_str())) {
            cJSON_ReplaceItemInObject(node, name.c_str(), subNode);
        } else {
            cJSON_AddItemToObject(node, name.c_str(), subNode);
        }
    }
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_SERIALIZABLE_SERIALIZABLE_H
