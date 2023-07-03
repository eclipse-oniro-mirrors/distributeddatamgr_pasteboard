/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hiview_adapter.h"
#include "pasteboard_behaviour_reporter_impl.h"
#include "pasteboard_fault_impl.h"
#include "pasteboard_hilog.h"
#include "reporter.h"
#include "time_consuming_statistic_impl.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
class DFXTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DFXTest::SetUpTestCase(void)
{
}

void DFXTest::TearDownTestCase(void)
{
}

void DFXTest::SetUp(void)
{
    HiViewAdapter::StartTimerThread();
}

void DFXTest::TearDown(void)
{
}

/**
* @tc.name: DFXTest001
* @tc.desc: test dfx report fault.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DFXTest, DFXTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    PasteboardFaultMsg faultMsg = { .userId = 1, .errorCode = "error" };
    auto status = Reporter::GetInstance().PasteboardFault().Report(faultMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
* @tc.name: DFXTest002
* @tc.desc: test dfx report time consuming statistic with SPS_COPY_STATE.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DFXTest, DFXTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    TimeConsumingStat timeConsumingStat = { .pasteboardState = SPS_COPY_STATE, .dataSize = -1, .timeConsuming = -1 };
    auto status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    timeConsumingStat = { .pasteboardState = SPS_COPY_STATE, .dataSize = DATA_LEVEL_ONE, .timeConsuming = -1 };
    status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    timeConsumingStat = {
        .pasteboardState = SPS_COPY_STATE, .dataSize = DATA_LEVEL_ONE, .timeConsuming = TCS_TIME_CONSUMING_LEVEL_ONE
    };
    status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
* @tc.name: DFXTest003
* @tc.desc: test dfx report time consuming statistic with SPS_PASTE_STATE.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DFXTest, DFXTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    TimeConsumingStat timeConsumingStat = { .pasteboardState = SPS_PASTE_STATE, .dataSize = -1, .timeConsuming = -1 };
    auto status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    timeConsumingStat = { .pasteboardState = SPS_PASTE_STATE, .dataSize = DATA_LEVEL_ONE, .timeConsuming = -1 };
    status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    timeConsumingStat = {
        .pasteboardState = SPS_PASTE_STATE, .dataSize = DATA_LEVEL_ONE, .timeConsuming = TCS_TIME_CONSUMING_LEVEL_ONE
    };
    status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
* @tc.name: DFXTest004
* @tc.desc: test dfx report time consuming statistic with SPS_REMOTE_PASTE_STATE.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DFXTest, DFXTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    TimeConsumingStat timeConsumingStat = {
        .pasteboardState = SPS_REMOTE_PASTE_STATE, .dataSize = -1, .timeConsuming = -1
    };
    auto status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    timeConsumingStat = { .pasteboardState = SPS_REMOTE_PASTE_STATE, .dataSize = DATA_LEVEL_ONE, .timeConsuming = -1 };
    status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    timeConsumingStat = { .pasteboardState = SPS_REMOTE_PASTE_STATE,
        .dataSize = DATA_LEVEL_ONE,
        .timeConsuming = TCS_TIME_CONSUMING_LEVEL_ONE };
    status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
* @tc.name: DFXTest005
* @tc.desc: test dfx report time consuming statistic with SPS_INVALID_STATE.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DFXTest, DFXTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    TimeConsumingStat timeConsumingStat = {
        .pasteboardState = SPS_INVALID_STATE, .dataSize = DATA_LEVEL_ONE, .timeConsuming = TCS_TIME_CONSUMING_LEVEL_ONE
    };
    auto status = Reporter::GetInstance().TimeConsumingStatistic().Report(timeConsumingStat);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}

/**
* @tc.name: DFXTest006
* @tc.desc: test dfx report behaviour.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(DFXTest, DFXTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "Start.");
    PasteboardBehaviourMsg behaviourMsg = { .pasteboardState = BPS_COPY_STATE, .bundleName = "com.paste.test" };
    auto status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    behaviourMsg = { .pasteboardState = BPS_PASTE_STATE, .bundleName = "com.paste.test" };
    status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    behaviourMsg = { .pasteboardState = BPS_REMOTE_PASTE_STATE, .bundleName = "com.paste.test" };
    status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);

    behaviourMsg = { .pasteboardState = BPS_INVALID_STATE, .bundleName = "com.paste.test" };
    status = Reporter::GetInstance().PasteboardBehaviour().Report(behaviourMsg);
    ASSERT_EQ(status, ReportStatus::SUCCESS);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_INNERKIT, "End.");
}
} // namespace OHOS::MiscServices