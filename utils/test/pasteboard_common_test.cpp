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
#include <thread>
#include <chrono>

#include "pasteboard_common.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

constexpr int32_t THREAD_SLEEP_MS = 100;

class PasteboardCommonTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardCommonTest::SetUpTestCase(void) { }

void PasteboardCommonTest::TearDownTestCase(void) { }

void PasteboardCommonTest::SetUp(void) { }

void PasteboardCommonTest::TearDown(void) { }

/**
 * @tc.name: IsPasteboardService
 * @tc.desc: Check whether this process is pasteboard service or not.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, IsPasteboardService, TestSize.Level0)
{
    auto result = PasteBoardCommon::GetInstance().IsPasteboardService();
    EXPECT_FALSE(result);
}

/**
 * @tc.name: GetAppBundleManager
 * @tc.desc: Get app bundle manager.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, GetAppBundleManager, TestSize.Level0)
{
    auto result = PasteBoardCommon::GetInstance().GetAppBundleManager();
    EXPECT_TRUE(result != nullptr);
}

/**
 * @tc.name: GetApiTargetVersionForSelf
 * @tc.desc: Get api target version for this process.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, GetApiTargetVersionForSelf, TestSize.Level0)
{
    auto result = PasteBoardCommon::GetInstance().GetApiTargetVersionForSelf();
    EXPECT_TRUE(result >= 0);
}

/**
 * @tc.name: GetAnonymousStringTest001
 * @tc.desc: GetAnonymousString.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardCommonTest, GetAnonymousStringTest001, TestSize.Level0)
{
    std::string testStr = "anon";
    std::string resultStr = PasteBoardCommon::GetAnonymousString(testStr);
    EXPECT_EQ(resultStr, testStr);
    testStr = "anon123456789";
    resultStr = PasteBoardCommon::GetAnonymousString(testStr);
    std::string anonStr = "anon**6789";
    EXPECT_EQ(resultStr, anonStr);
}

/**
 * @tc.name: SetThreadTaskNameTest001
 * @tc.desc: Test SetThreadTaskName function with valid name.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, SetThreadTaskNameTest001, TestSize.Level0)
{
    bool threadExecuted = false;
    std::thread thread([&threadExecuted]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        threadExecuted = true;
    });
    PasteBoardCommon::SetThreadTaskName(thread, "TestThreadName");
    thread.join();
    EXPECT_TRUE(threadExecuted);
}

/**
 * @tc.name: SetThreadTaskNameTest002
 * @tc.desc: Test SetThreadTaskName with empty name.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, SetThreadTaskNameTest002, TestSize.Level0)
{
    bool threadExecuted = false;
    std::thread thread([&threadExecuted]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_SLEEP_MS));
        threadExecuted = true;
    });
    PasteBoardCommon::SetThreadTaskName(thread, "");
    thread.join();
    EXPECT_TRUE(threadExecuted);
}

/**
 * @tc.name: SetTaskNameTest001
 * @tc.desc: Test SetTaskName function with valid name.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, SetTaskNameTest001, TestSize.Level0)
{
    bool nameSet = false;
    std::thread thread([&nameSet]() {
        PasteBoardCommon::SetTaskName("TestCurrentThreadName");
        nameSet = true;
    });
    thread.join();
    EXPECT_TRUE(nameSet);
}

/**
 * @tc.name: SetTaskNameTest002
 * @tc.desc: Test SetTaskName with empty name.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, SetTaskNameTest002, TestSize.Level0)
{
    bool nameSet = false;
    std::thread thread([&nameSet]() {
        PasteBoardCommon::SetTaskName("");
        nameSet = true;
    });
    thread.join();
    EXPECT_TRUE(nameSet);
}

/**
 * @tc.name: SetTaskNameInThreadTest001
 * @tc.desc: Test SetTaskName in a new thread.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardCommonTest, SetTaskNameInThreadTest001, TestSize.Level0)
{
    bool nameSet = false;
    std::thread thread([&nameSet]() {
        PasteBoardCommon::SetTaskName("ThreadTaskName");
        nameSet = true;
    });
    thread.join();
    EXPECT_TRUE(nameSet);
}


} // namespace OHOS::MiscServices
