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

#include "pasteboard_ability_manager.h"
#include "pasteboard_dialog.h"
#include "pasteboard_error.h"
#include "pasteboard_signal_callback.h"

using namespace OHOS;

namespace OHOS::MiscServices {
using namespace testing::ext;

class PasteboardAbilityManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardAbilityManagerTest::SetUpTestCase(void) { }

void PasteboardAbilityManagerTest::TearDownTestCase(void) { }

void PasteboardAbilityManagerTest::SetUp(void) { }

void PasteboardAbilityManagerTest::TearDown(void) { }

/**
 * @tc.name: ShowProgressFailedTest
 * @tc.desc: should return error when has no permission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardAbilityManagerTest, ShowProgressFailedTest, TestSize.Level0)
{
    PasteboardDialog::ProgressMessageInfo message;
    int32_t ret = PasteboardDialog::ShowProgress(message);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PROGRESS_START_ERROR));

    message.callerToken = sptr<PasteboardSignalCallback>::MakeSptr();
    message.clientCallback = sptr<PasteboardSignalCallback>::MakeSptr();
    ret = PasteboardDialog::ShowProgress(message);
    EXPECT_EQ(ret, static_cast<int32_t>(PasteboardError::PROGRESS_START_ERROR));
}

/**
 * @tc.name: CheckUIExtensionIsFocusedFailedTest
 * @tc.desc: should return error when has no permission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardAbilityManagerTest, CheckUIExtensionIsFocusedFailedTest, TestSize.Level0)
{
    uint32_t tokenId = 0;
    bool isFocused = false;
    int32_t ret = PasteboardAbilityManager::CheckUIExtensionIsFocused(tokenId, isFocused);
    EXPECT_NE(ret, NO_ERROR);
}
} // namespace OHOS::MiscServices
