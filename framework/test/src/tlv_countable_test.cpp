/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <variant>

#include "tlv_countable.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

class TLVCountableTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void TLVCountableTest::SetUpTestCase(void) { }

void TLVCountableTest::TearDownTestCase(void) { }

void TLVCountableTest::SetUp(void) { }

void TLVCountableTest::TearDown(void) { }

/**
 * @tc.name: CountVariantBranchTest001
 * @tc.desc: Test CountVariant when input.index() == 0 (branch true)
 * @tc.type: FUNC
 */
HWTEST_F(TLVCountableTest, CountVariantBranchTest001, TestSize.Level1)
{
    using TestVariant = std::variant<std::monostate, int, std::string>;
    TestVariant testVariant;
    
    size_t result = TLVCountable::CountVariant<TestVariant, std::monostate, int, std::string>(0, testVariant);
    EXPECT_EQ(result, 16);
}

/**
 * @tc.name: CountVariantBranchTest002
 * @tc.desc: Test CountVariant when input.index() < step (branch true)
 * @tc.type: FUNC
 */
HWTEST_F(TLVCountableTest, CountVariantBranchTest002, TestSize.Level1)
{
    using TestVariant = std::variant<std::monostate, int, std::string>;
    TestVariant testVariant = 1;
    
    size_t result = TLVCountable::CountVariant<TestVariant, std::monostate, int, std::string>(2, testVariant);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: CountVariantBranchTest003
 * @tc.desc: Test CountVariant when step == std::variant_npos (branch true)
 * @tc.type: FUNC
 */
HWTEST_F(TLVCountableTest, CountVariantBranchTest003, TestSize.Level1)
{
    using TestVariant = std::variant<std::monostate, int, std::string>;
    TestVariant testVariant;
    
    size_t result = TLVCountable::CountVariant<TestVariant, std::monostate, int, std::string>(
        static_cast<uint32_t>(std::variant_npos), testVariant);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: CountVariantBranchTest004
 * @tc.desc: Test CountVariant when step != input.index() (recursive branch)
 * @tc.type: FUNC
 */
HWTEST_F(TLVCountableTest, CountVariantBranchTest004, TestSize.Level1)
{
    using TestVariant = std::variant<std::monostate, int, std::string>;
    TestVariant testVariant = std::string("Hello");
    
    size_t result = TLVCountable::CountVariant<TestVariant, std::monostate, int, std::string>(0, testVariant);
    size_t expected = TLVCountable::Count(static_cast<uint32_t>(2)) +
        TLVCountable::Count(std::get<std::string>(testVariant));
    EXPECT_EQ(result, expected);
}
} // namespace OHOS::MiscServices