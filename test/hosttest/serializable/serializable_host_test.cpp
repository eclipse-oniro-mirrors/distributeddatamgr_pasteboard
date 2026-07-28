/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Host-only unit test for OHOS::DistributedData::Serializable.
//
// Unlike framework/test/src/serializable_test.cpp (which pulls in clip_plugin,
// config and pasteboard_hilog and therefore only runs on a device), this test
// depends on nothing but serializable.h + cJSON + gtest. It compiles and runs
// on a plain developer/CI host with no IPC, no device and no running service.

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "serializable/serializable.h"

using namespace testing::ext;

namespace OHOS::DistributedData {
using json = Serializable::json;

namespace {
constexpr int32_t TEST_I32_SAMPLE = -42;
constexpr uint64_t TEST_U64_SAMPLE = 0xFFFFFFFFFFULL;
constexpr int32_t TEST_I32_INNER = 7;
constexpr uint32_t TEST_COUNT_INNER = 99;
constexpr int32_t TEST_I32_UNTOUCHED = 123;
constexpr uint8_t TEST_U8_VALUE = 200;
constexpr uint16_t TEST_U16_VALUE = 60000;
constexpr uint32_t TEST_U32_VALUE = 4000000000u;
constexpr int32_t TEST_I32_VALUE = -123;
} // namespace

// A minimal concrete Serializable used to exercise the base helpers end to end.
struct SampleData : public Serializable {
    std::string name;
    int32_t i32 = 0;
    uint64_t u64 = 0;
    bool flag = false;
    std::vector<uint8_t> blob;

    bool Marshal(json &node) const override
    {
        bool ret = true;
        ret = SetValue(node, name, GET_NAME(name)) && ret;
        ret = SetValue(node, i32, GET_NAME(i32)) && ret;
        ret = SetValue(node, u64, GET_NAME(u64)) && ret;
        ret = SetValue(node, flag, GET_NAME(flag)) && ret;
        ret = SetValue(node, blob, GET_NAME(blob)) && ret;
        return ret;
    }

    bool Unmarshal(const json &node) override
    {
        bool ret = true;
        ret = GetValue(node, GET_NAME(name), name) && ret;
        ret = GetValue(node, GET_NAME(i32), i32) && ret;
        ret = GetValue(node, GET_NAME(u64), u64) && ret;
        ret = GetValue(node, GET_NAME(flag), flag) && ret;
        ret = GetValue(node, GET_NAME(blob), blob) && ret;
        return ret;
    }

    bool operator==(const SampleData &o) const
    {
        return name == o.name && i32 == o.i32 && u64 == o.u64 && flag == o.flag && blob == o.blob;
    }
};

// Wraps a nested Serializable to cover the object-in-object path.
struct NestedData : public Serializable {
    SampleData inner;
    uint32_t count = 0;

    bool Marshal(json &node) const override
    {
        bool ret = SetValue(node, inner, GET_NAME(inner));
        ret = SetValue(node, count, GET_NAME(count)) && ret;
        return ret;
    }

    bool Unmarshal(const json &node) override
    {
        bool ret = GetValue(node, GET_NAME(inner), inner);
        ret = GetValue(node, GET_NAME(count), count) && ret;
        return ret;
    }
};

// Has a std::vector<std::string> field. Marshalling this used to crash before
// serializable.cpp initialized its `json node` to nullptr: the vector<T>
// SetValue template + SetValueToObject read an uninitialized cJSON* pointer
// (undefined behaviour whose symptom depended on build flags). Regression guard.
struct VecStringData : public Serializable {
    std::vector<std::string> items;

    bool Marshal(json &node) const override
    {
        return SetValue(node, items, GET_NAME(items));
    }
    bool Unmarshal(const json &node) override
    {
        return GetValue(node, GET_NAME(items), items);
    }
};

class SerializableHostTest : public testing::Test {};

/**
 * @tc.name: RoundTripFull
 * @tc.desc: Round-trip: a fully populated object survives Marshall -> Unmarshall.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, RoundTripFull, TestSize.Level0)
{
    SampleData src;
    src.name = "hello";
    src.i32 = TEST_I32_SAMPLE;
    src.u64 = TEST_U64_SAMPLE;
    src.flag = true;
    src.blob = {1, 2, 3, 250};

    std::string jsonStr = src.Marshall();
    EXPECT_FALSE(jsonStr.empty());

    SampleData dst;
    EXPECT_TRUE(dst.Unmarshall(jsonStr));
    EXPECT_TRUE(src == dst);
}

/**
 * @tc.name: RoundTripNested
 * @tc.desc: Round-trip through a nested Serializable object.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, RoundTripNested, TestSize.Level0)
{
    NestedData src;
    src.inner.name = "child";
    src.inner.i32 = TEST_I32_INNER;
    src.count = TEST_COUNT_INNER;

    std::string jsonStr = src.Marshall();
    EXPECT_FALSE(jsonStr.empty());

    NestedData dst;
    EXPECT_TRUE(dst.Unmarshall(jsonStr));
    EXPECT_TRUE(src.inner == dst.inner);
    EXPECT_EQ(src.count, dst.count);
}

/**
 * @tc.name: UnmarshallInvalidJson
 * @tc.desc: Unmarshall fails cleanly on malformed JSON.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, UnmarshallInvalidJson, TestSize.Level0)
{
    SampleData dst;
    EXPECT_FALSE(dst.Unmarshall("this is not json"));
    EXPECT_FALSE(dst.Unmarshall(""));
}

/**
 * @tc.name: GetValueTypeMismatch
 * @tc.desc: GetValue returns false on a type mismatch (number requested, string present).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, GetValueTypeMismatch, TestSize.Level0)
{
    json node = cJSON_Parse(R"({"i32":"not_a_number"})");
    ASSERT_NE(node, nullptr);
    int32_t out = TEST_I32_UNTOUCHED;
    EXPECT_FALSE(Serializable::GetValue(node, "i32", out));
    EXPECT_EQ(out, TEST_I32_UNTOUCHED); // left untouched on failure
    cJSON_Delete(node);
}

/**
 * @tc.name: GetValueMissingField
 * @tc.desc: GetValue returns false when the named field is absent.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, GetValueMissingField, TestSize.Level0)
{
    json node = cJSON_Parse(R"({"other":1})");
    ASSERT_NE(node, nullptr);
    std::string s = "keep";
    EXPECT_FALSE(Serializable::GetValue(node, "name", s));
    EXPECT_EQ(s, "keep");
    cJSON_Delete(node);
}

/**
 * @tc.name: GetValueOnNullNode
 * @tc.desc: GetSubNode on a null node yields nullptr, not a crash.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, GetValueOnNullNode, TestSize.Level0)
{
    json nullNode = cJSON_CreateNull();
    ASSERT_NE(nullNode, nullptr);
    std::string s;
    EXPECT_FALSE(Serializable::GetValue(nullNode, "name", s));
    cJSON_Delete(nullNode);
}

/**
 * @tc.name: ScalarOverloadsRoundTrip
 * @tc.desc: Each scalar SetValue/GetValue overload round-trips its value.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, ScalarOverloadsRoundTrip, TestSize.Level0)
{
    json node = cJSON_CreateObject();

    EXPECT_TRUE(Serializable::SetValue(node, std::string("str"), "s"));
    EXPECT_TRUE(Serializable::SetValue(node, TEST_U8_VALUE, "u8"));
    EXPECT_TRUE(Serializable::SetValue(node, TEST_U16_VALUE, "u16"));
    EXPECT_TRUE(Serializable::SetValue(node, TEST_U32_VALUE, "u32"));
    EXPECT_TRUE(Serializable::SetValue(node, TEST_I32_VALUE, "i32"));
    EXPECT_TRUE(Serializable::SetValue(node, static_cast<int64_t>(-1), "i64"));
    EXPECT_TRUE(Serializable::SetValue(node, true, "b"));

    std::string s;
    uint8_t u8 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;
    int32_t i32 = 0;
    int64_t i64 = 0;
    bool b = false;

    EXPECT_TRUE(Serializable::GetValue(node, "s", s));
    EXPECT_TRUE(Serializable::GetValue(node, "u8", u8));
    EXPECT_TRUE(Serializable::GetValue(node, "u16", u16));
    EXPECT_TRUE(Serializable::GetValue(node, "u32", u32));
    EXPECT_TRUE(Serializable::GetValue(node, "i32", i32));
    EXPECT_TRUE(Serializable::GetValue(node, "i64", i64));
    EXPECT_TRUE(Serializable::GetValue(node, "b", b));

    EXPECT_EQ(s, "str");
    EXPECT_EQ(u8, TEST_U8_VALUE);
    EXPECT_EQ(u16, TEST_U16_VALUE);
    EXPECT_EQ(u32, TEST_U32_VALUE);
    EXPECT_EQ(i32, TEST_I32_VALUE);
    EXPECT_EQ(i64, -1);
    EXPECT_TRUE(b);

    cJSON_Delete(node);
}

/**
 * @tc.name: ByteVectorRoundTrip
 * @tc.desc: A uint8_t vector round-trips element for element.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, ByteVectorRoundTrip, TestSize.Level0)
{
    json node = cJSON_CreateObject();
    std::vector<uint8_t> in = {0, 1, 127, 255};
    EXPECT_TRUE(Serializable::SetValue(node, in, "blob"));

    std::vector<uint8_t> out;
    EXPECT_TRUE(Serializable::GetValue(node, "blob", out));
    EXPECT_EQ(in, out);

    cJSON_Delete(node);
}

/**
 * @tc.name: BoolTypeMismatch
 * @tc.desc: Bool GetValue rejects a non-bool node.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, BoolTypeMismatch, TestSize.Level0)
{
    json node = cJSON_Parse(R"({"flag":5})");
    ASSERT_NE(node, nullptr);
    bool b = false;
    EXPECT_FALSE(Serializable::GetValue(node, "flag", b));
    cJSON_Delete(node);
}

/**
 * @tc.name: ByteVectorTypeMismatch
 * @tc.desc: Byte-vector GetValue rejects a non-array node.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, ByteVectorTypeMismatch, TestSize.Level0)
{
    json node = cJSON_Parse(R"({"blob":"nope"})");
    ASSERT_NE(node, nullptr);
    std::vector<uint8_t> out;
    EXPECT_FALSE(Serializable::GetValue(node, "blob", out));
    cJSON_Delete(node);
}

/**
 * @tc.name: NestedGetValueTypeMismatch
 * @tc.desc: Nested-object GetValue rejects a non-object node.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, NestedGetValueTypeMismatch, TestSize.Level0)
{
    json node = cJSON_Parse(R"({"inner":123})");
    ASSERT_NE(node, nullptr);
    SampleData out;
    EXPECT_FALSE(Serializable::GetValue(node, "inner", out));
    cJSON_Delete(node);
}

// ---- SetValue with empty name assigns the node directly (the "bare value" path) ----
// This exercises the name.empty() branch of every scalar/array/object overload
// as well as GetSubNode's empty-name early return.
/**
 * @tc.name: SetValueBareNoName
 * @tc.desc: SetValue with an empty name assigns the node directly, exercising the name.empty() branch.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, SetValueBareNoName, TestSize.Level0)
{
    // string
    json s = nullptr;
    EXPECT_TRUE(Serializable::SetValue(s, std::string("bare"), ""));
    ASSERT_TRUE(cJSON_IsString(s));
    std::string sv;
    EXPECT_TRUE(Serializable::GetValue(s, "", sv)); // empty-name GetSubNode -> node
    EXPECT_EQ(sv, "bare");
    cJSON_Delete(s);

    // bool
    json b = nullptr;
    EXPECT_TRUE(Serializable::SetValue(b, true, ""));
    ASSERT_TRUE(cJSON_IsBool(b));
    cJSON_Delete(b);

    // byte vector
    json v = nullptr;
    std::vector<uint8_t> bytes = {9, 8, 7};
    EXPECT_TRUE(Serializable::SetValue(v, bytes, ""));
    ASSERT_TRUE(cJSON_IsArray(v));
    cJSON_Delete(v);

    // nested Serializable
    json obj = nullptr;
    SampleData inner;
    inner.name = "x";
    EXPECT_TRUE(Serializable::SetValue(obj, inner, ""));
    ASSERT_TRUE(cJSON_IsObject(obj));
    cJSON_Delete(obj);
}

// A Serializable whose Marshal always fails, to cover the failure branch of
// SetValue(json&, const Serializable&, ...).
struct FailingMarshal : public Serializable {
    bool Marshal(json &node) const override
    {
        (void)node;
        return false;
    }
    bool Unmarshal(const json &node) override
    {
        (void)node;
        return false;
    }
};

/**
 * @tc.name: SetValueNestedMarshalFails
 * @tc.desc: SetValue(Serializable) returns false when the child's Marshal fails.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, SetValueNestedMarshalFails, TestSize.Level0)
{
    json node = cJSON_CreateObject();
    FailingMarshal bad;
    EXPECT_FALSE(Serializable::SetValue(node, bad, "child"));
    cJSON_Delete(node);
}
// ---- Regression: Marshalling a vector<string> field must not crash and must
// round-trip. This exercises the vector<T> SetValue template + SetValueToObject
// path that read an uninitialized `json node` in serializable.cpp (fixed to
// `= nullptr`). NOTE: the bug was undefined behaviour, so a reverted build does
// not crash *deterministically* here; this test documents the expected
// behaviour and exercises the path. The config suite hits the same path with
// fuller data and crashed reliably pre-fix.
/**
 * @tc.name: VectorStringMarshallRegression
 * @tc.desc: Regression: marshalling a vector<string> field round-trips without crashing (uninitialized-node fix).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SerializableHostTest, VectorStringMarshallRegression, TestSize.Level0)
{
    VecStringData src;
    src.items = {"copy", "paste", "cut"};

    std::string jsonStr = src.Marshall(); // used to segfault
    EXPECT_FALSE(jsonStr.empty());

    VecStringData dst;
    ASSERT_TRUE(dst.Unmarshall(jsonStr));
    EXPECT_EQ(dst.items, src.items);
}
} // namespace OHOS::DistributedData
