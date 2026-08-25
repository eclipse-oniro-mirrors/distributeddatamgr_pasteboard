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

// Host-only unit test for OHOS::MiscServices::Command (services/dfx/command.cpp).
// Pure CLI-argument helper: depends only on command.h + gtest. No shim, no fake.

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "command.h"

using namespace testing::ext;

namespace OHOS::MiscServices {

class CommandHostTest : public testing::Test {};

/**
 * @tc.name: ShowHelpReturnsHelp
 * @tc.desc: ShowHelp returns the help string given at construction.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, ShowHelpReturnsHelp, TestSize.Level0)
{
    Command cmd({"--dump"}, "dump help text");
    EXPECT_EQ(cmd.ShowHelp(), "dump help text");
}

/**
 * @tc.name: GetOptionReturnsFirstSegment
 * @tc.desc: GetOption returns the first format segment.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, GetOptionReturnsFirstSegment, TestSize.Level0)
{
    Command cmd({"--dump", "<arg>"}, "help");
    EXPECT_EQ(cmd.GetOption(), "--dump");
}

/**
 * @tc.name: GetFormatJoinsSegments
 * @tc.desc: GetFormat joins the format segments with spaces (trailing space).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, GetFormatJoinsSegments, TestSize.Level0)
{
    Command cmd({"--set", "<key>", "<value>"}, "help");
    EXPECT_EQ(cmd.GetFormat(), "--set <key> <value> ");
}

/**
 * @tc.name: GetFormatSingleSegment
 * @tc.desc: GetFormat of a single-segment command.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, GetFormatSingleSegment, TestSize.Level0)
{
    Command cmd({"--all"}, "help");
    EXPECT_EQ(cmd.GetFormat(), "--all ");
}

/**
 * @tc.name: DoActionInvokesBoundAction
 * @tc.desc: DoAction invokes the bound action and passes input/output through.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, DoActionInvokesBoundAction, TestSize.Level0)
{
    bool called = false;
    Command cmd({"--echo"}, "help",
        [&called](const std::vector<std::string> &input, std::string &output) {
            called = true;
            output = input.empty() ? "" : input[0];
            return true;
        });

    std::string out;
    EXPECT_TRUE(cmd.DoAction({"hello"}, out));
    EXPECT_TRUE(called);
    EXPECT_EQ(out, "hello");
}

/**
 * @tc.name: DoActionPropagatesFalse
 * @tc.desc: An action returning false is propagated.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, DoActionPropagatesFalse, TestSize.Level0)
{
    Command cmd({"--fail"}, "help",
        [](const std::vector<std::string> &, std::string &) { return false; });
    std::string out;
    EXPECT_FALSE(cmd.DoAction({}, out));
}

/**
 * @tc.name: IndependentInstances
 * @tc.desc: Distinct commands keep independent help/format state.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(CommandHostTest, IndependentInstances, TestSize.Level0)
{
    Command a({"--a"}, "helpA");
    Command b({"--b", "<x>"}, "helpB");
    EXPECT_EQ(a.GetOption(), "--a");
    EXPECT_EQ(b.GetOption(), "--b");
    EXPECT_EQ(a.ShowHelp(), "helpA");
    EXPECT_EQ(b.ShowHelp(), "helpB");
}
} // namespace OHOS::MiscServices
