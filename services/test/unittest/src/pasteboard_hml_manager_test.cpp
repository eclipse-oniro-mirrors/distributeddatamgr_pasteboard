/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "pasteboard_hml_manager.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

class PasteboardHmlManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardHmlManagerTest::SetUpTestCase()
{
}

void PasteboardHmlManagerTest::TearDownTestCase()
{
}

void PasteboardHmlManagerTest::SetUp()
{
}

void PasteboardHmlManagerTest::TearDown()
{
}

HWTEST_F(PasteboardHmlManagerTest, IsHmlSupported_001, TestSize.Level1)
{
    bool isWlanSupported = false;
    int32_t wlanRet = PasteboardHmlManager::IsWlansupported(isWlanSupported);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "IsWlansupported ret: %{public}d, isSupported: %{public}d", wlanRet, isWlanSupported);

    bool hmlResult = PasteboardHmlManager::IsHmlSupported();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "IsHmlSupported result: %{public}d", hmlResult);

    EXPECT_EQ(hmlResult, isWlanSupported);
}

HWTEST_F(PasteboardHmlManagerTest, IsWlansupported_001, TestSize.Level1)
{
    bool isSupported = false;
    int32_t ret = PasteboardHmlManager::IsWlansupported(isSupported);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
        "IsWlansupported ret: %{public}d, isSupported: %{public}d", ret, isSupported);
    EXPECT_TRUE(ret == 0);
}

HWTEST_F(PasteboardHmlManagerTest, GetSystemAbility_001, TestSize.Level1)
{
    sptr<IRemoteObject> remoteObject = nullptr;
    int32_t ret = PasteboardHmlManager::GetSystemAbility(remoteObject);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetSystemAbility ret: %{public}d", ret);
    EXPECT_TRUE(ret == 0);
}
}