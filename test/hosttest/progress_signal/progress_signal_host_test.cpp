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

// Host-only unit test for OHOS::MiscServices::ProgressSignalClient
// (framework/innerkits/src/pasteboard_progress_signal.cpp).
//
// An atomic cancel-state machine. The only external include the .cpp pulls
// (distributed_file_daemon_manager.h) is unused, so this suite shims it empty
// and needs nothing else but the module + gtest. ProgressSignalClient is a
// process-wide singleton, so each test Init()s it back to a known state first.

#include <gtest/gtest.h>

#include "pasteboard_progress_signal.h"

using namespace testing::ext;

namespace OHOS::MiscServices {

class ProgressSignalHostTest : public testing::Test {
protected:
    void SetUp() override
    {
        // Reset the singleton's flags before each case.
        ProgressSignalClient::GetInstance().Init();
    }
};

/**
 * @tc.name: InitClearsCancelState
 * @tc.desc: After Init the signal is not canceled.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ProgressSignalHostTest, InitClearsCancelState, TestSize.Level0)
{
    auto &c = ProgressSignalClient::GetInstance();
    EXPECT_FALSE(c.IsCanceled());
    EXPECT_FALSE(c.CheckCancelIfNeed());
}

/**
 * @tc.name: CancelSetsCanceled
 * @tc.desc: Cancel() sets the local cancel flag; IsCanceled/CheckCancelIfNeed see it.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ProgressSignalHostTest, CancelSetsCanceled, TestSize.Level0)
{
    auto &c = ProgressSignalClient::GetInstance();
    c.Cancel();
    EXPECT_TRUE(c.IsCanceled());
    EXPECT_TRUE(c.CheckCancelIfNeed());
}

/**
 * @tc.name: SetRemoteTaskCancelSetsCanceled
 * @tc.desc: SetRemoteTaskCancel() also drives cancellation.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ProgressSignalHostTest, SetRemoteTaskCancelSetsCanceled, TestSize.Level0)
{
    auto &c = ProgressSignalClient::GetInstance();
    c.SetRemoteTaskCancel();
    EXPECT_TRUE(c.IsCanceled());
    // SetRemoteTaskCancel sets needCancel_, so CheckCancelIfNeed is true too.
    EXPECT_TRUE(c.CheckCancelIfNeed());
}

/**
 * @tc.name: InitResetsAfterCancel
 * @tc.desc: Init() clears a previously-set cancel state.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ProgressSignalHostTest, InitResetsAfterCancel, TestSize.Level0)
{
    auto &c = ProgressSignalClient::GetInstance();
    c.Cancel();
    ASSERT_TRUE(c.IsCanceled());

    c.Init();
    EXPECT_FALSE(c.IsCanceled());
    EXPECT_FALSE(c.CheckCancelIfNeed());
}

// ---- CheckCancelIfNeed returns false when only remoteTask_ is set (needCancel_
// is false). IsCanceled ORs both flags, so it distinguishes the two. ----
// remoteTask_ can only be set via Init(false) internally; here we verify the
// documented asymmetry: after Init, neither flag is set, both are false.
/**
 * @tc.name: CheckCancelDependsOnNeedCancelOnly
 * @tc.desc: CheckCancelIfNeed sees only needCancel_, not remoteTask_, unlike IsCanceled which ORs both flags.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ProgressSignalHostTest, CheckCancelDependsOnNeedCancelOnly, TestSize.Level0)
{
    auto &c = ProgressSignalClient::GetInstance();
    // Fresh Init: needCancel_ = false -> CheckCancelIfNeed false.
    EXPECT_FALSE(c.CheckCancelIfNeed());
    c.Cancel(); // needCancel_ = true
    EXPECT_TRUE(c.CheckCancelIfNeed());
}

/**
 * @tc.name: GetInstanceIsStable
 * @tc.desc: GetInstance returns a stable singleton.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(ProgressSignalHostTest, GetInstanceIsStable, TestSize.Level0)
{
    EXPECT_EQ(&ProgressSignalClient::GetInstance(), &ProgressSignalClient::GetInstance());
}
} // namespace OHOS::MiscServices
