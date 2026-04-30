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
#include "tlv_writeable.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

class TLVWriteableTest : public testing::Test {
public:
    TLVWriteableTest() {};
    ~TLVWriteableTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void TLVWriteableTest::SetUpTestCase(void) { }

void TLVWriteableTest::TearDownTestCase(void) { }

void TLVWriteableTest::SetUp(void) { }

void TLVWriteableTest::TearDown(void) { }

/**
 * @tc.name: WriteBoolTest001
 * @tc.desc: test Write for bool type true value
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteBoolTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBoolTest001 start");
    WriteOnlyBuffer buff(100);
    bool res = buff.Write(1, true);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBoolTest001 end");
}

/**
 * @tc.name: WriteBoolTest002
 * @tc.desc: test Write for bool type false value
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteBoolTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBoolTest002 start");
    WriteOnlyBuffer buff(100);
    bool res = buff.Write(1, false);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBoolTest002 end");
}

/**
 * @tc.name: WriteInt8Test001
 * @tc.desc: test Write for int8_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteInt8Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt8Test001 start");
    WriteOnlyBuffer buff(100);
    int8_t value = -128;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt8Test001 end");
}

/**
 * @tc.name: WriteInt8Test002
 * @tc.desc: test Write for int8_t type positive value
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteInt8Test002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt8Test002 start");
    WriteOnlyBuffer buff(100);
    int8_t value = 127;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt8Test002 end");
}

/**
 * @tc.name: WriteInt16Test001
 * @tc.desc: test Write for int16_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteInt16Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt16Test001 start");
    WriteOnlyBuffer buff(100);
    int16_t value = 1000;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt16Test001 end");
}

/**
 * @tc.name: WriteInt32Test001
 * @tc.desc: test Write for int32_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteInt32Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt32Test001 start");
    WriteOnlyBuffer buff(100);
    int32_t value = 12345;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt32Test001 end");
}

/**
 * @tc.name: WriteInt64Test001
 * @tc.desc: test Write for int64_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteInt64Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt64Test001 start");
    WriteOnlyBuffer buff(100);
    int64_t value = 1234567890123LL;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteInt64Test001 end");
}

/**
 * @tc.name: WriteUint32Test001
 * @tc.desc: test Write for uint32_t type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteUint32Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteUint32Test001 start");
    WriteOnlyBuffer buff(100);
    uint32_t value = 4294967295;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteUint32Test001 end");
}

/**
 * @tc.name: WriteDoubleTest001
 * @tc.desc: test Write for double type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteDoubleTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteDoubleTest001 start");
    WriteOnlyBuffer buff(100);
    double value = 3.14159265358979;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteDoubleTest001 end");
}

/**
 * @tc.name: WriteStringTest001
 * @tc.desc: test Write for std::string type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteStringTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteStringTest001 start");
    WriteOnlyBuffer buff(100);
    std::string value = "Hello World";
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteStringTest001 end");
}

/**
 * @tc.name: WriteStringTest002
 * @tc.desc: test Write for empty std::string
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteStringTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteStringTest002 start");
    WriteOnlyBuffer buff(100);
    std::string value = "";
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteStringTest002 end");
}

/**
 * @tc.name: WriteVectorUint8Test001
 * @tc.desc: test Write for std::vector<uint8_t> type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteVectorUint8Test001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVectorUint8Test001 start");
    WriteOnlyBuffer buff(100);
    std::vector<uint8_t> value = {1, 2, 3, 4, 5
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVectorUint8Test001 end");
};
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: WriteVectorUint8Test002
 * @tc.desc: test Write for empty std::vector<uint8_t>
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteVectorUint8Test002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVectorUint8Test002 start");
    WriteOnlyBuffer buff(100);
    std::vector<uint8_t> value;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVectorUint8Test002 end");
}

/**
 * @tc.name: WriteMonostateTest001
 * @tc.desc: test Write for std::monostate type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteMonostateTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteMonostateTest001 start");
    WriteOnlyBuffer buff(100);
    std::monostate value;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteMonostateTest001 end");
}

/**
 * @tc.name: WriteVoidPtrTest001
 * @tc.desc: test Write for void* type
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteVoidPtrTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVoidPtrTest001 start");
    WriteOnlyBuffer buff(100);
    void *value = nullptr;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVoidPtrTest001 end");
}

/**
 * @tc.name: WriteSharedPtrTest001
 * @tc.desc: test Write for std::shared_ptr nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteSharedPtrTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteSharedPtrTest001 start");
    WriteOnlyBuffer buff(100);
    std::shared_ptr<int32_t> value = nullptr;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteSharedPtrTest001 end");
}

/**
 * @tc.name: WriteVectorTemplateTest001
 * @tc.desc: test Write for std::vector<int32_t> template
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteVectorTemplateTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVectorTemplateTest001 start");
    WriteOnlyBuffer buff(200);
    std::vector<int32_t> value = {1, 2, 3, 4, 5
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteVectorTemplateTest001 end");
};
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: WriteBufferOverflowTest001
 * @tc.desc: test Write with buffer overflow
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteBufferOverflowTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBufferOverflowTest001 start");
    WriteOnlyBuffer buff(10);
    std::string value = "This string is too long for the buffer";
    bool res = buff.Write(1, value);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBufferOverflowTest001 end");
}

/**
 * @tc.name: WriteBufferOverflowTest002
 * @tc.desc: test Write int32_t with insufficient buffer
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteBufferOverflowTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBufferOverflowTest002 start");
    WriteOnlyBuffer buff(5);
    int32_t value = 100;
    bool res = buff.Write(1, value);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteBufferOverflowTest002 end");
}

/**
 * @tc.name: WriteMapTest001
 * @tc.desc: test Write for std::map<std::string, std::vector<uint8_t>>
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteMapTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteMapTest001 start");
    WriteOnlyBuffer buff(200);
    std::map<std::string, std::vector<uint8_t>> value;
    value["key1"] = {1, 2, 3
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteMapTest001 end");
};
    value["key2"] = {4, 5, 6};
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: WriteMapTest002
 * @tc.desc: test Write for empty std::map
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, WriteMapTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteMapTest002 start");
    WriteOnlyBuffer buff(100);
    std::map<std::string, std::vector<uint8_t>> value;
    bool res = buff.Write(1, value);
    EXPECT_TRUE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "WriteMapTest002 end");
}

/**
 * @tc.name: IsRemoteEncodeTest001
 * @tc.desc: test IsRemoteEncode function
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, IsRemoteEncodeTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsRemoteEncodeTest001 start");
    EXPECT_FALSE(IsRemoteEncode());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsRemoteEncodeTest001 end");
}

/**
 * @tc.name: HasExpectBufferTest001
 * @tc.desc: test WriteOnlyBuffer HasExpectBuffer
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, HasExpectBufferTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HasExpectBufferTest001 start");
    WriteOnlyBuffer buff(100);
    EXPECT_TRUE(buff.HasExpectBuffer(50));
    EXPECT_TRUE(buff.HasExpectBuffer(100));
    EXPECT_FALSE(buff.HasExpectBuffer(101));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "HasExpectBufferTest001 end");
}

/**
 * @tc.name: SkipTest001
 * @tc.desc: test WriteOnlyBuffer Skip method
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, SkipTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SkipTest001 start");
    WriteOnlyBuffer buff(100);
    bool res = buff.Skip(50);
    EXPECT_TRUE(res);
    res = buff.Skip(50);
    EXPECT_TRUE(res);
    res = buff.Skip(1);
    EXPECT_FALSE(res);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SkipTest001 end");
}

/**
 * @tc.name: IsEnoughTest001
 * @tc.desc: test WriteOnlyBuffer IsEnough method
 * @tc.type: FUNC
 */
HWTEST_F(TLVWriteableTest, IsEnoughTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsEnoughTest001 start");
    WriteOnlyBuffer buff(100);
    EXPECT_TRUE(buff.IsEnough());
    buff.Skip(99);
    EXPECT_TRUE(buff.IsEnough());
    buff.Skip(1);
    EXPECT_FALSE(buff.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "IsEnoughTest001 end");
}