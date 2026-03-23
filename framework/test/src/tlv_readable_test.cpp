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
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    void *value = nullptr;
    TLVHead head;
    bool res = buff.ReadValue(value, head);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: ReadValueTest002
 * @tc.desc: ReadValue
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueTest002, TestSize.Level1)
{
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    head.len = 0;
    bool value = true;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
}

/**
 * @tc.name: ReadValueTest003
 * @tc.desc: ReadValue
 * @tc.type: FUNC
 */
HWTEST_F(TLVReadableTest, ReadValueTest003, TestSize.Level1)
{
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    TLVHead head;
    head.len = 0;
    int32_t value = 10;
    bool res = buff.ReadValue(value, head);
    EXPECT_FALSE(res);
}
