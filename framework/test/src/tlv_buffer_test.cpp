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
#include "tlv_buffer.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;

class TestTLVBuffer : public TLVBuffer {
public:
    void SetCursor(size_t cursor) { cursor_ = cursor; }
    void SetTotal(size_t total) { total_ = total; }
};

class TLVBufferTest : public testing::Test {
public:
    TLVBufferTest() {};
    ~TLVBufferTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void TLVBufferTest::SetUpTestCase(void) { }

void TLVBufferTest::TearDownTestCase(void) { }

void TLVBufferTest::SetUp(void) { }

void TLVBufferTest::TearDown(void) { }

/**
 * @tc.name: TLVBufferConstructorTest001
 * @tc.desc: test TLVBuffer default constructor
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferConstructorTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferConstructorTest001 start");
    TLVBuffer buffer;
    EXPECT_FALSE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferConstructorTest001 end");
}

/**
 * @tc.name: TLVBufferConstructorTest002
 * @tc.desc: test TLVBuffer constructor with size
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferConstructorTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferConstructorTest002 start");
    TLVBuffer buffer(100);
    EXPECT_TRUE(buffer.IsEnough());
    EXPECT_TRUE(buffer.HasExpectBuffer(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferConstructorTest002 end");
}

/**
 * @tc.name: TLVBufferConstructorTest003
 * @tc.desc: test TLVBuffer constructor with zero size
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferConstructorTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferConstructorTest003 start");
    TLVBuffer buffer(0);
    EXPECT_FALSE(buffer.IsEnough());
    EXPECT_FALSE(buffer.HasExpectBuffer(1));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferConstructorTest003 end");
}

/**
 * @tc.name: TLVBufferSkipTest001
 * @tc.desc: test TLVBuffer Skip with valid length
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferSkipTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest001 start");
    TLVBuffer buffer(100);
    bool result = buffer.Skip(50);
    EXPECT_TRUE(result);
    EXPECT_TRUE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest001 end");
}

/**
 * @tc.name: TLVBufferSkipTest002
 * @tc.desc: test TLVBuffer Skip with exact length
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferSkipTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest002 start");
    TLVBuffer buffer(100);
    bool result = buffer.Skip(100);
    EXPECT_TRUE(result);
    EXPECT_FALSE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest002 end");
}

/**
 * @tc.name: TLVBufferSkipTest003
 * @tc.desc: test TLVBuffer Skip with overflow length
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferSkipTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest003 start");
    TLVBuffer buffer(100);
    bool result = buffer.Skip(101);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest003 end");
}

/**
 * @tc.name: TLVBufferSkipTest004
 * @tc.desc: test TLVBuffer Skip with zero length
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferSkipTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest004 start");
    TLVBuffer buffer(100);
    bool result = buffer.Skip(0);
    EXPECT_TRUE(result);
    EXPECT_TRUE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest004 end");
}

/**
 * @tc.name: TLVBufferSkipTest005
 * @tc.desc: test TLVBuffer multiple Skips
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferSkipTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest005 start");
    TLVBuffer buffer(100);
    EXPECT_TRUE(buffer.Skip(30));
    EXPECT_TRUE(buffer.Skip(30));
    EXPECT_TRUE(buffer.Skip(30));
    EXPECT_FALSE(buffer.Skip(11));
    EXPECT_TRUE(buffer.Skip(10));
    EXPECT_FALSE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferSkipTest005 end");
}

/**
 * @tc.name: TLVBufferIsEnoughTest001
 * @tc.desc: test TLVBuffer IsEnough when cursor < total
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferIsEnoughTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest001 start");
    TLVBuffer buffer(100);
    buffer.Skip(99);
    EXPECT_TRUE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest001 end");
}

/**
 * @tc.name: TLVBufferIsEnoughTest002
 * @tc.desc: test TLVBuffer IsEnough when cursor == total
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferIsEnoughTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest002 start");
    TLVBuffer buffer(100);
    buffer.Skip(100);
    EXPECT_FALSE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest002 end");
}

/**
 * @tc.name: TLVBufferIsEnoughTest003
 * @tc.desc: test TLVBuffer IsEnough when cursor > total
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferIsEnoughTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest003 start");
    TestTLVBuffer buffer;
    buffer.SetTotal(100);
    buffer.SetCursor(101);
    EXPECT_FALSE(buffer.IsEnough());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferIsEnoughTest003 end");
}

/**
 * @tc.name: TLVBufferHasExpectBufferTest001
 * @tc.desc: test TLVBuffer HasExpectBuffer with valid expectLen
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferHasExpectBufferTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest001 start");
    TLVBuffer buffer(100);
    EXPECT_TRUE(buffer.HasExpectBuffer(50));
    EXPECT_TRUE(buffer.HasExpectBuffer(100));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest001 end");
}

/**
 * @tc.name: TLVBufferHasExpectBufferTest002
 * @tc.desc: test TLVBuffer HasExpectBuffer with overflow expectLen
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferHasExpectBufferTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest002 start");
    TLVBuffer buffer(100);
    EXPECT_FALSE(buffer.HasExpectBuffer(101));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest002 end");
}

/**
 * @tc.name: TLVBufferHasExpectBufferTest003
 * @tc.desc: test TLVBuffer HasExpectBuffer with zero expectLen
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferHasExpectBufferTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest003 start");
    TLVBuffer buffer(100);
    EXPECT_TRUE(buffer.HasExpectBuffer(0));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest003 end");
}

/**
 * @tc.name: TLVBufferHasExpectBufferTest004
 * @tc.desc: test TLVBuffer HasExpectBuffer after Skip
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferHasExpectBufferTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest004 start");
    TLVBuffer buffer(100);
    buffer.Skip(50);
    EXPECT_TRUE(buffer.HasExpectBuffer(50));
    EXPECT_FALSE(buffer.HasExpectBuffer(51));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest004 end");
}

/**
 * @tc.name: TLVBufferHasExpectBufferTest005
 * @tc.desc: test TLVBuffer HasExpectBuffer with large expectLen
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVBufferHasExpectBufferTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest005 start");
    TLVBuffer buffer(UINT32_MAX - 1);
    EXPECT_TRUE(buffer.HasExpectBuffer(UINT32_MAX - 1));
    EXPECT_FALSE(buffer.HasExpectBuffer(UINT32_MAX));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVBufferHasExpectBufferTest005 end");
}

/**
 * @tc.name: TLVHeadStructTest001
 * @tc.desc: test TLVHead struct size
 * @tc.type: FUNC
 */
HWTEST_F(TLVBufferTest, TLVHeadStructTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVHeadStructTest001 start");
    EXPECT_EQ(sizeof(TLVHead), sizeof(uint16_t) + sizeof(uint32_t));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "TLVHeadStructTest001 end");
}