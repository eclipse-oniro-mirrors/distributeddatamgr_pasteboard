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

#include "common/pasteboard_common_utils.h"

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
    PasteBoardCommonUtils::SetThreadTaskName(thread, "TestThreadName");
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
    PasteBoardCommonUtils::SetThreadTaskName(thread, "");
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
        PasteBoardCommonUtils::SetTaskName("TestCurrentThreadName");
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
        PasteBoardCommonUtils::SetTaskName("");
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
        PasteBoardCommonUtils::SetTaskName("ThreadTaskName");
        nameSet = true;
    });
    thread.join();
    EXPECT_TRUE(nameSet);
}

} // namespace OHOS::MiscServices
