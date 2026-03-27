/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "eventcenter/event_center.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
class EventCenterTest : public testing::Test {
public:
    enum TestEventId {
        TEST_EVT_UNKNOWN = Event::EVT_REMOTE_CHANGE,
        TEST_EVT_BEGIN = Event::EVT_REMOTE_CHANGE + 1,
        TEST_EVT_MIDDLE,
        TEST_EVT_END,
    };
    class TestBegin : public Event {
    public:
        TestBegin() : Event(TEST_EVT_BEGIN) {};
    };
    class TestMiddle : public Event {
    public:
        TestMiddle() : Event(TEST_EVT_MIDDLE) {};
    };
    class TestEnd : public Event {
    public:
        TestEnd() : Event(TEST_EVT_END) {};
    };
    static void SetUpTestCase(void) { }
    static void TearDownTestCase(void) { }
    void SetUp()
    {
        waitEvent_ = TEST_EVT_UNKNOWN;
        currEvent_ = TEST_EVT_UNKNOWN;
        EventCenter::GetInstance().Subscribe(TEST_EVT_BEGIN, [this](const Event &evt) {
            ASSERT_EQ(waitEvent_, TEST_EVT_BEGIN);
            EventCenter::Defer defer;
            EventCenter::GetInstance().PostEvent(std::make_unique<TestMiddle>());
            currEvent_ = TEST_EVT_BEGIN;
            waitEvent_ = TEST_EVT_MIDDLE;
        });
        EventCenter::GetInstance().Subscribe(TEST_EVT_MIDDLE, [this](const Event &evt) {
            ASSERT_EQ(waitEvent_, TEST_EVT_MIDDLE);
            EventCenter::Defer defer;
            EventCenter::GetInstance().PostEvent(std::make_unique<TestEnd>());
            currEvent_ = TEST_EVT_MIDDLE;
            waitEvent_ = TEST_EVT_END;
        });
        EventCenter::GetInstance().Subscribe(TEST_EVT_END, [this](const Event &evt) {
            ASSERT_EQ(waitEvent_, TEST_EVT_END);
            currEvent_ = TEST_EVT_END;
            waitEvent_ = TEST_EVT_UNKNOWN;
        });
    }

    void TearDown()
    {
        EventCenter::GetInstance().Unsubscribe(TEST_EVT_BEGIN);
        EventCenter::GetInstance().Unsubscribe(TEST_EVT_MIDDLE);
        EventCenter::GetInstance().Unsubscribe(TEST_EVT_END);
        waitEvent_ = TEST_EVT_UNKNOWN;
        currEvent_ = TEST_EVT_UNKNOWN;
    }

protected:
    int32_t waitEvent_ = TEST_EVT_UNKNOWN;
    int32_t currEvent_ = TEST_EVT_UNKNOWN;
    EventCenter::AsyncQueue queue_;
};

/**
 * @tc.name: TopLayerASyncEvent
 * @tc.desc: the async event on the top layer will dispatch, until the function completed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, TopLayerASyncEvent, TestSize.Level2)
{
    auto test = [this]() {
        EventCenter::Defer defer;
        EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
        waitEvent_ = TEST_EVT_BEGIN;
    };
    test();
    ASSERT_EQ(currEvent_, TEST_EVT_END);
    ASSERT_EQ(waitEvent_, TEST_EVT_UNKNOWN);
}

/**
 * @tc.name: SubLayerASyncEvent
 * @tc.desc: the async event on sub layer will defer to dispatch, until the top layer function completed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, SubLayerASyncEvent, TestSize.Level2)
{
    EventCenter::Defer defer;
    EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
    waitEvent_ = TEST_EVT_BEGIN;
    ASSERT_EQ(currEvent_, TEST_EVT_UNKNOWN);
    ASSERT_EQ(waitEvent_, TEST_EVT_BEGIN);
}

/**
 * @tc.name: ASyncEventWithoutDefer
 * @tc.desc: async event without defer may call or not
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, ASyncEventWithoutDefer, TestSize.Level2)
{
    EventCenter::Defer defer;
    waitEvent_ = TEST_EVT_BEGIN;
    auto test = [this]() {
        EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
    };
    test();
    ASSERT_EQ(currEvent_, TEST_EVT_UNKNOWN);
    ASSERT_EQ(waitEvent_, TEST_EVT_BEGIN);
}

/**
 * @tc.name: ImmediatelyASyncEvent
 * @tc.desc: post the async event, there is top layer and no defer; we will dispatch the async event Immediately.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, ImmediatelyASyncEvent, TestSize.Level2)
{
    waitEvent_ = TEST_EVT_BEGIN;
    EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
    ASSERT_EQ(currEvent_, TEST_EVT_END);
    ASSERT_EQ(waitEvent_, TEST_EVT_UNKNOWN);
}

/**
 * @tc.name: PostEvent
 * @tc.desc: post the async event.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, PostEvent, TestSize.Level2)
{
    int32_t result = EventCenter::GetInstance().PostEvent(nullptr);
    EXPECT_EQ(result, EventCenter::CODE_INVALID_ARGS);
}

/**
 * @tc.name: SubscribeTest
 * @tc.desc: Subscribe.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, SubscribeTest, TestSize.Level2)
{
    int32_t evtId = 1;
    std::function<void(const Event &)> observer = nullptr;
    bool result = EventCenter::GetInstance().Subscribe(evtId, observer);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: UnsubscribeTest
 * @tc.desc: Unsubscribe.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(EventCenterTest, UnsubscribeTest, TestSize.Level2)
{
    int32_t evtId = 2;
    bool result = EventCenter::GetInstance().Unsubscribe(evtId);
    EXPECT_FALSE(result);
}

 /*
* @tc.name: OperatorDecrementTest
* @tc.desc: operator--.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, OperatorDecrementTest, TestSize.Level2)
{
    EventCenter::AsyncQueue queue;
    const int32_t TEST_EVENT_ID = 1001;
    queue.depth_= 1;
    
    queue.events_.clear();
    auto test_event = std::make_unique<Event>(TEST_EVENT_ID);
    queue.events_.push_back(std::move(test_event));
    bool handlerCalled = false;
    queue.handlers_[TEST_EVENT_ID] = [&](const Event &evt) {
        handlerCalled = true;
        EXPECT_EQ(evt.GetEventId(), TEST_EVENT_ID);
    };
    --queue;
    EXPECT_TRUE(handlerCalled);
}

/*
* @tc.name: AddHandlerTest01
* @tc.desc: Triggering logging when verifying evtId as EVT_INVALID.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, AddHandlerTest01, TestSize.Level2)
{
    auto validHandler = [](const Event &evt) {};
    int32_t invalidEvtId = Event::EVT_INVALID;
    queue_.AddHandler(invalidEvtId, validHandler);
    EXPECT_TRUE(queue_.handlers_.empty());
}

/*
* @tc.name: AddHandlerTest02
* @tc.desc: Trigger logging when handler is null.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, AddHandlerTest02, TestSize.Level2)
{
    int32_t validEvtId = 1001;
    std::function<void(const Event &)> nullHandler = nullptr;
    queue_.AddHandler(validEvtId, nullHandler);
    EXPECT_TRUE(queue_.handlers_.empty());
}

/*
* @tc.name: AddHandlerTest03
* @tc.desc: Trigger the second PASTEBOARD_CHECK-AND-RETURN_LOGE.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, AddHandlerTest03, TestSize.Level2)
{
    int32_t testEvtId = 1002;
    auto firstHandler = [](const Event &evt) {};
    queue_.AddHandler(testEvtId, firstHandler);
    EXPECT_EQ(queue_.handlers_.size(), 1);
    EXPECT_NE(queue_.handlers_.find(testEvtId), queue_.handlers_.end());
    auto secondHandler = [](const Event &evt) {};
    queue_.AddHandler(testEvtId, secondHandler);
    EXPECT_EQ(queue_.handlers_.size(), 1);
}

/*
* @tc.name: AddHandlerTest04
* @tc.desc: Verify normal branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, AddHandlerTest04, TestSize.Level2)
{
    int32_t testEvtId = 1003;
    bool isHandlerCalled = false;
    auto testHandler = [&isHandlerCalled](const Event &evt) {
        isHandlerCalled = true;
    };
    queue_.AddHandler(testEvtId, testHandler);
    EXPECT_EQ(queue_.handlers_.size(), 1);
    EXPECT_NE(queue_.handlers_.find(testEvtId), queue_.handlers_.end());
}
} // namespace OHOS::MiscServices