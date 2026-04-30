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

#include <gtest/gtest.h>
#include "tlv_readable.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
class TLVReadableTest : public testing::Test {
public:
    TLVReadableTest() {};
    ~TLVReadableTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void TLVReadableTest::SetUpTestCase(void) { }

void TLVReadableTest::TearDownTestCase(void) { }

void TLVReadableTest::SetUp(void) { }

void TLVReadableTest::TearDown(void) { }

/**
 * @tc.name: ReadValueTest001
 * @tc.desc: ReadValue
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueTest001 start");
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    void *value = nullptr;
    TLVHead head;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueTest001 end");
}

/**
 * @tc.name: ReadValueTest002
 * @tc.desc: ReadValue
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueTest002 start");
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    head.len = 0;
    bool value = true;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueTest002 end");
}

/**
 * @tc.name: ReadValueTest003
 * @tc.desc: ReadValue
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueTest003 start");
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    head.len = 0;
    int32_t value = 10;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueTest003 end");
}

/**
 * @tc.name: ReadHeadTest001
 * @tc.desc: test ReadHead with empty buffer
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadHeadTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest001 start");
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    bool res = buff.ReadHead(head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest001 end");
}

/**
 * @tc.name: ReadHeadTest002
 * @tc.desc: test ReadHead with buffer size less than sizeof(TLVHead)
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadHeadTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest002 start");
    std::vector<std::uint8_t> buffer(1);
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    bool res = buff.ReadHead(head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest002 end");
}

/**
 * @tc.name: ReadHeadTest003
 * @tc.desc: test ReadHead with buffer size equal to sizeof(TLVHead)
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadHeadTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest003 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = 10;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    bool res = buff.ReadHead(head);
    EXPECT_TRUE(res);
    EXPECT_EQ(head.tag, 1);
    EXPECT_EQ(head.len, 10);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest003 end");
}

/**
 * @tc.name: ReadHeadTest004
 * @tc.desc: test ReadHead with buffer size larger than sizeof(TLVHead)
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadHeadTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest004 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + 100);
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 100;
    pHead->len = 50;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    bool res = buff.ReadHead(head);
    EXPECT_TRUE(res);
    EXPECT_EQ(head.tag, 100);
    EXPECT_EQ(head.len, 50);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest004 end");
}

/**
 * @tc.name: ReadHeadTest005
 * @tc.desc: test ReadHead called multiple times to verify cursor advancement
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadHeadTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest005 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) * 3);
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = 0;
    TLVHead *pHead2 = reinterpret_cast<TLVHead *>(buffer.data() + sizeof(TLVHead));
    pHead2->tag = 2;
    pHead2->len = 0;
    TLVHead *pHead3 = reinterpret_cast<TLVHead *>(buffer.data() + sizeof(TLVHead) * 2);
    pHead3->tag = 3;
    pHead3->len = 0;
    ReadOnlyBuffer buff(buffer);
    TLVHead head1, head2, head3;
    bool res1 = buff.ReadHead(head1);
    EXPECT_TRUE(res1);
    EXPECT_EQ(head1.tag, 1);
    bool res2 = buff.ReadHead(head2);
    EXPECT_TRUE(res2);
    EXPECT_EQ(head2.tag, 2);
    bool res3 = buff.ReadHead(head3);
    EXPECT_TRUE(res3);
    EXPECT_EQ(head3.tag, 3);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadTest005 end");
}

/**
 * @tc.name: ReadValueBoolTest001
 * @tc.desc: test ReadValue for bool type with valid data
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueBoolTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueBoolTest001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(uint8_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(uint8_t);
    buffer[sizeof(TLVHead)] = 1;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    bool value = false;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_TRUE(value);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueBoolTest001 end");
}

/**
 * @tc.name: ReadValueBoolTest002
 * @tc.desc: test ReadValue for bool type with invalid value (>1)
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueBoolTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueBoolTest002 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(uint8_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(uint8_t);
    buffer[sizeof(TLVHead)] = 2;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    bool value = false;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueBoolTest002 end");
}

/**
 * @tc.name: ReadValueBoolTest003
 * @tc.desc: test ReadValue for bool type with zero value
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueBoolTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueBoolTest003 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(uint8_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(uint8_t);
    buffer[sizeof(TLVHead)] = 0;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    bool value = true;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_FALSE(value);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueBoolTest003 end");
}

/**
 * @tc.name: ReadValueInt8Test001
 * @tc.desc: test ReadValue for int8_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueInt8Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt8Test001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(int8_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(int8_t);
    int8_t expectedValue = -128;
    memcpy_s(buffer.data() + sizeof(TLVHead), sizeof(int8_t), &expectedValue, sizeof(int8_t));
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    int8_t value = 0;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_EQ(value, expectedValue);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt8Test001 end");
}

/**
 * @tc.name: ReadValueInt16Test001
 * @tc.desc: test ReadValue for int16_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueInt16Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt16Test001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(int16_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(int16_t);
    int16_t expectedValue = 1000;
    memcpy_s(buffer.data() + sizeof(TLVHead), sizeof(int16_t), &expectedValue, sizeof(int16_t));
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    int16_t value = 0;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt16Test001 end");
}

/**
 * @tc.name: ReadValueInt32Test001
 * @tc.desc: test ReadValue for int32_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueInt32Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt32Test001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(int32_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(int32_t);
    int32_t expectedValue = 12345;
    memcpy_s(buffer.data() + sizeof(TLVHead), sizeof(int32_t), &expectedValue, sizeof(int32_t));
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    int32_t value = 0;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt32Test001 end");
}

/**
 * @tc.name: ReadValueInt64Test001
 * @tc.desc: test ReadValue for int64_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueInt64Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt64Test001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(int64_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(int64_t);
    int64_t expectedValue = 1234567890123LL;
    memcpy_s(buffer.data() + sizeof(TLVHead), sizeof(int64_t), &expectedValue, sizeof(int64_t));
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    int64_t value = 0;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueInt64Test001 end");
}

/**
 * @tc.name: ReadValueDoubleTest001
 * @tc.desc: test ReadValue for double type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueDoubleTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueDoubleTest001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(double));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(double);
    double expectedValue = 3.14159265358979;
    memcpy_s(buffer.data() + sizeof(TLVHead), sizeof(double), &expectedValue, sizeof(double));
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    double value = 0.0;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueDoubleTest001 end");
}

/**
 * @tc.name: ReadValueUint32Test001
 * @tc.desc: test ReadValue for uint32_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueUint32Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueUint32Test001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + sizeof(uint32_t));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(uint32_t);
    uint32_t expectedValue = 4294967295;
    memcpy_s(buffer.data() + sizeof(TLVHead), sizeof(uint32_t), &expectedValue, sizeof(uint32_t));
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    uint32_t value = 0;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueUint32Test001 end");
}

/**
 * @tc.name: ReadValueStringTest001
 * @tc.desc: test ReadValue for std::string type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueStringTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueStringTest001 start");
    std::string expectedValue = "Hello World";
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + expectedValue.size());
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = expectedValue.size();
    memcpy_s(buffer.data() + sizeof(TLVHead), expectedValue.size(), expectedValue.c_str(), expectedValue.size());
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    std::string value;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_EQ(value, expectedValue);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueStringTest001 end");
}

/**
 * @tc.name: ReadValueStringTest002
 * @tc.desc: test ReadValue for empty std::string
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueStringTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueStringTest002 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = 0;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    std::string value;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_EQ(value, "");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueStringTest002 end");
}

/**
 * @tc.name: ReadValueVectorUint8Test001
 * @tc.desc: test ReadValue for std::vector<uint8_t> type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueVectorUint8Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueVectorUint8Test001 start");
    std::vector<uint8_t> expectedValue = {1, 2, 3, 4, 5
};
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + expectedValue.size());
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = expectedValue.size();
    memcpy_s(buffer.data() + sizeof(TLVHead), expectedValue.size(), expectedValue.data(), expectedValue.size());
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    std::vector<uint8_t> value;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_EQ(value, expectedValue);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueVectorUint8Test001 end");
}

/**
 * @tc.name: ReadValueVectorUint8Test002
 * @tc.desc: test ReadValue for empty std::vector<uint8_t>
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueVectorUint8Test002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueVectorUint8Test002 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = 0;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    std::vector<uint8_t> value;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    EXPECT_EQ(value.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueVectorUint8Test002 end");
}

/**
 * @tc.name: ReadValueMonostateTest001
 * @tc.desc: test ReadValue for std::monostate type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueMonostateTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueMonostateTest001 start");
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    std::monostate value;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueMonostateTest001 end");
}

/**
 * @tc.name: ReadValueRawMemTest001
 * @tc.desc: test ReadValue for RawMem type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueRawMemTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueRawMemTest001 start");
    std::vector<uint8_t> expectedData = {10, 20, 30, 40
};
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + expectedData.size());
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = expectedData.size();
    memcpy_s(buffer.data() + sizeof(TLVHead), expectedData.size(), expectedData.data(), expectedData.size());
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    RawMem rawMem;
    bool res = buff.ReadValue(rawMem, head);
    EXPECT_TRUE(res);
    EXPECT_EQ(rawMem.bufferLen, expectedData.size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueRawMemTest001 end");
}

/**
 * @tc.name: TLVBufferSkipTest001
 * @tc.desc: test TLVBuffer Skip method
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, TLVBufferSkipTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest001 start");
    std::vector<std::uint8_t> buffer(100);
    ReadOnlyBuffer buff(buffer);
    bool res = buff.Skip(50);
    EXPECT_TRUE(res);
    res = buff.Skip(50);
    EXPECT_TRUE(res);
    res = buff.Skip(1);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest001 end");
}

/**
 * @tc.name: TLVBufferIsEnoughTest001
 * @tc.desc: test TLVBuffer IsEnough method
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, TLVBufferIsEnoughTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest001 start");
    std::vector<std::uint8_t> buffer(100);
    ReadOnlyBuffer buff(buffer);
    EXPECT_TRUE(buff.IsEnough());
    buff.Skip(99);
    EXPECT_TRUE(buff.IsEnough());
    buff.Skip(1);
    EXPECT_FALSE(buff.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest001 end");
}

/**
 * @tc.name: TLVBufferHasExpectBufferTest001
 * @tc.desc: test TLVBuffer HasExpectBuffer method
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, TLVBufferHasExpectBufferTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest001 start");
    std::vector<std::uint8_t> buffer(100);
    ReadOnlyBuffer buff(buffer);
    EXPECT_TRUE(buff.HasExpectBuffer(50));
    EXPECT_TRUE(buff.HasExpectBuffer(100));
    EXPECT_FALSE(buff.HasExpectBuffer(101));
    buff.Skip(50);
    EXPECT_TRUE(buff.HasExpectBuffer(50));
    EXPECT_FALSE(buff.HasExpectBuffer(51));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest001 end");
}

/**
 * @tc.name: RecursiveGuardTest001
 * @tc.desc: test RecursiveGuard within limit
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, RecursiveGuardTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RecursiveGuardTest001 start");
    RecursiveGuard guard1;
    EXPECT_TRUE(guard1.IsValid());
    RecursiveGuard guard2;
    EXPECT_TRUE(guard2.IsValid());
    RecursiveGuard guard3;
    EXPECT_TRUE(guard3.IsValid());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RecursiveGuardTest001 end");
}

/**
 * @tc.name: ReadValueSharedPtrTest001
 * @tc.desc: test ReadValue for std::shared_ptr type
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueSharedPtrTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueSharedPtrTest001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead));
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = 0;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    std::shared_ptr<int32_t> value;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadValueSharedPtrTest001 end");
}

/**
 * @tc.name: ReadHeadInvalidLenTest001
 * @tc.desc: test ReadHead with len mismatch
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadHeadInvalidLenTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadInvalidLenTest001 start");
    std::vector<std::uint8_t> buffer(sizeof(TLVHead) + 1);
    TLVHead *pHead = reinterpret_cast<TLVHead *>(buffer.data());
    pHead->tag = 1;
    pHead->len = sizeof(int32_t);
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    buff.ReadHead(head);
    int32_t value = 0;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ReadHeadInvalidLenTest001 end");
}
