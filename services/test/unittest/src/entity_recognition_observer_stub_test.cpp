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
#include "entity_recognition_observer_stub.h"
#include "pasteboard_error.h"
using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
class MockEntityObserver : public EntityRecognitionObserverStub {
public:
    void OnRecognitionEvent(EntityType entityType, std::string& entity) override
    {}
};
class EntityRecognitionObserverStubTest : public testing::Test {
public:
    EntityRecognitionObserverStubTest() {};
    ~EntityRecognitionObserverStubTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void EntityRecognitionObserverStubTest::SetUpTestCase(void) { }

void EntityRecognitionObserverStubTest::TearDownTestCase(void) { }

void EntityRecognitionObserverStubTest::SetUp(void) { }

void EntityRecognitionObserverStubTest::TearDown(void) { }

/**
 * @tc.name: OnRemoteRequestTest001
 * @tc.desc: test Func OnRemoteRequest
 * @tc.type: FUNC
 */
HWTEST_F(EntityRecognitionObserverStubTest, OnRemoteRequestTest001, TestSize.Level1)
{
    auto pbserver = std::make_shared<MockEntityObserver>();
    uint32_t code = 10;
    OHOS::MessageParcel data;
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    auto res = pbserver->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(res, static_cast<int32_t>(PasteboardError::CHECK_DESCRIPTOR_ERROR));
}

/**
 * @tc.name: OnRecognitionEventStubTest001
 * @tc.desc: test Func OnRecognitionEventStub
 * @tc.type: FUNC
 */
HWTEST_F(EntityRecognitionObserverStubTest, OnRecognitionEventStubTest001, TestSize.Level1)
{
    OHOS::MessageParcel data;
    data.WriteString("test");
    data.WriteUint32(static_cast<uint32_t>(EntityType::MAX) - 1);
    OHOS::MessageParcel reply;
    auto pbserver = std::make_shared<MockEntityObserver>();
    auto res = pbserver->OnRecognitionEventStub(data, reply);
    EXPECT_EQ(res, OHOS::ERR_OK);
}