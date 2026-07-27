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

// Host-only unit test for OHOS::MiscServices::PasteboardDumpHelper
// (services/dfx/pasteboard_dump_helper.cpp).
//
// Composition sample: builds on the already-host-tested Command (links the real
// command.cpp). Dump() writes to a file descriptor via dprintf, so the tests
// capture output through a private temp file and read it back.

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include <unistd.h>

#include <gtest/gtest.h>

#include "pasteboard_dump_helper.h"

using namespace testing::ext;

namespace OHOS::MiscServices {

class DumpHelperHostTest : public testing::Test {
protected:
    // Run helper.Dump(fd, args) against a temp file and return (retval, output).
    // Uses mkstemp (not tmpfile) so the file is created with a private,
    // race-free name; it is unlinked immediately, so the fd is the only handle
    // and nothing is left on disk even if the test aborts.
    static std::pair<bool, std::string> RunDump(
        PasteboardDumpHelper &helper, const std::vector<std::string> &args)
    {
        char path[] = "/tmp/pasteboard_dump_helper_XXXXXX";
        int fd = mkstemp(path);
        EXPECT_GE(fd, 0);
        if (fd < 0) {
            return { false, "" };
        }
        EXPECT_EQ(unlink(path), 0);

        bool ret = helper.Dump(fd, args);

        // dprintf writes straight through the fd, so there is no buffer to
        // flush; just rewind and read back whatever Dump produced.
        std::string out;
        off_t size = lseek(fd, 0, SEEK_END);
        if (size > 0) {
            EXPECT_EQ(lseek(fd, 0, SEEK_SET), 0);
            out.resize(static_cast<size_t>(size));
            ssize_t rd = read(fd, out.data(), out.size());
            out.resize(rd > 0 ? static_cast<size_t>(rd) : 0);
        }
        EXPECT_EQ(close(fd), 0);
        return { ret, out };
    }
};

/**
 * @tc.name: DumpNoArgsShowsHelpReturnsFalse
 * @tc.desc: Dump with no args prints the help header and returns false.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(DumpHelperHostTest, DumpNoArgsShowsHelpReturnsFalse, TestSize.Level0)
{
    PasteboardDumpHelper helper;
    auto [ret, out] = RunDump(helper, {});
    EXPECT_FALSE(ret);
    EXPECT_NE(out.find("---"), std::string::npos); // the separator line
}

/**
 * @tc.name: DumpHelpFlagShowsHelp
 * @tc.desc: Dump with "-h" behaves like no args (help, false).
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(DumpHelperHostTest, DumpHelpFlagShowsHelp, TestSize.Level0)
{
    PasteboardDumpHelper helper;
    auto [ret, out] = RunDump(helper, {"-h"});
    EXPECT_FALSE(ret);
    EXPECT_FALSE(out.empty());
}

/**
 * @tc.name: DumpDispatchesRegisteredCommand
 * @tc.desc: A registered command is dispatched by its option; its output is written.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(DumpHelperHostTest, DumpDispatchesRegisteredCommand, TestSize.Level0)
{
    PasteboardDumpHelper helper;
    auto cmd = std::make_shared<Command>(
        std::vector<std::string>{"--data"}, "dump data",
        [](const std::vector<std::string> &input, std::string &output) {
            output = "DATA_OUTPUT";
            return true;
        });
    helper.RegisterCommand(cmd);

    auto [ret, out] = RunDump(helper, {"--data"});
    EXPECT_TRUE(ret);
    EXPECT_NE(out.find("DATA_OUTPUT"), std::string::npos);
}

/**
 * @tc.name: DumpUnknownOptionReturnsFalse
 * @tc.desc: An unknown option (no matching command) returns false.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(DumpHelperHostTest, DumpUnknownOptionReturnsFalse, TestSize.Level0)
{
    PasteboardDumpHelper helper;
    auto cmd = std::make_shared<Command>(
        std::vector<std::string>{"--known"}, "help",
        [](const std::vector<std::string> &, std::string &output) {
            output = "x";
            return true;
        });
    helper.RegisterCommand(cmd);

    auto [ret, out] = RunDump(helper, {"--unknown"});
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: HelpListingIncludesRegisteredCommand
 * @tc.desc: The help listing includes a registered command's format + help text.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(DumpHelperHostTest, HelpListingIncludesRegisteredCommand, TestSize.Level0)
{
    PasteboardDumpHelper helper;
    auto cmd = std::make_shared<Command>(
        std::vector<std::string>{"--foo", "<bar>"}, "foo help text",
        [](const std::vector<std::string> &, std::string &) { return true; });
    helper.RegisterCommand(cmd);

    auto [ret, out] = RunDump(helper, {}); // no args -> help listing
    EXPECT_FALSE(ret);
    EXPECT_NE(out.find("--foo"), std::string::npos);
    EXPECT_NE(out.find("foo help text"), std::string::npos);
}

/**
 * @tc.name: GetInstanceIsStable
 * @tc.desc: GetInstance returns a stable singleton reference.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(DumpHelperHostTest, GetInstanceIsStable, TestSize.Level0)
{
    PasteboardDumpHelper &a = PasteboardDumpHelper::GetInstance();
    PasteboardDumpHelper &b = PasteboardDumpHelper::GetInstance();
    EXPECT_EQ(&a, &b);
}
} // namespace OHOS::MiscServices
