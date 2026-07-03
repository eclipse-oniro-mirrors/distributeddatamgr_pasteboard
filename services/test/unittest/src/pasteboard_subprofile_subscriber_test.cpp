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
#include <thread>
#include <chrono>
#include "pasteboard_subprofile_subscriber.h"
#include "pasteboard_service.h"
#include "distributed_account_subscribe_callback.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace {
    const int32_t TEST_USER_ID = 100;
    const int32_t TEST_USER_ID_2 = 101;
    const int32_t TEST_SUBSPACE_ID_A = 1;
    const int32_t TEST_SUBSPACE_ID_B = 2;
    const int32_t INVALID_USER_ID = -1;
    constexpr int32_t WAIT_TIMEOUT_MS = 200;
}

namespace OHOS::AccountSA {
bool DistributedAccountSubProfileEventData::Marshalling(Parcel &parcel) const
{
    return false;
}

DistributedAccountSubProfileEventData *DistributedAccountSubProfileEventData::Unmarshalling(Parcel &parcel)
{
    return nullptr;
}

bool DistributedAccountSubProfileEventData::operator==(const DistributedAccountSubProfileEventData &eventData) const
{
    return false;
}

bool DistributedAccountSubProfileEventData::ReadFromParcel(Parcel &parcel)
{
    return false;
}
}
class PasteboardSubProfileSubscriberTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void PasteboardSubProfileSubscriberTest::SetUpTestCase() {}
void PasteboardSubProfileSubscriberTest::TearDownTestCase() {}
void PasteboardSubProfileSubscriberTest::SetUp() {}
void PasteboardSubProfileSubscriberTest::TearDown() {}

/**
 * @tc.name: SpaceSwitchingTest001
 * @tc.desc: Test subscriber handles SWITCHED event with real PasteboardService
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSubProfileSubscriberTest, SpaceSwitchingTest001, TestSize.Level1)
{
    sptr<PasteboardService> service = new PasteboardService();
    ASSERT_NE(service, nullptr);

    auto subscriber = std::make_shared<PasteboardSubProfileSubscriber>(service);
    ASSERT_NE(subscriber, nullptr);

    AccountSA::DistributedAccountSubProfileEventData eventData;
    eventData.type_ = AccountSA::DistributedAccountSubProfileEventType::SWITCHED;
    eventData.osAccountId_ = TEST_USER_ID;
    eventData.subspaceId_ = TEST_SUBSPACE_ID_B;
    eventData.previousSubspaceId_ = TEST_SUBSPACE_ID_A;

    subscriber->OnSubProfileAccountsChanged(eventData);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
}

/**
 * @tc.name: SpaceSwitchingTest002
 * @tc.desc: Test subscriber with null service handles event safely
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSubProfileSubscriberTest, SpaceSwitchingTest002, TestSize.Level1)
{
    auto subscriber = std::make_shared<PasteboardSubProfileSubscriber>(nullptr);
    ASSERT_NE(subscriber, nullptr);

    AccountSA::DistributedAccountSubProfileEventData eventData;
    eventData.type_ = AccountSA::DistributedAccountSubProfileEventType::SWITCHED;
    eventData.osAccountId_ = TEST_USER_ID;
    eventData.subspaceId_ = TEST_SUBSPACE_ID_A;
    eventData.previousSubspaceId_ = TEST_SUBSPACE_ID_B;

    subscriber->OnSubProfileAccountsChanged(eventData);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
}

/**
 * @tc.name: SpaceSwitchingTest003
 * @tc.desc: Test subscriber ignores invalid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSubProfileSubscriberTest, SpaceSwitchingTest003, TestSize.Level1)
{
    sptr<PasteboardService> service = new PasteboardService();
    ASSERT_NE(service, nullptr);

    auto subscriber = std::make_shared<PasteboardSubProfileSubscriber>(service);
    ASSERT_NE(subscriber, nullptr);

    AccountSA::DistributedAccountSubProfileEventData eventData;
    eventData.type_ = AccountSA::DistributedAccountSubProfileEventType::SWITCHED;
    eventData.osAccountId_ = INVALID_USER_ID;
    eventData.subspaceId_ = TEST_SUBSPACE_ID_A;
    eventData.previousSubspaceId_ = TEST_SUBSPACE_ID_B;

    subscriber->OnSubProfileAccountsChanged(eventData);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
}

/**
 * @tc.name: SpaceSwitchingTest004
 * @tc.desc: Test subscriber ignores invalid event type
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSubProfileSubscriberTest, SpaceSwitchingTest004, TestSize.Level1)
{
    sptr<PasteboardService> service = new PasteboardService();
    ASSERT_NE(service, nullptr);

    auto subscriber = std::make_shared<PasteboardSubProfileSubscriber>(service);
    ASSERT_NE(subscriber, nullptr);

    AccountSA::DistributedAccountSubProfileEventData eventData;
    eventData.type_ = AccountSA::DistributedAccountSubProfileEventType::INVALID_TYPE;
    eventData.osAccountId_ = TEST_USER_ID;
    eventData.subspaceId_ = TEST_SUBSPACE_ID_A;
    eventData.previousSubspaceId_ = TEST_SUBSPACE_ID_B;

    subscriber->OnSubProfileAccountsChanged(eventData);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
}

/**
 * @tc.name: SpaceSwitchingTest005
 * @tc.desc: Test subscriber handles multiple events in sequence
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardSubProfileSubscriberTest, SpaceSwitchingTest005, TestSize.Level1)
{
    sptr<PasteboardService> service = new PasteboardService();
    ASSERT_NE(service, nullptr);

    auto subscriber = std::make_shared<PasteboardSubProfileSubscriber>(service);
    ASSERT_NE(subscriber, nullptr);

    AccountSA::DistributedAccountSubProfileEventData eventData1;
    eventData1.type_ = AccountSA::DistributedAccountSubProfileEventType::SWITCHED;
    eventData1.osAccountId_ = TEST_USER_ID;
    eventData1.subspaceId_ = TEST_SUBSPACE_ID_A;
    eventData1.previousSubspaceId_ = TEST_SUBSPACE_ID_B;

    subscriber->OnSubProfileAccountsChanged(eventData1);

    AccountSA::DistributedAccountSubProfileEventData eventData2;
    eventData2.type_ = AccountSA::DistributedAccountSubProfileEventType::SWITCHED;
    eventData2.osAccountId_ = TEST_USER_ID_2;
    eventData2.subspaceId_ = TEST_SUBSPACE_ID_B;
    eventData2.previousSubspaceId_ = TEST_SUBSPACE_ID_A;

    subscriber->OnSubProfileAccountsChanged(eventData2);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));
}