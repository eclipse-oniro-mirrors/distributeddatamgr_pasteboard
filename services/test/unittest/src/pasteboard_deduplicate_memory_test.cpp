/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <thread>

#include "pasteboard_deduplicate_memory.h"
#include "pasteboard_error.h"

namespace OHOS {
namespace MiscServices {
using namespace testing::ext;

class PasteboardDeduplicateMemoryTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardDeduplicateMemoryTest::SetUpTestCase(void) { }

void PasteboardDeduplicateMemoryTest::TearDownTestCase(void) { }

void PasteboardDeduplicateMemoryTest::SetUp(void) { }

void PasteboardDeduplicateMemoryTest::TearDown(void) { }

struct RadarReportIdentity {
    pid_t pid;
    PasteboardError errorCode;
};

bool operator==(const RadarReportIdentity &lhs, const RadarReportIdentity &rhs)
{
    return lhs.pid == rhs.pid && lhs.errorCode == rhs.errorCode;
}

/**
 * @tc.name: TestIsDuplicate001
 * @tc.desc: should return false when first called IsDuplicate
 *           should return true when called IsDuplicate with same params within expirationMilliSeconds
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDeduplicateMemoryTest, TestIsDuplicate001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate001 start");
    int64_t expirationMilliSeconds = 1000;
    DeduplicateMemory<RadarReportIdentity> reportMemory(expirationMilliSeconds);

    bool isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);

    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_TRUE(isDuplicate);

    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_TRUE(isDuplicate);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate001 end");
}

/**
 * @tc.name: TestIsDuplicate002
 * @tc.desc: should return false when first called IsDuplicate
 *           should return false when called IsDuplicate with same params after expirationMilliSeconds
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDeduplicateMemoryTest, TestIsDuplicate002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate002 end");
    int64_t expirationMilliSeconds = 900;
    DeduplicateMemory<RadarReportIdentity> reportMemory(expirationMilliSeconds);

    bool isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate002 end");
}

/**
 * @tc.name: TestIsDuplicate003
 * @tc.desc: should return false when first called IsDuplicate
 *           should return true when called IsDuplicate with same params within expirationMilliSeconds
 *           should return false when called IsDuplicate with same params after expirationMilliSeconds
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDeduplicateMemoryTest, TestIsDuplicate003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate003 end");
    int64_t expirationMilliSeconds = 1100;
    DeduplicateMemory<RadarReportIdentity> reportMemory(expirationMilliSeconds);

    bool isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_TRUE(isDuplicate);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate003 end");
}

/**
 * @tc.name: TestIsDuplicate004
 * @tc.desc: should return false when first called IsDuplicate
 *           should return true when called IsDuplicate with same params within expirationMilliSeconds
 *           should return false when called IsDuplicate with different params within expirationMilliSeconds
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDeduplicateMemoryTest, TestIsDuplicate004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate004 start");
    int64_t expirationMilliSeconds = 1100;
    DeduplicateMemory<RadarReportIdentity> reportMemory(expirationMilliSeconds);

    bool isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_FALSE(isDuplicate);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_PARAM_ERROR });
    EXPECT_TRUE(isDuplicate);

    isDuplicate = reportMemory.IsDuplicate({.pid = 1, .errorCode = PasteboardError::INVALID_DATA_ERROR });
    EXPECT_FALSE(isDuplicate);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TestIsDuplicate004 end");
}
} // namespace MiscServices
} // namespace OHOS
