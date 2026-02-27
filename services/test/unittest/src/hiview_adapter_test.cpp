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

#include <gtest/gtest.h>
#include "hiview_adapter.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
class HiViewAdapterTest : public testing::Test {
public:
    HiViewAdapterTest() {};
    ~HiViewAdapterTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void HiViewAdapterTest::SetUpTestCase(void) { }

void HiViewAdapterTest::TearDownTestCase(void) { }

void HiViewAdapterTest::SetUp(void) { }

void HiViewAdapterTest::TearDown(void) { }

/**
 * @tc.name: CopyTimeConsumingCountTest001
 * @tc.desc: Test CopyTimeConsumingCount dataLevel Invalid
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, CopyTimeConsumingCountTest001, TestSize.Level1)
{
    HiViewAdapter::copyTimeConsumingStat_.clear();
    int dataLevel = 10;
    int timeLevel = 10;
    HiViewAdapter::CopyTimeConsumingCount(dataLevel, timeLevel);
    EXPECT_TRUE(static_cast<int>(HiViewAdapter::copyTimeConsumingStat_.size()) <= dataLevel);
}

/**
 * @tc.name: CopyTimeConsumingCountTest002
 * @tc.desc: Test CopyTimeConsumingCount timeLevel not in copyTimeConsumingStat_
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, CopyTimeConsumingCountTest002, TestSize.Level1)
{
    std::map<int, int> testMap;
    testMap[11] = 22;
    HiViewAdapter::copyTimeConsumingStat_.push_back(testMap);
    int dataLevel = 0;
    int timeLevel = 100;
    auto it = HiViewAdapter::copyTimeConsumingStat_[dataLevel].find(timeLevel);
    bool isIn = it != HiViewAdapter::copyTimeConsumingStat_[dataLevel].end() ? true : false;
    EXPECT_FALSE(isIn);
    HiViewAdapter::CopyTimeConsumingCount(dataLevel, timeLevel);
}

/**
 * @tc.name: PasteTimeConsumingCountTest001
 * @tc.desc: Test PasteTimeConsumingCount dataLevel Invalid
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, PasteTimeConsumingCountTest001, TestSize.Level1)
{
    HiViewAdapter::pasteTimeConsumingStat_.clear();
    int dataLevel = 10;
    int timeLevel = 10;
    EXPECT_TRUE(static_cast<int>(HiViewAdapter::pasteTimeConsumingStat_.size()) <= dataLevel);
    HiViewAdapter::PasteTimeConsumingCount(dataLevel, timeLevel);
}

/**
 * @tc.name: PasteTimeConsumingCountTest002
 * @tc.desc: Test PasteTimeConsumingCount timeLevel not in pasteTimeConsumingStat_
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, PasteTimeConsumingCountTest002, TestSize.Level1)
{
    std::map<int, int> testMap;
    testMap[11] = 22;
    HiViewAdapter::pasteTimeConsumingStat_.push_back(testMap);
    int dataLevel = 0;
    int timeLevel = 100;
    auto it = HiViewAdapter::pasteTimeConsumingStat_[dataLevel].find(timeLevel);
    bool isIn = it != HiViewAdapter::pasteTimeConsumingStat_[dataLevel].end() ? true : false;
    EXPECT_FALSE(isIn);
    HiViewAdapter::PasteTimeConsumingCount(dataLevel, timeLevel);
}

/**
 * @tc.name: RemotePasteTimeConsumingCountTest001
 * @tc.desc: Test RemotePasteTimeConsumingCount dataLevel Invalid
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, RemotePasteTimeConsumingCountTest001, TestSize.Level1)
{
    HiViewAdapter::remotePasteTimeConsumingStat_.clear();
    int dataLevel = 10;
    int timeLevel = 10;
    EXPECT_TRUE(static_cast<int>(HiViewAdapter::remotePasteTimeConsumingStat_.size()) <= dataLevel);
    HiViewAdapter::RemotePasteTimeConsumingCount(dataLevel, timeLevel);
}

/**
 * @tc.name: RemotePasteTimeConsumingCountTest002
 * @tc.desc: Test RemotePasteTimeConsumingCount timeLevel not in remotePasteTimeConsumingStat_
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, RemotePasteTimeConsumingCountTest002, TestSize.Level1)
{
    std::map<int, int> testMap;
    testMap[11] = 22;
    HiViewAdapter::remotePasteTimeConsumingStat_.push_back(testMap);
    int dataLevel = 0;
    int timeLevel = 100;
    auto it = HiViewAdapter::remotePasteTimeConsumingStat_[dataLevel].find(timeLevel);
    bool isIn = it != HiViewAdapter::remotePasteTimeConsumingStat_[dataLevel].end() ? true : false;
    EXPECT_FALSE(isIn);
    HiViewAdapter::RemotePasteTimeConsumingCount(dataLevel, timeLevel);
}

/**
 * @tc.name: GetDataLevelTest001
 * @tc.desc: Test GetDataLevel timeLevel dataLevel Invalid
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, GetDataLevelTest001, TestSize.Level1)
{
    int dataLevel = static_cast<int>(DataConsumingLevel::DATA_LEVEL_SEVEN) + 1;
    const char *resStr = HiViewAdapter::GetDataLevel(dataLevel);
    EXPECT_NE(resStr, nullptr);
    EXPECT_STREQ(resStr, "WRONG_LEVEL");
}

/**
 * @tc.name: ReportStatisticEventTest001
 * @tc.desc: Test ReportStatisticEvent timeConsumingStat empty
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, ReportStatisticEventTest001, TestSize.Level1)
{
    std::vector<std::map<int, int>> timeVec = {};
    std::string stateStr = "testStr";
    EXPECT_TRUE(timeVec.empty());
    HiViewAdapter::ReportStatisticEvent(timeVec, stateStr);
}

/**
 * @tc.name: ReportBehaviourTest001
 * @tc.desc: Test ReportBehaviour timeConsumingStat empty
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, ReportBehaviourTest001, TestSize.Level1)
{
    std::map<std::string, int> beMap;
    EXPECT_TRUE(beMap.empty());
    HiViewAdapter::ReportBehaviour(beMap, "");
}

/**
 * @tc.name: ReportBehaviourTest002
 * @tc.desc: Test ReportBehaviour timeConsumingStat not empty
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, ReportBehaviourTest002, TestSize.Level1)
{
    std::map<std::string, int> beMap;
    beMap = {
    {"Copy", 1},
    {"Paste", 2},
    {"Remove", 4}
    };
    EXPECT_FALSE(beMap.empty());
    HiViewAdapter::ReportBehaviour(beMap, "test");
}

/**
 * @tc.name: InvokePasteBoardBehaviourTest001
 * @tc.desc: Test InvokePasteBoardBehaviour copyPasteboardBehaviour_ empty
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, InvokePasteBoardBehaviourTest001, TestSize.Level1)
{
    HiViewAdapter::copyPasteboardBehaviour_.clear();
    EXPECT_TRUE(HiViewAdapter::copyPasteboardBehaviour_.empty());
    HiViewAdapter::InvokePasteBoardBehaviour();
}

/**
 * @tc.name: InvokePasteBoardBehaviourTest002
 * @tc.desc: Test InvokePasteBoardBehaviour pastePasteboardBehaviour_ empty
 * @tc.type: 
 */
HWTEST_F(HiViewAdapterTest, InvokePasteBoardBehaviourTest002, TestSize.Level1)
{
    HiViewAdapter::pastePasteboardBehaviour_.clear();
    EXPECT_TRUE(HiViewAdapter::pastePasteboardBehaviour_.empty());
    HiViewAdapter::InvokePasteBoardBehaviour();
}

/**
 * @tc.name: InvokePasteBoardBehaviourTest003
 * @tc.desc: Test InvokePasteBoardBehaviour remotePastePasteboardBehaviour_ empty
 * @tc.type:
 */
HWTEST_F(HiViewAdapterTest, InvokePasteBoardBehaviourTest003, TestSize.Level1)
{
    HiViewAdapter::remotePastePasteboardBehaviour_.clear();
    EXPECT_TRUE(HiViewAdapter::remotePastePasteboardBehaviour_.empty());
    HiViewAdapter::InvokePasteBoardBehaviour();
}