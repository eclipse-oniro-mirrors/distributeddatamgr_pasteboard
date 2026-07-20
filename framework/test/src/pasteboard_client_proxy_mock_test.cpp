/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "pasteboard_client_mock_test.h"

#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "pasteboard_service_loader.h"
#include "pasteboard_hilog.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MiscServices {

void PasteboardClientMockTest::SetUpTestCase(void)
{
    PasteSystemAbilityManagerClient::pasteSystemAbilityManagerClient = systemAbilityManagerClientMock_;
}

void PasteboardClientMockTest::TearDownTestCase(void)
{
    PasteSystemAbilityManagerClient::pasteSystemAbilityManagerClient = nullptr;
    systemAbilityManagerClientMock_ = nullptr;
}

void PasteboardClientMockTest::SetUp(void) { }

void PasteboardClientMockTest::TearDown(void)
{
    testing::Mock::VerifyAndClear(systemAbilityManagerClientMock_.get());
}

/**
 * @tc.name: GetChangeCount001
 * @tc.desc: GetChangeCount.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, GetChangeCount001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetChangeCount001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    uint32_t changeCount = 0;
    int32_t ret = PasteboardClient::GetInstance()->GetChangeCount(changeCount);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetChangeCount001 end");
}

/**
 * @tc.name: GetPasteData001
 * @tc.desc: GetPasteData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, GetPasteData001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteData001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    PasteData pasteData;
    int32_t status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteData001 end");
}

/**
 * @tc.name: GetPasteDataFromService001
 * @tc.desc: GetPasteDataFromService.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, GetPasteDataFromService001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteDataFromService001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    PasteData pasteData;
    PasteDataFromServiceInfo pasteDataFromServiceInfo;
    std::string progressKey = "";
    std::shared_ptr<GetDataParams> params = nullptr;
    int32_t status = PasteboardClient::GetInstance()->GetPasteDataFromService(
        pasteData, pasteDataFromServiceInfo, progressKey, params);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteDataFromService001 end");
}

/**
 * @tc.name: SetPasteData001
 * @tc.desc: SetPasteData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SetPasteData001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteData001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    PasteData pasteData;
    int32_t status = PasteboardClient::GetInstance()->SetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPasteData001 end");
}

/**
 * @tc.name: GetPasteDataInfo001
 * @tc.desc: GetPasteDataInfo return OBTAIN_SERVER_SA_ERROR when proxy is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, GetPasteDataInfo001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteDataInfo001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    PasteDataInfo pasteDataInfo;
    int32_t status = PasteboardClient::GetInstance()->GetPasteDataInfo(pasteDataInfo);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::OBTAIN_SERVER_SA_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetPasteDataInfo001 end");
}

/**
 * @tc.name: SubscribeTest001
 * @tc.desc: SubscribeTest.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SubscribeTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SubscribeTest001 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    sptr<PasteboardObserver> callback = nullptr;
    auto ret = PasteboardClient::GetInstance()->Subscribe(PasteboardObserverType::OBSERVER_LOCAL, callback);
    ASSERT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SubscribeTest001 end");
}

/**
 * @tc.name: SubscribeTest002
 * @tc.desc: SubscribeTest.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientMockTest, SubscribeTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SubscribeTest002 start");
    EXPECT_CALL(*systemAbilityManagerClientMock_, GetSystemAbilityManager()).WillRepeatedly(testing::Return(nullptr));
    PasteboardServiceLoader::GetInstance().pasteboardServiceProxy_ = nullptr;
    PasteboardServiceLoader::GetInstance().constructing_ = false;
    sptr<PasteboardObserver> callback = sptr<PasteboardObserver>::MakeSptr();
    auto ret = PasteboardClient::GetInstance()->Subscribe(PasteboardObserverType::OBSERVER_LOCAL, callback);
    ASSERT_EQ(ret, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SubscribeTest002 end");
}

} // namespace MiscServices
} // namespace OHOS
