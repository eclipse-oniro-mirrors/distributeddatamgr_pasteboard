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

// Host-only unit test for OHOS::MiscServices::EventCenter (sync/async event
// dispatch). Depends on event.cpp + event_center.cpp + a hilog shim + gtest.
// No device, no IPC, no running service.

#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "eventcenter/event.h"
#include "eventcenter/event_center.h"

using namespace testing::ext;

namespace OHOS::MiscServices {
namespace {
constexpr int32_t TEST_EVENT_PAYLOAD = 42;
} // namespace

// Concrete test event carrying a payload so we can assert dispatch happened.
class TestEvent : public Event {
public:
    explicit TestEvent(int32_t id, int32_t payload = 0) : Event(id), payload_(payload) {}
    int32_t payload_;
};

class EventCenterHostTest : public testing::Test {
protected:
    // EventCenter is a singleton; clean up any subscriptions between tests so
    // cases stay independent.
    void TearDown() override
    {
        EventCenter::GetInstance().Unsubscribe(Event::EVT_UPDATE);
        EventCenter::GetInstance().Unsubscribe(Event::EVT_INIT);
    }
};

/**
 * @tc.name: SyncPostDispatchesToSubscriber
 * @tc.desc: A synchronous PostEvent dispatches to a subscriber immediately.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, SyncPostDispatchesToSubscriber, TestSize.Level0)
{
    int hits = 0;
    int32_t seenId = -1;
    EXPECT_TRUE(EventCenter::GetInstance().Subscribe(Event::EVT_UPDATE, [&](const Event &e) {
        hits++;
        seenId = e.GetEventId();
    }));

    int32_t rc = EventCenter::GetInstance().PostEvent(std::make_unique<TestEvent>(Event::EVT_UPDATE));
    EXPECT_EQ(rc, EventCenter::CODE_SYNC);
    EXPECT_EQ(hits, 1);
    EXPECT_EQ(seenId, Event::EVT_UPDATE);
}

/**
 * @tc.name: SyncPostFansOutToAllSubscribers
 * @tc.desc: Multiple subscribers on the same id all fire.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, SyncPostFansOutToAllSubscribers, TestSize.Level0)
{
    int a = 0;
    int b = 0;
    EventCenter::GetInstance().Subscribe(Event::EVT_UPDATE, [&](const Event &) { a++; });
    EventCenter::GetInstance().Subscribe(Event::EVT_UPDATE, [&](const Event &) { b++; });

    EventCenter::GetInstance().PostEvent(std::make_unique<TestEvent>(Event::EVT_UPDATE));
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
}

/**
 * @tc.name: PostNullEventReturnsInvalidArgs
 * @tc.desc: Posting a null event returns CODE_INVALID_ARGS.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, PostNullEventReturnsInvalidArgs, TestSize.Level0)
{
    int32_t rc = EventCenter::GetInstance().PostEvent(nullptr);
    EXPECT_EQ(rc, EventCenter::CODE_INVALID_ARGS);
}

/**
 * @tc.name: PostWithNoSubscriberIsSync
 * @tc.desc: Posting an event with no subscriber is a no-op but still CODE_SYNC.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, PostWithNoSubscriberIsSync, TestSize.Level0)
{
    int32_t rc = EventCenter::GetInstance().PostEvent(std::make_unique<TestEvent>(Event::EVT_INIT));
    EXPECT_EQ(rc, EventCenter::CODE_SYNC);
}

/**
 * @tc.name: UnsubscribeStopsDispatch
 * @tc.desc: Unsubscribe removes the handler; a later post no longer fires it.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, UnsubscribeStopsDispatch, TestSize.Level0)
{
    int hits = 0;
    EventCenter::GetInstance().Subscribe(Event::EVT_UPDATE, [&](const Event &) { hits++; });
    EXPECT_TRUE(EventCenter::GetInstance().Unsubscribe(Event::EVT_UPDATE));

    EventCenter::GetInstance().PostEvent(std::make_unique<TestEvent>(Event::EVT_UPDATE));
    EXPECT_EQ(hits, 0);
}

/**
 * @tc.name: UnsubscribeUnknownReturnsFalse
 * @tc.desc: Unsubscribing an id that was never subscribed returns false.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, UnsubscribeUnknownReturnsFalse, TestSize.Level0)
{
    EXPECT_FALSE(EventCenter::GetInstance().Unsubscribe(Event::EVT_REMOTE_CHANGE));
}

/**
 * @tc.name: DeferMakesPostAsyncAndFlushesOnExit
 * @tc.desc: Inside a Defer scope, posts are async and flush on scope exit.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, DeferMakesPostAsyncAndFlushesOnExit, TestSize.Level0)
{
    int hits = 0;
    EventCenter::GetInstance().Subscribe(Event::EVT_UPDATE, [&](const Event &) { hits++; });

    {
        EventCenter::Defer defer;
        int32_t rc = EventCenter::GetInstance().PostEvent(std::make_unique<TestEvent>(Event::EVT_UPDATE));
        EXPECT_EQ(rc, EventCenter::CODE_ASYNC);
        // Not dispatched yet: still inside the deferred scope.
        EXPECT_EQ(hits, 0);
    }
    // On Defer destruction the queued event is flushed to subscribers.
    EXPECT_EQ(hits, 1);
}

/**
 * @tc.name: DeferTemporaryHandlerFires
 * @tc.desc: A temporary handler passed to Defer also fires on flush.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, DeferTemporaryHandlerFires, TestSize.Level0)
{
    int temp = 0;
    {
        EventCenter::Defer defer([&](const Event &) { temp++; }, Event::EVT_UPDATE);
        EventCenter::GetInstance().PostEvent(std::make_unique<TestEvent>(Event::EVT_UPDATE));
        EXPECT_EQ(temp, 0);
    }
    EXPECT_EQ(temp, 1);
}

/**
 * @tc.name: EventBasics
 * @tc.desc: Event basics: id round-trips, base Equals is always false.
 * @tc.type: FUNC
 * @tc.require: issueI1669
 * @tc.author:
 */
HWTEST_F(EventCenterHostTest, EventBasics, TestSize.Level0)
{
    TestEvent e(Event::EVT_UPDATE, TEST_EVENT_PAYLOAD);
    EXPECT_EQ(e.GetEventId(), Event::EVT_UPDATE);
    TestEvent other(Event::EVT_UPDATE, TEST_EVENT_PAYLOAD);
    // Base Event::Equals is defined to return false (no dedup by default).
    EXPECT_FALSE(e.Equals(other));
}
} // namespace OHOS::MiscServices
