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
#include <thread>

#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "unistd.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;

constexpr uint32_t MAX_RECOGNITION_LENGTH = 1000;
constexpr uint32_t TEST_RECOGNITION_LENGTH = 100;

class PasteboardEntityClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

class TestEntityRecognitionObserver : public EntityRecognitionObserver {
public:
    void OnRecognitionEvent(EntityType entityType, std::string &entity) override
    {
        entityType_ = entityType;
        entity_ = entity;
        uint32_t type = static_cast<uint32_t>(entityType);
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE,
            "observer callback, entityType=%{public}u, entity=%{private}s", type, entity.c_str());
    }
    EntityType entityType_;
    std::string entity_ = "";
};

void PasteboardEntityClientTest::SetUpTestCase(void)
{
}

void PasteboardEntityClientTest::TearDownTestCase(void)
{
}

void PasteboardEntityClientTest::SetUp(void)
{
}

void PasteboardEntityClientTest::TearDown(void)
{
}


/**
 * @tc.name: SubscribeEntityObserverTest001
 * @tc.desc: Subscribe EntityObserver when entityType is invalid value, should return ERR_INVALID_VALUE.
 * hen EntityType is MAX, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest001, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::MAX, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest002
 * @tc.desc: Subscribe EntityObserver when expectedDataLength exceeds limitation, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest002, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH + 1;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest003
 * @tc.desc: Subscribe EntityObserver when observer is nullptr, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest003, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = nullptr;
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest004
 * @tc.desc: Subscribe EntityObserver normally should return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest004, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest005
 * @tc.desc: Subscribe EntityObserver normally should return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest005, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    PasteData pasteData;
    std::string plainText = "陕西省西安市高新区丈八八路";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_NE(newData, nullptr);
    result = PasteboardClient::GetInstance()->SetPasteData(*newData);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest006
 * @tc.desc: Subscribe observer again will return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest006, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest007
 * @tc.desc: Subscribe another observer will replace old one and return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest007, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = new TestEntityRecognitionObserver();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    sptr<EntityRecognitionObserver> otherObserver = sptr<EntityRecognitionObserver>::MakeSptr();
    result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, otherObserver);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, otherObserver);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest008
 * @tc.desc: Subscribe observer again with different param will return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest008, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, TEST_RECOGNITION_LENGTH, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, TEST_RECOGNITION_LENGTH, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: SubscribeEntityObserverTest009
 * @tc.desc: Unsubscribe EntityObserver and copy plainText, will not exec callback.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest009, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<TestEntityRecognitionObserver> observer = sptr<TestEntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    PasteData pasteData;
    std::string plainText = "陕西省西安市高新区丈八八路";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_NE(newData, nullptr);
    result = PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    ASSERT_TRUE(observer->entity_.empty());
}

/**
 * @tc.name: SubscribeEntityObserverTest010
 * @tc.desc: Subscribe EntityObserver and copy plainText with ShareOption::InApp, will not exec callback.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, SubscribeEntityObserverTest0010, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<TestEntityRecognitionObserver> observer = sptr<TestEntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->SubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    std::string plainText = "陕西省西安市高新区丈八八路";
    auto newData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_NE(newData, nullptr);
    newData->SetShareOption(ShareOption::InApp);
    result = PasteboardClient::GetInstance()->SetPasteData(*newData);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    ASSERT_TRUE(observer->entity_.empty());
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest001
 * @tc.desc: Subscribe EntityObserver when EntityType is MAX, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, UnsubscribeEntityObserverTest001, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::MAX, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest002
 * @tc.desc: Subscribe EntityObserver when expectedDataLength exceeds limitation, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, UnsubscribeEntityObserverTest002, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH + 1;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest003
 * @tc.desc: Subscribe EntityObserver when observer is nullptr, should return INVALID_PARAM_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, UnsubscribeEntityObserverTest003, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = nullptr;
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest004
 * @tc.desc: Subscribe EntityObserver normally should return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, UnsubscribeEntityObserverTest004, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = sptr<EntityRecognitionObserver>::MakeSptr();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}

/**
 * @tc.name: UnsubscribeEntityObserverTest005
 * @tc.desc: Unsubscribe observer again will return E_OK.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardEntityClientTest, UnsubscribeEntityObserverTest005, TestSize.Level0)
{
    uint32_t expectedDataLength = MAX_RECOGNITION_LENGTH;
    sptr<EntityRecognitionObserver> observer = new TestEntityRecognitionObserver();
    int32_t result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
    result = PasteboardClient::GetInstance()->UnsubscribeEntityObserver(
        EntityType::ADDRESS, expectedDataLength, observer);
    ASSERT_EQ(static_cast<int32_t>(PasteboardError::E_OK), result);
}
} // namespace OHOS::MiscServices
