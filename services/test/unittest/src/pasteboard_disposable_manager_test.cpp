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

#include <algorithm>
#include <gtest/gtest.h>
#include <thread>
#include <tuple>

#include "accesstoken_kit_mock.h"
#include "ffrt/ffrt_utils.h"
#include "pasteboard_disposable_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_window_manager.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using testing::NiceMock;

constexpr int32_t INVALID_VALUE = -1;

class PasteboardDisposableManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardDisposableManagerTest::SetUpTestCase()
{
}

void PasteboardDisposableManagerTest::TearDownTestCase()
{
}

void PasteboardDisposableManagerTest::SetUp()
{
}

void PasteboardDisposableManagerTest::TearDown()
{
    FFRTPool::Clear();
    DisposableManager::GetInstance().disposableInfoList_.clear();
}

class DisposableObserverImpl : public IPasteboardDisposableObserver {
public:
    void OnTextReceived(const std::string &text, int32_t errCode) override
    {
        PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "text=%{public}s, errCode=%{public}d", text.c_str(), errCode);
        text_ = text;
        errCode_ = errCode;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    std::string text_;
    int32_t errCode_ = INVALID_VALUE;
};

class DelayGetterImpl : public IPasteboardDelayGetter {
public:
    void GetPasteData(const std::string &type, PasteData &data) override
    {
        (void)type;
        data.AddTextRecord(text_);
    }

    void GetUnifiedData(const std::string &type, UDMF::UnifiedData &data) override
    {
        (void)type;
        (void)data;
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    std::string text_;
};

class EntryGetterImpl : public IPasteboardEntryGetter {
public:
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &entry) override
    {
        constexpr uint32_t recordId1 = 1;
        constexpr uint32_t recordId2 = 2;
        if (recordId == recordId1) {
            return static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID);
        }
        if (recordId == recordId2) {
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        entry.SetValue(text_);
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    std::string text_;
};

bool operator==(const DisposableInfo &lhs, const DisposableInfo &rhs)
{
    return std::tie(lhs.pid, lhs.tokenId, lhs.targetWindowId, lhs.type, lhs.maxLen)
        == std::tie(rhs.pid, rhs.tokenId, rhs.targetWindowId, rhs.type, rhs.maxLen);
}

/**
 * @tc.name: AddDisposableInfoTest001
 * @tc.desc: should return INVALID_PARAM_ERROR when param invalid
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, AddDisposableInfoTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "AddDisposableInfoTest001 start");
    pid_t pid = 1;
    uint32_t tokenId = 100;
    int32_t targetWindowId = 1;
    DisposableType type = DisposableType::MAX;
    uint32_t maxLen = 1000;
    sptr<IPasteboardDisposableObserver> observer = nullptr;
    DisposableInfo info(pid, tokenId, targetWindowId, type, maxLen, observer);
    int32_t ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));

    info.observer = sptr<DisposableObserverImpl>::MakeSptr();
    ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::INVALID_PARAM_ERROR));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: AddDisposableInfoTest002
 * @tc.desc: should return PERMISSION_VERIFICATION_ERROR when verify permission denied
 *           should return E_OK when verify permission granted
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, AddDisposableInfoTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillOnce(testing::Return(Security::AccessToken::PermissionState::PERMISSION_DENIED))
        .WillOnce(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    pid_t pid = 1;
    uint32_t tokenId = 100;
    int32_t targetWindowId = 1;
    DisposableType type = DisposableType::PLAIN_TEXT;
    uint32_t maxLen = 1000;
    sptr<IPasteboardDisposableObserver> observer = sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info(pid, tokenId, targetWindowId, type, maxLen, observer);
    int32_t ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::PERMISSION_VERIFICATION_ERROR));

    ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    auto infoList = DisposableManager::GetInstance().disposableInfoList_;
    ASSERT_EQ(infoList.size(), 1);
    EXPECT_EQ(infoList[0], info);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: AddDisposableInfoTest003
 * @tc.desc: should update info when called twice with same pid before timeout
 *           should append info when called twice with diff pid before timeout
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, AddDisposableInfoTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillRepeatedly(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    pid_t pid = 1;
    uint32_t tokenId = 100;
    int32_t targetWindowId = 1;
    DisposableType type = DisposableType::PLAIN_TEXT;
    uint32_t maxLen = 1000;
    sptr<IPasteboardDisposableObserver> observer = sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info(pid, tokenId, targetWindowId, type, maxLen, observer);
    int32_t ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    auto infoList = DisposableManager::GetInstance().disposableInfoList_;
    ASSERT_EQ(infoList.size(), 1);
    EXPECT_EQ(infoList[0], info);

    DisposableInfo samePid(pid, tokenId + 1, targetWindowId + 1, type, maxLen + 1, observer);
    ret = DisposableManager::GetInstance().AddDisposableInfo(samePid);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    infoList = DisposableManager::GetInstance().disposableInfoList_;
    ASSERT_EQ(infoList.size(), 1);
    EXPECT_EQ(infoList[0], samePid);

    DisposableInfo diffPid = info;
    diffPid.pid += 1;
    ret = DisposableManager::GetInstance().AddDisposableInfo(diffPid);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    infoList = DisposableManager::GetInstance().disposableInfoList_;
    ASSERT_EQ(infoList.size(), 2);
    EXPECT_EQ(infoList[0], samePid);
    EXPECT_EQ(infoList[1], diffPid);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: AddDisposableInfoTest004
 * @tc.desc: should callback ERR_TIMEOUT when timeout
 *           should clear info after timeout
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, AddDisposableInfoTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillRepeatedly(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    pid_t pid = 1;
    uint32_t tokenId = 100;
    int32_t targetWindowId = 1;
    DisposableType type = DisposableType::PLAIN_TEXT;
    uint32_t maxLen = 1000;
    sptr<DisposableObserverImpl> observer = sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info(pid, tokenId, targetWindowId, type, maxLen, observer);
    int32_t ret = DisposableManager::GetInstance().AddDisposableInfo(info);
    ASSERT_EQ(ret, static_cast<int32_t>(PasteboardError::E_OK));
    auto infoList = DisposableManager::GetInstance().disposableInfoList_;
    ASSERT_EQ(infoList.size(), 1);
    EXPECT_EQ(infoList[0], info);

    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(observer->errCode_, IPasteboardDisposableObserver::ERR_TIMEOUT);
    EXPECT_STREQ(observer->text_.c_str(), "");
    infoList = DisposableManager::GetInstance().disposableInfoList_;
    EXPECT_EQ(infoList.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: RemoveDisposableInfoTest001
 * @tc.desc: should do nothing when pid not find
 *           should remove info & not callback when pid find but observer is null
 *           should remove info & callback ERR_TIMEOUT when pid find & observer not null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, RemoveDisposableInfoTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    pid_t pid = 1;
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);

    DisposableInfo info(pid, 1, 1, DisposableType::PLAIN_TEXT, 1, nullptr);
    DisposableManager::GetInstance().disposableInfoList_ = {info};
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());

    DisposableManager::GetInstance().disposableInfoList_ = {info};
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, true);
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());

    sptr<DisposableObserverImpl> observer = sptr<DisposableObserverImpl>::MakeSptr();
    info.observer = observer;
    DisposableManager::GetInstance().disposableInfoList_ = {info};
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, false);
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());
    EXPECT_EQ(observer->errCode_, INVALID_VALUE);

    DisposableManager::GetInstance().disposableInfoList_ = {info};
    DisposableManager::GetInstance().RemoveDisposableInfo(pid, true);
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());
    EXPECT_EQ(observer->errCode_, IPasteboardDisposableObserver::ERR_TIMEOUT);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: TryProcessDisposableDataTest001
 * @tc.desc: should remove info but not callback when observer is null
 *           should remove info & callback ERR_TARGET_MISMATCH when windowId missmatch & observer not null
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, TryProcessDisposableDataTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillRepeatedly(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    PasteData pasteData;
    int32_t windowId = 1;
    sptr<DisposableObserverImpl> observer = sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo infoMatched(1, 1, windowId, DisposableType::PLAIN_TEXT, 1, nullptr);
    DisposableInfo infoNoMatch(1, 1, windowId + 1, DisposableType::PLAIN_TEXT, 1, nullptr);

    WindowManager::SetFocusWindowId(windowId);
    DisposableManager::GetInstance().disposableInfoList_ = {};
    bool ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
    EXPECT_FALSE(ret);

    DisposableManager::GetInstance().disposableInfoList_ = {infoMatched};
    ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());

    DisposableManager::GetInstance().disposableInfoList_ = {infoNoMatch};
    ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
    EXPECT_FALSE(ret);
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());

    infoNoMatch.observer = observer;
    DisposableManager::GetInstance().disposableInfoList_ = {infoMatched, infoNoMatch};
    ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
    EXPECT_TRUE(ret);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(DisposableManager::GetInstance().disposableInfoList_.empty());
    EXPECT_EQ(observer->errCode_, IPasteboardDisposableObserver::ERR_TARGET_MISMATCH);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: TryProcessDisposableDataTest002
 * @tc.desc: should callback ERR_NO_PERMISSION without text when verify permission denied
 *           should callback ERR_TYPE_NOT_SUPPORT without text when type not support
 *           should callback ERR_NO_TEXT without text when paste data has no text
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, TryProcessDisposableDataTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillOnce(testing::Return(Security::AccessToken::PermissionState::PERMISSION_DENIED))
        .WillRepeatedly(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    PasteData pasteData;
    int32_t windowId = 1;
    sptr<DisposableObserverImpl> observer1 = nullptr;
    sptr<DisposableObserverImpl> observer2 = sptr<DisposableObserverImpl>::MakeSptr();
    sptr<DisposableObserverImpl> observer3 = sptr<DisposableObserverImpl>::MakeSptr();
    sptr<DisposableObserverImpl> observer4 = sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info1(1, 1, windowId, DisposableType::PLAIN_TEXT, 1, observer1);
    DisposableInfo info2(1, 1, windowId, DisposableType::PLAIN_TEXT, 1, observer2);
    DisposableInfo info3(1, 1, windowId, DisposableType::MAX, 1, observer3);
    DisposableInfo info4(1, 1, windowId, DisposableType::PLAIN_TEXT, 1, observer4);

    WindowManager::SetFocusWindowId(windowId);
    DisposableManager::GetInstance().disposableInfoList_ = {info1, info2, info3, info4};
    bool ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
    EXPECT_TRUE(ret);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(observer2->errCode_, IPasteboardDisposableObserver::ERR_NO_PERMISSION);
    EXPECT_EQ(observer3->errCode_, IPasteboardDisposableObserver::ERR_TYPE_NOT_SUPPORT);
    EXPECT_EQ(observer4->errCode_, IPasteboardDisposableObserver::ERR_NO_TEXT);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: TryProcessDisposableDataTest003
 * @tc.desc: should callback ERR_LENGTH_MISMATCH without text when paste data text length > maxLen
 *           else should callback ERR_OK with text
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, TryProcessDisposableDataTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillRepeatedly(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    int32_t windowId = 1;
    std::string text = "123456";
    PasteData pasteData;
    pasteData.AddTextRecord(text);
    sptr<DisposableObserverImpl> observer1 = sptr<DisposableObserverImpl>::MakeSptr();
    sptr<DisposableObserverImpl> observer2 = sptr<DisposableObserverImpl>::MakeSptr();
    DisposableInfo info1(1, 1, windowId, DisposableType::PLAIN_TEXT, text.length() - 1, observer1);
    DisposableInfo info2(1, 1, windowId, DisposableType::PLAIN_TEXT, text.length() + 1, observer2);

    WindowManager::SetFocusWindowId(windowId);
    DisposableManager::GetInstance().disposableInfoList_ = {info1, info2};
    bool ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
    EXPECT_TRUE(ret);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_EQ(observer1->errCode_, IPasteboardDisposableObserver::ERR_LENGTH_MISMATCH);
    EXPECT_EQ(observer2->errCode_, IPasteboardDisposableObserver::ERR_OK);
    EXPECT_STREQ(observer2->text_.c_str(), text.c_str());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: TryProcessDisposableDataTest004
 * @tc.desc: should callback ERR_DATA_IN_APP without text when paste data with ShareOption::InApp
 *           should callback ERR_OK with text when paste data with ShareOption::LocalDevice
 *           should callback ERR_OK with text when paste data with ShareOption::CrossDevice
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, TryProcessDisposableDataTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    NiceMock<Security::AccessToken::AccessTokenKitMock> accessTokenMock;
    EXPECT_CALL(accessTokenMock, VerifyAccessToken)
        .WillRepeatedly(testing::Return(Security::AccessToken::PermissionState::PERMISSION_GRANTED));

    int32_t windowId = 1;
    std::string text = "123456";
    PasteData pasteData;
    pasteData.AddTextRecord(text);
    WindowManager::SetFocusWindowId(windowId);

    size_t loopCnt = 3;
    ShareOption shareOptions[] = {ShareOption::InApp, ShareOption::LocalDevice, ShareOption::CrossDevice};
    int32_t errCodes[] = {IPasteboardDisposableObserver::ERR_DATA_IN_APP, IPasteboardDisposableObserver::ERR_OK,
        IPasteboardDisposableObserver::ERR_OK};
    std::string texts[] = {"", text, text};

    for (size_t i = 0; i < loopCnt; ++i) {
        pasteData.SetShareOption(shareOptions[i]);

        sptr<DisposableObserverImpl> observer = sptr<DisposableObserverImpl>::MakeSptr();
        DisposableInfo info(1, 1, windowId, DisposableType::PLAIN_TEXT, text.length(), observer);

        DisposableManager::GetInstance().disposableInfoList_ = {info};
        bool ret = DisposableManager::GetInstance().TryProcessDisposableData(pasteData, nullptr, nullptr);
        EXPECT_TRUE(ret);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        EXPECT_EQ(observer->errCode_, errCodes[i]);
        EXPECT_STREQ(observer->text_.c_str(), texts[i].c_str());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: GetPlainTextTest001
 * @tc.desc: get text from delay getter
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, GetPlainTextTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    PasteData pasteData;
    pasteData.SetDelayData(true);
    std::string text = DisposableManager::GetInstance().GetPlainText(pasteData, nullptr, nullptr);
    EXPECT_STREQ(text.c_str(), "");

    sptr<DelayGetterImpl> delayGetter = sptr<DelayGetterImpl>::MakeSptr();
    delayGetter->text_ = "123456";
    text = DisposableManager::GetInstance().GetPlainText(pasteData, delayGetter, nullptr);
    EXPECT_STREQ(text.c_str(), delayGetter->text_.c_str());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}

/**
 * @tc.name: GetPlainTextTest002
 * @tc.desc: get text from entry getter
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDisposableManagerTest, GetPlainTextTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
    constexpr size_t recordNum = 5;
    std::string text1 = "abcdefghijk";
    std::string text2 = "123456";
    std::string text3 = text2 + text1;

    std::string textUtdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::UDType::PLAIN_TEXT);
    std::string htmlUtdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::UDType::HTML);
    PasteDataRecord records[recordNum];
    records[0].AddEntry(textUtdId, std::make_shared<PasteDataEntry>(textUtdId, nullptr));
    records[0].AddEntry(htmlUtdId, std::make_shared<PasteDataEntry>(htmlUtdId, nullptr));
    records[1].AddEntry(textUtdId, std::make_shared<PasteDataEntry>(textUtdId, nullptr));
    records[2].AddEntry(textUtdId, std::make_shared<PasteDataEntry>(textUtdId, nullptr));
    records[3].AddEntry(textUtdId, std::make_shared<PasteDataEntry>(textUtdId, text1));
    records[4].AddEntry(htmlUtdId, std::make_shared<PasteDataEntry>(htmlUtdId, text1));
    PasteData pasteData;
    pasteData.SetDelayRecord(true);
    for (auto &record : records) {
        pasteData.AddRecord(record);
    }
    std::reverse(pasteData.records_.begin(), pasteData.records_.end());

    std::string text = DisposableManager::GetInstance().GetPlainText(pasteData, nullptr, nullptr);
    EXPECT_STREQ(text.c_str(), text1.c_str());

    sptr<EntryGetterImpl> entryGetter = sptr<EntryGetterImpl>::MakeSptr();
    entryGetter->text_ = text2;
    text = DisposableManager::GetInstance().GetPlainText(pasteData, nullptr, entryGetter);
    EXPECT_STREQ(text.c_str(), text3.c_str());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, " start");
}
} // namespace OHOS::MiscServices
