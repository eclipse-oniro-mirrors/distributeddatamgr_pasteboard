/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "clip/clip_plugin.h"
#include "config.h"
#include "pasteboard_hilog.h"
#include "serializable/serializable.h"

namespace OHOS::DistributedData {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
class MockSerializable : public OHOS::DistributedData::Serializable {
public:
    MOCK_CONST_METHOD1(Marshal, bool(OHOS::DistributedData::Serializable::json &node));
    MOCK_METHOD1(Unmarshal, bool(const OHOS::DistributedData::Serializable::json &node));
    bool Unmarshall(const std::string &jsonStr) { return true; }
};
class SerializableTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void CreateConfig(Config &config, const std::string &prefix = "");
    void CreateComponent(Config::Component &component, int32_t index, const std::string &prefix);
    Serializable::json ToJson(const std::string &str);
    bool IsSame(Config &oldConfig, Config &newConfig);
    static bool IsSame(Config::Component &oldComp, Config::Component &newComp);

    template <typename T>
    static bool IsSame(std::vector<T> &olds, std::vector<T> &news)
    {
        if (olds.size() != news.size()) {
            return false;
        }

        bool isSame = true;
        auto len = olds.size();
        for (int i = 0; i < len; ++i) {
            isSame = IsSame(olds[i], news[i]) && isSame;
        }
        return isSame;
    }
};

void SerializableTest::SetUpTestCase(void) { }

void SerializableTest::TearDownTestCase(void) { }

void SerializableTest::SetUp(void) { }

void SerializableTest::TearDown(void) { }

Serializable::json SerializableTest::ToJson(const std::string &str)
{
    return cJSON_Parse(str.c_str());
}

bool SerializableTest::IsSame(Config::Component &oldComp, Config::Component &newComp)
{
    bool isSame = true;
    isSame = oldComp.description == newComp.description;
    isSame = oldComp.lib == newComp.lib && isSame;
    isSame = oldComp.constructor == newComp.constructor && isSame;
    isSame = oldComp.destructor == newComp.destructor && isSame;
    isSame = oldComp.params == newComp.params && isSame;
    return isSame;
}

bool SerializableTest::IsSame(Config &oldConfig, Config &newConfig)
{
    bool isSame = true;
    isSame = oldConfig.processLabel == newConfig.processLabel;
    isSame = oldConfig.version == newConfig.version && isSame;
    isSame = oldConfig.features == newConfig.features && isSame;
    isSame = oldConfig.plugins == newConfig.plugins && isSame;
    isSame = IsSame(oldConfig.components, newConfig.components) && isSame;
    return isSame;
}

void SerializableTest::CreateComponent(Config::Component &component, int32_t index, const std::string &prefix)
{
    component.description = prefix + "description" + std::to_string(index);
    component.lib = prefix + "lib" + std::to_string(index);
    component.constructor = prefix + "constructor" + std::to_string(index);
    component.destructor = prefix + "destructor" + std::to_string(index);
    component.params = prefix + "params" + std::to_string(index);
}

void SerializableTest::CreateConfig(Config &config, const std::string &prefix)
{
    config.processLabel = prefix + "processLabel";
    config.version = prefix + "version";
    config.features = { prefix + "feature1", prefix + "feature2" };
    config.plugins = { prefix + "plugin1", prefix + "plugin2" };
    Config::Component component1;
    Config::Component component2;
    int32_t index = 0;
    CreateComponent(component1, index++, prefix);
    CreateComponent(component2, index++, prefix);
    config.components = { component1, component2 };
}

/**
 * @tc.name: SerializableTest001
 * @tc.desc: test serializable with invalid jsonStr .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "SerializableTest001 Start.");
    Config config;
    std::string jsonStr = "";
    auto ret = config.Unmarshall(jsonStr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Unmarshall NULL.");
    ASSERT_FALSE(ret);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Unmarshall not null.");
    jsonStr = "{ a }";
    ret = config.Unmarshall(jsonStr);
    ASSERT_FALSE(ret);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest002
 * @tc.desc: test serializable with valid jsonStr .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "SerializableTest002 Start.");

    std::vector<uint8_t> vec = { 1, 2, 3, 4, 5 };
    std::string jsonStr = "{\n"
                          "        \"param1\":       2,                                \n"
                          "        \"param2\":       3,                                \n"
                          "        \"param3\":       4,                                \n"
                          "        \"param4\":       true,                             \n"
                          "        \"param5\":       [1, 2, 3, 4, 5]                   \n"
                          "}";

    uint32_t res1;
    int32_t res2;
    int64_t res3;
    bool res4;
    std::vector<uint8_t> res5;

    auto node = ToJson(jsonStr);
    auto ret = Serializable::GetValue(node, GET_NAME(param1), res1);
    ASSERT_TRUE(ret);
    ASSERT_EQ(res1, 2);
    ret = Serializable::GetValue(node, GET_NAME(param2), res2);
    ASSERT_TRUE(ret);
    ASSERT_EQ(res2, 3);
    ret = Serializable::GetValue(node, GET_NAME(param3), res3);
    ASSERT_TRUE(ret);
    ASSERT_EQ(res3, 4);
    ret = Serializable::GetValue(node, GET_NAME(param4), res4);
    ASSERT_TRUE(ret);
    ASSERT_TRUE(res4);
    ret = Serializable::GetValue(node, GET_NAME(param5), res5);
    ASSERT_TRUE(ret);
    ASSERT_EQ(res5.size(), vec.size());
    for (uint64_t i = 0; i < res5.size(); i++) {
        ASSERT_EQ(res5[i], vec[i]);
    }

    cJSON_Delete(node);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest003
 * @tc.desc: test serializable GetValue and SetValue with invalid string value .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = R"({"key":null})";
    auto json = ToJson(jsonStr);
    std::string value;
    auto ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);

    jsonStr = R"({"key":1})";
    json = ToJson(jsonStr);
    ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest004
 * @tc.desc: test serializable GetValue and SetValue with invalid uint32_t value .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = R"({"key":null})";
    auto json = ToJson(jsonStr);
    uint32_t value;
    auto ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);

    jsonStr = R"({"key":"1"})";
    json = ToJson(jsonStr);
    ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest005
 * @tc.desc: test serializable GetValue and SetValue with invalid int32_t value .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = R"({"key":null})";
    auto json = ToJson(jsonStr);
    int32_t value;
    auto ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);

    jsonStr = R"({"key":"1"})";
    json = ToJson(jsonStr);
    ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest006
 * @tc.desc: test serializable GetValue and SetValue with invalid int64_t value .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = R"({"key":null})";
    auto json = ToJson(jsonStr);
    int64_t value;
    auto ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);

    jsonStr = R"({"key":"1"})";
    json = ToJson(jsonStr);
    ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest007
 * @tc.desc: test serializable GetValue and SetValue with invalid bool value .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest007, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = R"({"key":null})";
    auto json = ToJson(jsonStr);
    bool value;
    auto ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);

    jsonStr = R"({"key":1})";
    json = ToJson(jsonStr);
    ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest008
 * @tc.desc: test serializable GetValue and SetValue with invalid std::vector<uint8_t> value .
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest008, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = R"({"key":null})";
    auto json = ToJson(jsonStr);
    std::vector<uint8_t> value;
    auto ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);

    jsonStr = R"({"key":"{1, 2, 3}"})";
    json = ToJson(jsonStr);
    ret = Serializable::GetValue(json, "key", value);
    ASSERT_FALSE(ret);
    cJSON_Delete(json);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest009
 * @tc.desc: test serializable SetValue with valid value.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest009, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    uint32_t param1 = 2;
    int32_t param2 = 3;
    int64_t param3 = 4;
    bool param4 = false;
    std::vector<uint8_t> param5 = { 1, 2, 3, 4, 5 };
    Config config;
    CreateConfig(config);

    Serializable::json node = nullptr;
    config.Marshal(node);
    auto ret = Serializable::SetValue(node, param1, GET_NAME(param1));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, param2, GET_NAME(param2));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, param3, GET_NAME(param3));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, param4, GET_NAME(param4));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, param5, GET_NAME(param5));
    ASSERT_TRUE(ret);
    ASSERT_TRUE(cJSON_HasObjectItem(node, GET_NAME(param1)));
    ret = Serializable::GetValue(node, GET_NAME(param1), param1);
    ASSERT_TRUE(ret);
    ASSERT_EQ(param1, 2);
    ASSERT_TRUE(cJSON_HasObjectItem(node, GET_NAME(param2)));
    ret = Serializable::GetValue(node, GET_NAME(param2), param2);
    ASSERT_TRUE(ret);
    ASSERT_EQ(param2, 3);
    ASSERT_TRUE(cJSON_HasObjectItem(node, GET_NAME(param3)));
    ret = Serializable::GetValue(node, GET_NAME(param3), param3);
    ASSERT_TRUE(ret);
    ASSERT_EQ(param3, 4);
    ASSERT_TRUE(cJSON_HasObjectItem(node, GET_NAME(param4)));
    ret = Serializable::GetValue(node, GET_NAME(param4), param4);
    ASSERT_TRUE(ret);
    ASSERT_EQ(param4, false);
    ASSERT_TRUE(cJSON_HasObjectItem(node, GET_NAME(param5)));
    cJSON_Delete(node);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest010
 * @tc.desc: test serializable SetValue with valid value.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest010, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "SerializableTest010 Start.");
    Config config;
    CreateConfig(config);

    Config newConfig;
    CreateConfig(newConfig, "new");

    Serializable::json node = nullptr;
    config.Marshal(node);
    auto ret = Serializable::SetValue(node, newConfig.version, GET_NAME(version));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, newConfig.processLabel, GET_NAME(processLabel));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, newConfig.features, GET_NAME(features));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, newConfig.plugins, GET_NAME(plugins));
    ASSERT_TRUE(ret);
    ret = Serializable::SetValue(node, newConfig.components, GET_NAME(components));
    ASSERT_TRUE(ret);
    config.Unmarshal(node);
    ASSERT_EQ(IsSame(config, newConfig), true);
    cJSON_Delete(node);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: SerializableTest011
 * @tc.desc: test serializable Unmarshall with valid jsonstr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SerializableTest011, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    std::string jsonStr = "{\n"
                          "        \"processLabel\": \"processLabel\",                \n"
                          "        \"version\":      \"version\",                     \n"
                          "        \"features\":     [\"features1\", \"features2\"], \n"
                          "        \"plugins\":      [\"plugins1\", \"plugins2\"],   \n"
                          "        \"components\":   [{                                \n"
                          "                        \"description\":  \"description1\",\n"
                          "                        \"lib\":  \"lib1\",                \n"
                          "                        \"constructor\":  \"constructor1\",\n"
                          "                        \"destructor\":   \"destructor1\", \n"
                          "                        \"params\":       \"params1\"      \n"
                          "                }, {                                      \n"
                          "                        \"description\":  \"description2\",\n"
                          "                        \"lib\":  \"lib2\",                \n"
                          "                        \"constructor\":  \"constructor2\",\n"
                          "                        \"destructor\":   \"destructor2\", \n"
                          "                        \"params\":       \"params2\"      \n"
                          "                }]                                      \n"
                          "}";

    Config config;
    auto ret = config.Unmarshall(jsonStr);
    ASSERT_TRUE(ret);
    ASSERT_EQ(config.processLabel, "processLabel");
    ASSERT_EQ(config.version, "version");
    ASSERT_EQ(config.features[0], "features1");
    ASSERT_EQ(config.features[1], "features2");
    ASSERT_EQ(config.plugins[0], "plugins1");
    ASSERT_EQ(config.plugins[1], "plugins2");
    ASSERT_EQ(config.components[0].description, "description1");
    ASSERT_EQ(config.components[0].lib, "lib1");
    ASSERT_EQ(config.components[0].constructor, "constructor1");
    ASSERT_EQ(config.components[0].destructor, "destructor1");
    ASSERT_EQ(config.components[0].params, "params1");
    ASSERT_EQ(config.components[1].description, "description2");
    ASSERT_EQ(config.components[1].lib, "lib2");
    ASSERT_EQ(config.components[1].constructor, "constructor2");
    ASSERT_EQ(config.components[1].destructor, "destructor2");
    ASSERT_EQ(config.components[1].params, "params2");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
 * @tc.name: GlobalEventTest001
 * @tc.desc: test GlobalEvent serializable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, GlobalEventTest001, TestSize.Level0)
{
    ClipPlugin::GlobalEvent event;
    event.deviceId = "deviceId";
    event.expiration = 1;
    event.seqId = 0;
    std::string data = event.Marshall();
    ClipPlugin::GlobalEvent event1;
    if (!event1.Unmarshall(data)) {
        PASTEBOARD_HILOGE(PASTEBOARD_MODULE_INNERKIT, "Unmarshall event failed.");
    }
    ASSERT_TRUE(event == event1);
}

/**
 * @tc.name: GetValueTest001
 * @tc.desc: GetValue return false
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, GetValueTest001, TestSize.Level0)
{
    std::string value;
    Serializable::json nullNode = nullptr;
    bool ret = Serializable::GetValue(nullNode, "key", value);
    ASSERT_FALSE(ret);
}
/**
 * @tc.name: SetValueTest001
 * @tc.desc: test SetValue name.empty
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SetValueTest001, TestSize.Level0)
{
    Serializable::json node = nullptr;
    bool ret = Serializable::SetValue(node, true, "");
    ASSERT_TRUE(ret);
    ASSERT_TRUE(cJSON_IsBool(node));
    ASSERT_EQ(cJSON_IsTrue(node), true);
    if (node != nullptr) {
        cJSON_Delete(node);
    }
}

/**
 * @tc.name: SetValueTest002
 * @tc.desc: test SetValue name.empty branch (vector<uint8_t> version)
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SetValueTest002, TestSize.Level0)
{
    Serializable::json node = nullptr;
    std::vector<uint8_t> testValue = {1, 2, 3};
    bool ret = Serializable::SetValue(node, testValue, "");
    ASSERT_TRUE(ret);
    ASSERT_TRUE(cJSON_IsArray(node));
    if (node != nullptr) {
        cJSON_Delete(node);
    }
}

/**
 * @tc.name: SetValueTest003
 * @tc.desc: test SetValue - cover !value.Marshal
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SetValueTest003, TestSize.Level0)
{
    OHOS::DistributedData::Serializable::json node = nullptr;
    MockSerializable mockValue;
    std::string name = "test_key";
    EXPECT_CALL(mockValue, Marshal(_)).WillOnce(Return(false));
    bool ret = OHOS::DistributedData::Serializable::SetValue(node, mockValue, name);
    ASSERT_FALSE(ret);
}

/**
 * @tc.name: SetValueTest004
 * @tc.desc: test SetValue - cover SetValueToObject + return true
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(SerializableTest, SetValueTest004, TestSize.Level0)
{
    OHOS::DistributedData::Serializable::json node = cJSON_CreateObject();
    MockSerializable mockValue;
    std::string name = "test_serializable_key";
    EXPECT_CALL(mockValue, Marshal(_))
        .WillOnce(DoAll(
            Invoke([](OHOS::DistributedData::Serializable::json &node) {
                node = cJSON_CreateBool(true);
                return true;
            })
        ));

    bool ret = OHOS::DistributedData::Serializable::SetValue(node, mockValue, name);
    ASSERT_TRUE(ret);
    cJSON* subNode = cJSON_GetObjectItem(node, name.c_str());
    ASSERT_TRUE(subNode != nullptr);
    ASSERT_TRUE(cJSON_IsBool(subNode));
    ASSERT_TRUE(cJSON_IsTrue(subNode));
    if (node != nullptr) {
        cJSON_Delete(node);
    }
}
/**
 * @tc.name: GetSubNodeTest001
 * @tc.desc: test GetSubNode - cover cJSON_IsNull(node)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SerializableTest, GetSubNodeTest001, TestSize.Level0)
{
    Serializable::json nullNode = nullptr;
    auto result = Serializable::GetSubNode(nullNode, "test_key");
    ASSERT_EQ(result, nullptr);
}
} // namespace OHOS::DistributedData