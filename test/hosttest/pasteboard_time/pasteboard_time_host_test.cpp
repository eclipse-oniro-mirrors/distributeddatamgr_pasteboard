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

// Host-only unit test for OHOS::MiscServices::PasteBoardTime.
// Pure time logic over POSIX clocks; depends only on pasteboard_time.cpp + the
// c_utils include path (for the header's singleton.h include) + gtest. No
// device, no IPC, no service.

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <thread>

#include <gtest/gtest.h>

#include "pasteboard_time.h"

using namespace testing::ext;

namespace OHOS::MiscServices {
namespace {
// Allowed skew between PasteBoardTime and std::chrono, in milliseconds.
constexpr int64_t MAX_CLOCK_SKEW_MS = 5000;
// Short sleep used to prove a clock advances, in milliseconds.
constexpr int SLEEP_MS = 10;
} // namespace

class PasteBoardTimeHostTest : public testing::Test {};

/**
 * @tc.name: GetCurrentTimeMicrosIsSane
 * @tc.desc: Current time (ms since epoch) is non-zero and near the system clock.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(PasteBoardTimeHostTest, GetCurrentTimeMicrosIsSane, TestSize.Level0)
{
    uint64_t t = PasteBoardTime::GetCurrentTimeMicros();
    EXPECT_GT(t, 0u);

    // The function returns tv_sec*1000 + tv_usec/1000 (milliseconds since
    // epoch). Cross-check it is within a few seconds of std::chrono's now().
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t diff = static_cast<int64_t>(nowMs) - static_cast<int64_t>(t);
    EXPECT_LT(std::llabs(diff), MAX_CLOCK_SKEW_MS);
}

/**
 * @tc.name: GetBootTimeMsIsMonotonic
 * @tc.desc: Boot time is non-negative and monotonically non-decreasing.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(PasteBoardTimeHostTest, GetBootTimeMsIsMonotonic, TestSize.Level0)
{
    int64_t a = PasteBoardTime::GetBootTimeMs();
    EXPECT_GE(a, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    int64_t b = PasteBoardTime::GetBootTimeMs();
    EXPECT_GE(b, a);
}

/**
 * @tc.name: GetWallTimeMsAdvances
 * @tc.desc: Wall time is positive and advances across a short sleep.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(PasteBoardTimeHostTest, GetWallTimeMsAdvances, TestSize.Level0)
{
    int64_t a = PasteBoardTime::GetWallTimeMs();
    EXPECT_GT(a, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
    int64_t b = PasteBoardTime::GetWallTimeMs();
    EXPECT_GE(b, a);
}

/**
 * @tc.name: WallAndBootAreIndependentlyValid
 * @tc.desc: Wall time (since epoch) is far larger than boot time (since boot).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(PasteBoardTimeHostTest, WallAndBootAreIndependentlyValid, TestSize.Level0)
{
    int64_t wall = PasteBoardTime::GetWallTimeMs();
    int64_t boot = PasteBoardTime::GetBootTimeMs();
    EXPECT_GT(wall, boot);
}
} // namespace OHOS::MiscServices
