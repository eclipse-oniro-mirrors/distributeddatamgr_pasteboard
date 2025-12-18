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

#include "pasteboard_dialog.h"
#include "pasteboard_error.h"

using namespace OHOS;

namespace OHOS::MiscServices {
using namespace testing::ext;

class PasteboardDialogTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardDialogTest::SetUpTestCase(void) { }

void PasteboardDialogTest::TearDownTestCase(void) { }

void PasteboardDialogTest::SetUp(void) { }

void PasteboardDialogTest::TearDown(void) { }

/**
 * @tc.name: ShowProgressFailedTest
 * @tc.desc: should return error when has no permission
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDialogTest, ShowProgressFailedTest, TestSize.Level0)
{
    PasteboardDialog::ProgressMessageInfo message;
    int32_t ret = PasteboardDialog::ShowProgress(message);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::PROGRESS_START_ERROR));
}
} // namespace OHOS::MiscServices
