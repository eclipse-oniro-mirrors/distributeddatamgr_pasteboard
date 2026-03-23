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

#include "pasteboard_time.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

class PasteboardTimeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardTimeTest::SetUpTestCase(void) { }

void PasteboardTimeTest::TearDownTestCase(void) { }

void PasteboardTimeTest::SetUp(void) { }

void PasteboardTimeTest::TearDown(void) { }

/**
 * @tc.name: GetCurrentTimeMicros
 * @tc.desc: Get current time.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardTimeTest, GetCurrentTimeMicros, TestSize.Level0)
{
    auto result = PasteBoardTime::GetCurrentTimeMicros();
    EXPECT_TRUE(result > 0);
}

/**
 * @tc.name: GetBootTimeMs
 * @tc.desc: Get boot time.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardTimeTest, GetBootTimeMs, TestSize.Level0)
{
    auto result = PasteBoardTime::GetBootTimeMs();
    EXPECT_TRUE(result >= 0);
}

/**
 * @tc.name: GetWallTimeMs
 * @tc.desc: Get wall time.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardTimeTest, GetWallTimeMs, TestSize.Level0)
{
    auto result = PasteBoardTime::GetWallTimeMs();
    EXPECT_TRUE(result >= 0);
}

/**
 * @tc.name: GetWallTimeMsMultipleCalls
 * @tc.desc: Get wall time multiple times.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardTimeTest, GetWallTimeMsMultipleCalls, TestSize.Level1)
{
    auto result1 = PasteBoardTime::GetWallTimeMs();
    auto result2 = PasteBoardTime::GetWallTimeMs();
    EXPECT_TRUE(result2 >= result1);
}

/**
 * @tc.name: GetBootTimeMsMultipleCalls
 * @tc.desc: Get boot time multiple times.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardTimeTest, GetBootTimeMsMultipleCalls, TestSize.Level1)
{
    auto result1 = PasteBoardTime::GetBootTimeMs();
    auto result2 = PasteBoardTime::GetBootTimeMs();
    EXPECT_TRUE(result2 >= result1);
}

/**
 * @tc.name: GetCurrentTimeMicrosMultipleCalls
 * @tc.desc: Get current time micros multiple times.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardTimeTest, GetCurrentTimeMicrosMultipleCalls, TestSize.Level1)
{
    auto result1 = PasteBoardTime::GetCurrentTimeMicros();
    auto result2 = PasteBoardTime::GetCurrentTimeMicros();
    EXPECT_TRUE(result2 >= result1);
}
} // namespace OHOS::MiscServices
