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

#include "serializable/serializable.h"
namespace OHOS {
namespace DistributedData {
bool Serializable::Unmarshall(const std::string &jsonStr)
{
    json jsonObj = cJSON_Parse(jsonStr.c_str());
    auto result = false;
    if (jsonObj != nullptr) {
        result = Unmarshal(jsonObj);
        cJSON_Delete(jsonObj);
    }
    return result;
}

std::string Serializable::Marshall() const
{
    json node = nullptr;
    Marshal(node);
    char *value = cJSON_PrintUnformatted(node);
    std::string result;
    if (value != nullptr) {
        result = std::string(value);
        cJSON_free(value);
    }
    cJSON_Delete(node);
    return result;
}

bool Serializable::GetValue(const json node, const std::string &name, std::string &value)
{
    auto subNode = GetSubNode(node, name);
    if (!cJSON_IsString(subNode)) {
        return false;
    }
    value = cJSON_GetStringValue(subNode);
    return true;
}

bool Serializable::GetValue(const json node, const std::string &name, uint8_t &value)
{
    return GetNumber(node, name, value);
}

bool Serializable::GetValue(const json node, const std::string &name, uint16_t &value)
{
    return GetNumber(node, name, value);
}

bool Serializable::GetValue(const json node, const std::string &name, uint32_t &value)
{
    return GetNumber(node, name, value);
}

bool Serializable::GetValue(const json node, const std::string &name, uint64_t &value)
{
    return GetNumber(node, name, value);
}

bool Serializable::GetValue(const json node, const std::string &name, int32_t &value)
{
    return GetNumber(node, name, value);
}

bool Serializable::GetValue(const json node, const std::string &name, int64_t &value)
{
    return GetNumber(node, name, value);
}

bool Serializable::GetValue(const json node, const std::string &name, bool &value)
{
    auto subNode = GetSubNode(node, name);
    if (!cJSON_IsBool(subNode)) {
        return false;
    }
    value = cJSON_IsTrue(subNode);
    return true;
}

bool Serializable::GetValue(const json node, const std::string &name, std::vector<uint8_t> &value)
{
    auto subNode = GetSubNode(node, name);
    if (!cJSON_IsArray(subNode)) {
        return false;
    }
    auto arraySize = cJSON_GetArraySize(subNode);
    for (int i = 0; i < arraySize; i++) {
        value.emplace_back(cJSON_GetNumberValue(cJSON_GetArrayItem(subNode, i)));
    }
    return true;
}

bool Serializable::GetValue(const json node, const std::string &name, Serializable &value)
{
    auto subNode = GetSubNode(node, name);
    if (!cJSON_IsObject(subNode)) {
        return false;
    }
    return value.Unmarshal(subNode);
}

bool Serializable::SetValue(json &node, const std::string &value, const std::string &name)
{
    json subNode = cJSON_CreateString(value.c_str());
    if (name.empty()) {
        node = subNode;
        return true;
    }
    SetValueToObject(node, subNode, name);
    return true;
}

bool Serializable::SetValue(json &node, const uint8_t &value, const std::string &name)
{
    return SetNumber(node, value, name);
}

bool Serializable::SetValue(json &node, const uint16_t &value, const std::string &name)
{
    return SetNumber(node, value, name);
}

bool Serializable::SetValue(json &node, const uint32_t &value, const std::string &name)
{
    return SetNumber(node, value, name);
}

bool Serializable::SetValue(json &node, const uint64_t &value, const std::string &name)
{
    return SetNumber(node, value, name);
}

bool Serializable::SetValue(json &node, const int32_t &value, const std::string &name)
{
    return SetNumber(node, value, name);
}

bool Serializable::SetValue(json &node, const int64_t &value, const std::string &name)
{
    return SetNumber(node, value, name);
}

bool Serializable::SetValue(json &node, const bool &value, const std::string &name)
{
    json subNode = cJSON_CreateBool(value);
    if (name.empty()) {
        node = subNode;
        return true;
    }
    SetValueToObject(node, subNode, name);
    return true;
}

bool Serializable::SetValue(json &node, const std::vector<uint8_t> &value, const std::string &name)
{
    json subNode = cJSON_CreateArray();
    for (const uint8_t &item : value) {
        cJSON_AddItemToArray(subNode, cJSON_CreateNumber(item));
    }
    if (name.empty()) {
        node = subNode;
        return true;
    }
    SetValueToObject(node, subNode, name);
    return true;
}

bool Serializable::SetValue(json &node, const Serializable &value, const std::string &name)
{
    json subNode = nullptr;
    if (!value.Marshal(subNode)) {
        cJSON_Delete(subNode);
        return false;
    }
    if (name.empty()) {
        node = subNode;
        return true;
    }
    SetValueToObject(node, subNode, name);
    return true;
}

Serializable::json Serializable::GetSubNode(const json node, const std::string &name)
{
    if (cJSON_IsNull(node)) {
        return nullptr;
    }

    if (name.empty()) {
        return node;
    }
    if (!cJSON_HasObjectItem(node, name.c_str())) {
        return nullptr;
    }

    return cJSON_GetObjectItem(node, name.c_str());
}
} // namespace DistributedData
} // namespace OHOS