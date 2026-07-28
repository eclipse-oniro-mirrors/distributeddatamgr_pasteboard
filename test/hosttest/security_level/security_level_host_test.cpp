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

// Host-only unit test for OHOS::MiscServices::SecurityLevel
// (adapter/security_level/security_level.cpp).
//
// Deep-dependency module: SecurityLevel sits behind the DEVSL C API
// (dataclassification) and DMAdapter (device_manager). This test builds against
// fakes under fakes/ that expose the DEVSL result/level and the local udid as
// test hooks, so every branch of the security-level logic can be driven without
// the real services. The private level getters are reached through the public
// IsSupportedDistributed(bool).

#include <cstdint>
#include <string>

#include <gtest/gtest.h>

#include "dev_slinfo_mgr.h" // fake
#include "security_level.h"

using namespace testing::ext;

// Definitions for the fake test hooks (declared extern in the fake headers).
namespace OHOS::MiscServices::hosttest_dm {
std::string g_udid = "test-udid-0123456789";
}
namespace hosttest_devsl {
int32_t g_result = DEVSL_SUCCESS;
uint32_t g_level = DATA_SEC_LEVEL0;
}

namespace OHOS::MiscServices {

class SecurityLevelHostTest : public testing::Test {
protected:
    void SetUp() override
    {
        // Reset hooks to a known baseline before each test.
        hosttest_dm::g_udid = "test-udid-0123456789";
        hosttest_devsl::g_result = DEVSL_SUCCESS;
        hosttest_devsl::g_level = DATA_SEC_LEVEL0;
    }
};

/**
 * @tc.name: HighLevelSupportsDistributed
 * @tc.desc: A device reporting level >= 3 supports distributed.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, HighLevelSupportsDistributed, TestSize.Level0)
{
    hosttest_devsl::g_result = DEVSL_SUCCESS;
    hosttest_devsl::g_level = DATA_SEC_LEVEL3;

    SecurityLevel sl;
    EXPECT_TRUE(sl.IsSupportedDistributed(false));
}

/**
 * @tc.name: AboveThresholdSupportsDistributed
 * @tc.desc: Level 4 (above the threshold) also supports distributed.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, AboveThresholdSupportsDistributed, TestSize.Level0)
{
    hosttest_devsl::g_level = DATA_SEC_LEVEL4;

    SecurityLevel sl;
    EXPECT_TRUE(sl.IsSupportedDistributed(false));
}

/**
 * @tc.name: LowLevelDoesNotSupportDistributed
 * @tc.desc: A device below level 3 does NOT support distributed.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, LowLevelDoesNotSupportDistributed, TestSize.Level0)
{
    hosttest_devsl::g_level = DATA_SEC_LEVEL2;

    SecurityLevel sl;
    EXPECT_FALSE(sl.IsSupportedDistributed(false));
}

/**
 * @tc.name: LowLevelWithLoggingReturnsFalse
 * @tc.desc: NeedLog=true path (below threshold) still returns false.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, LowLevelWithLoggingReturnsFalse, TestSize.Level0)
{
    hosttest_devsl::g_level = DATA_SEC_LEVEL1;

    SecurityLevel sl;
    EXPECT_FALSE(sl.IsSupportedDistributed(true));
}

/**
 * @tc.name: DevslFailureTreatedAsLevelZero
 * @tc.desc: When DEVSL query fails, level stays 0 -> not supported.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, DevslFailureTreatedAsLevelZero, TestSize.Level0)
{
    hosttest_devsl::g_result = DEVSL_ERROR;
    hosttest_devsl::g_level = DATA_SEC_LEVEL4; // ignored because result != SUCCESS

    SecurityLevel sl;
    EXPECT_FALSE(sl.IsSupportedDistributed(false));
}

/**
 * @tc.name: EmptyUdidFailsInitReturnsUnsupported
 * @tc.desc: An empty udid makes InitDEVSLQueryParams fail -> level 0 -> not supported.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, EmptyUdidFailsInitReturnsUnsupported, TestSize.Level0)
{
    hosttest_dm::g_udid = "";       // InitDEVSLQueryParams rejects empty udid
    hosttest_devsl::g_level = DATA_SEC_LEVEL4;

    SecurityLevel sl;
    EXPECT_FALSE(sl.IsSupportedDistributed(false));
}

// ---- The cached level short-circuits a second query: once a high level is
// observed, GetDeviceSecurityLevel returns it without re-querying DEVSL ----
/**
 * @tc.name: CachedLevelShortCircuits
 * @tc.desc: A cached high level short-circuits the second query, returned without re-querying DEVSL.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(SecurityLevelHostTest, CachedLevelShortCircuits, TestSize.Level0)
{
    hosttest_devsl::g_level = DATA_SEC_LEVEL3;
    SecurityLevel sl;
    EXPECT_TRUE(sl.IsSupportedDistributed(false)); // caches level 3

    // Now make a fresh query fail; the cached level should still be used.
    hosttest_devsl::g_result = DEVSL_ERROR;
    hosttest_devsl::g_level = DATA_SEC_LEVEL0;
    EXPECT_TRUE(sl.IsSupportedDistributed(false));
}
} // namespace OHOS::MiscServices
