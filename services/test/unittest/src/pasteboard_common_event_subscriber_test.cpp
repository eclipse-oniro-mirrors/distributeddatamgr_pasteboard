/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

#include "common_event_data.h"
#include "common_event_subscribe_info.h"
#include "common_event_support.h"
#include "pasteboard_common_event_subscriber.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service.h"
#include "paste_data.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MiscServices;

namespace OHOS {
namespace {
constexpr int32_t WIFI_DISABLED = 1;
constexpr int32_t TEST_USER_ID = 100;
constexpr int32_t INVALID_USER_ID = -1;
} // namespace

class PasteBoardCommonEventSubscriberTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteBoardCommonEventSubscriberTest::SetUpTestCase(void) { }

void PasteBoardCommonEventSubscriberTest::TearDownTestCase(void) { }

void PasteBoardCommonEventSubscriberTest::SetUp(void) { }

void PasteBoardCommonEventSubscriberTest::TearDown(void) { }

namespace MiscServices {
/**
 * @tc.name: ConstructorTest001
 * @tc.desc: test PasteBoardCommonEventSubscriber constructor with null service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, ConstructorTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConstructorTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(subscriber, nullptr);
    EXPECT_EQ(subscriber->pasteboardService_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConstructorTest001 end");
}

/**
 * @tc.name: ConstructorTest002
 * @tc.desc: test PasteBoardCommonEventSubscriber constructor with valid service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, ConstructorTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConstructorTest002 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);
    EXPECT_NE(subscriber, nullptr);
    EXPECT_NE(subscriber->pasteboardService_, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConstructorTest002 end");
}

/**
 * @tc.name: HandleScreenLockedTest001
 * @tc.desc: test HandleScreenLocked with null service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenLockedTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::CommonEventData data;
    subscriber->HandleScreenLocked(data);
    EXPECT_EQ(service, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest001 end");
}

/**
 * @tc.name: HandleScreenLockedTest002
 * @tc.desc: test HandleScreenLocked with invalid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenLockedTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest002 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetParam(PACKAGE_REMOVED_USER_ID, INVALID_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->HandleScreenLocked(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest002 end");
}

/**
 * @tc.name: HandleScreenLockedTest003
 * @tc.desc: test HandleScreenLocked with valid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenLockedTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest003 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetParam(PACKAGE_REMOVED_USER_ID, TEST_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->HandleScreenLocked(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, true);
    EXPECT_EQ(result.second, ScreenEvent::ScreenLocked);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest003 end");
}

/**
 * @tc.name: HandleScreenLockedTest004
 * @tc.desc: test HandleScreenLocked then HandleScreenUnlocked overwrite status
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenLockedTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest004 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetParam(PACKAGE_REMOVED_USER_ID, TEST_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->HandleScreenLocked(data);
    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.second, ScreenEvent::ScreenLocked);

    subscriber->HandleScreenUnlocked(data);
    result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.second, ScreenEvent::ScreenUnlocked);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenLockedTest004 end");
}

/**
 * @tc.name: HandleScreenUnlockedTest001
 * @tc.desc: test HandleScreenUnlocked with null service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenUnlockedTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenUnlockedTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::CommonEventData data;
    subscriber->HandleScreenUnlocked(data);
    EXPECT_EQ(service, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenUnlockedTest001 end");
}

/**
 * @tc.name: HandleScreenUnlockedTest002
 * @tc.desc: test HandleScreenUnlocked with invalid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenUnlockedTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenUnlockedTest002 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetParam(PACKAGE_REMOVED_USER_ID, INVALID_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->HandleScreenUnlocked(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenUnlockedTest002 end");
}

/**
 * @tc.name: HandleScreenUnlockedTest003
 * @tc.desc: test HandleScreenUnlocked with valid userId
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleScreenUnlockedTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenUnlockedTest003 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetParam(PACKAGE_REMOVED_USER_ID, TEST_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->HandleScreenUnlocked(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, true);
    EXPECT_EQ(result.second, ScreenEvent::ScreenUnlocked);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleScreenUnlockedTest003 end");
}

/**
 * @tc.name: HandleUserSwitchedTest001
 * @tc.desc: test HandleUserSwitched with null service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleUserSwitchedTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleUserSwitchedTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::CommonEventData data;
    subscriber->HandleUserSwitched(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleUserSwitchedTest001 end");
}

/**
 * @tc.name: HandleUserStoppingTest001
 * @tc.desc: test HandleUserStopping with null service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandleUserStoppingTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleUserStoppingTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::CommonEventData data;
    subscriber->HandleUserStopping(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandleUserStoppingTest001 end");
}

/**
 * @tc.name: HandlePackageRemovedTest001
 * @tc.desc: test HandlePackageRemoved with null service
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, HandlePackageRemovedTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandlePackageRemovedTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = nullptr;
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    subscriber->HandlePackageRemoved(want);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "HandlePackageRemovedTest001 end");
}

/**
 * @tc.name: OnReceiveEventInnerTest001
 * @tc.desc: test OnReceiveEventInner with screen locked action
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, OnReceiveEventInnerTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest001 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
    want.SetParam(PACKAGE_REMOVED_USER_ID, TEST_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->OnReceiveEventInner(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, true);
    EXPECT_EQ(result.second, ScreenEvent::ScreenLocked);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest001 end");
}

/**
 * @tc.name: OnReceiveEventInnerTest002
 * @tc.desc: test OnReceiveEventInner with screen unlocked action
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, OnReceiveEventInnerTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest002 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
    want.SetParam(PACKAGE_REMOVED_USER_ID, TEST_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->OnReceiveEventInner(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, true);
    EXPECT_EQ(result.second, ScreenEvent::ScreenUnlocked);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest002 end");
}

/**
 * @tc.name: OnReceiveEventInnerTest003
 * @tc.desc: test OnReceiveEventInner with unknown action
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, OnReceiveEventInnerTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest003 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetAction("unknown.action");
    want.SetParam(PACKAGE_REMOVED_USER_ID, TEST_USER_ID);
    EventFwk::CommonEventData data;
    data.SetWant(want);

    subscriber->OnReceiveEventInner(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest003 end");
}

/**
 * @tc.name: OnReceiveEventInnerTest004
 * @tc.desc: test OnReceiveEventInner with wifi disabled action
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, OnReceiveEventInnerTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest004 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_POWER_STATE);
    EventFwk::CommonEventData data;
    data.SetWant(want);
    data.SetCode(WIFI_DISABLED);

    subscriber->OnReceiveEventInner(data);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest004 end");
}

/**
 * @tc.name: OnReceiveEventInnerTest005
 * @tc.desc: test OnReceiveEventInner with wifi enabled action
 * @tc.type: FUNC
 */
HWTEST_F(PasteBoardCommonEventSubscriberTest, OnReceiveEventInnerTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest005 start");
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    sptr<PasteboardService> service = new PasteboardService();
    auto subscriber = std::make_shared<PasteBoardCommonEventSubscriber>(subscribeInfo, service);

    EventFwk::Want want;
    want.SetAction(EventFwk::CommonEventSupport::COMMON_EVENT_WIFI_POWER_STATE);
    EventFwk::CommonEventData data;
    data.SetWant(want);
    data.SetCode(0);

    subscriber->OnReceiveEventInner(data);

    auto result = service->screenStatusMap_.Find(TEST_USER_ID);
    EXPECT_EQ(result.first, false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "OnReceiveEventInnerTest005 end");
}

} // namespace MiscServices
} // namespace OHOS