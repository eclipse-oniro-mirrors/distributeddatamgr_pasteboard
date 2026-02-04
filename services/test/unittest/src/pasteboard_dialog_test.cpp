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
#include "pasteboard_dialog.h"
#include "pasteboard_ability_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "want.h"
using namespace testing;
using namespace testing::ext;
using namespace OHOS::AAFwk;
using namespace OHOS::MiscServices;
namespace OHOS {
namespace MiscServices {
int32_t PasteboardAbilityManager::StartAbility(const OHOS::AAFwk::Want& want)
{
    return 0;
}
} // namespace MiscServices
} // namespace OHOS

class PasteboardDialogTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: ShowProgressTest001
 * @tc.desc: retur E_OK
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardDialogTest, ShowProgressTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ShowProgressTest001 start");
    PasteboardDialog::ProgressMessageInfo message = {
        .promptText = "test success",
        .remoteDeviceName = "local device",
        .progressKey = "success_key",
        .isRemote = false,
        .left = 10,
        .top = 20,
        .width = 300,
        .height = 200,
        .clientCallback = nullptr,
        .callerToken = nullptr
    };
    PasteboardDialog dialog;
    int32_t result = dialog.ShowProgress(message);
    EXPECT_EQ(result, static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ShowProgressTest001 end");
}