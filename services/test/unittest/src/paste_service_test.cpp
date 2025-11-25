/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <iservice_registry.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "message_parcel_warp.h"
#include "os_account_manager.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_observer_callback.h"
#include "pasteboard_service_loader.h"
#include "permission_state_full.h"
#include "pixel_map.h"
#include "token_setproc.h"
#include "uri.h"
#include "want.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Security::AccessToken;
constexpr const char *CMD = "hidumper -s 3701 -a --data";
constexpr const uint16_t EACH_LINE_LENGTH = 50;
constexpr const uint16_t TOTAL_LENGTH = 500;
constexpr const int32_t EDM_UID = 3057;
constexpr int32_t MIN_ASHMEM_DATA_SIZE = 32 * 1024;
const uint64_t SYSTEM_APP_MASK = (static_cast<uint64_t>(1) << 32);
std::string g_webviewPastedataTag = "WebviewPasteDataTag";
class PasteboardServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static bool ExecuteCmd(std::string &result);

    static void AllocTestAppTokenId();
    static void DeleteTestTokenId();
    static void RestoreSelfTokenId();
    static void CommonTest(PasteData &oldPasteData, PasteData &newPasteData);
    static sptr<PasteboardObserver> pasteboardObserver_;
    static sptr<PasteboardObserver> pasteboardEventObserver_;
    static std::atomic_bool pasteboardChangedFlag_;
    static std::atomic_int32_t pasteboardEventStatus_;
    static uint64_t selfTokenId_;
    static AccessTokenID testAppTokenId_;
};
std::atomic_bool PasteboardServiceTest::pasteboardChangedFlag_ = false;
std::atomic_int32_t PasteboardServiceTest::pasteboardEventStatus_ = -1;
sptr<PasteboardObserver> PasteboardServiceTest::pasteboardObserver_ = nullptr;
sptr<PasteboardObserver> PasteboardServiceTest::pasteboardEventObserver_ = nullptr;
uint64_t PasteboardServiceTest::selfTokenId_ = 0;
AccessTokenID PasteboardServiceTest::testAppTokenId_ = 0;

void PasteboardServiceTest::SetUpTestCase(void)
{
    selfTokenId_ = GetSelfTokenID();
    AllocTestAppTokenId();
}

void PasteboardServiceTest::TearDownTestCase(void)
{
    DeleteTestTokenId();
}

void PasteboardServiceTest::SetUp(void) { }

void PasteboardServiceTest::TearDown(void)
{
    if (PasteboardServiceTest::pasteboardObserver_ != nullptr) {
        PasteboardClient::GetInstance()->RemovePasteboardEventObserver(PasteboardServiceTest::pasteboardObserver_);
    }
    if (PasteboardServiceTest::pasteboardEventObserver_ != nullptr) {
        PasteboardClient::GetInstance()->RemovePasteboardEventObserver(PasteboardServiceTest::pasteboardEventObserver_);
    }
    PasteboardClient::GetInstance()->Clear();
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "TearDown.");
}

void PasteboardObserverCallback::OnPasteboardChanged()
{
    PasteboardServiceTest::pasteboardChangedFlag_ = true;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "test changed callback.");
}

void PasteboardEventObserverCallback::OnPasteboardEvent(const PasteboardChangedEvent &event)
{
    PasteboardServiceTest::pasteboardEventStatus_ = event.status;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "test event callback bundleName: %{public}s,status:%{public}d",
        event.bundleName.c_str(), event.status);
}

bool PasteboardServiceTest::ExecuteCmd(std::string &result)
{
    char buff[EACH_LINE_LENGTH] = { 0x00 };
    char output[TOTAL_LENGTH] = { 0x00 };
    FILE *ptr = nullptr;
    if ((ptr = popen(CMD, "r")) != nullptr) {
        while (fgets(buff, sizeof(buff), ptr) != nullptr) {
            if (strcat_s(output, sizeof(output), buff) != 0) {
                pclose(ptr);
                ptr = nullptr;
                return false;
            }
        }
        pclose(ptr);
        ptr = nullptr;
    } else {
        return false;
    }
    result = std::string(output);
    return true;
}

void PasteboardServiceTest::AllocTestAppTokenId()
{
    HapInfoParams infoParams = { .userID = EDM_UID,
        .bundleName = "ohos.privacy_test.pasteboard",
        .instIndex = 0,
        .appIDDesc = "privacy_test.pasteboard" };
    PermissionStateFull testState = { .permissionName = "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION",
        .isGeneral = true,
        .resDeviceID = { "local" },
        .grantStatus = { PermissionState::PERMISSION_GRANTED },
        .grantFlags = { 1 } };
    HapPolicyParams policyParams = { .apl = APL_NORMAL,
        .domain = "test.domain.pasteboard",
        .permList = {},
        .permStateList = { testState } };

    AccessTokenKit::AllocHapToken(infoParams, policyParams);
    testAppTokenId_ = Security::AccessToken::AccessTokenKit::GetHapTokenID(
        infoParams.userID, infoParams.bundleName, infoParams.instIndex);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "userID = %{public}d, testTokenId = 0x%{public}x.", infoParams.userID,
        testAppTokenId_);
}

void PasteboardServiceTest::DeleteTestTokenId()
{
    AccessTokenKit::DeleteToken(testAppTokenId_);
}

void PasteboardServiceTest::RestoreSelfTokenId()
{
    auto ret = SetSelfTokenID(selfTokenId_);
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "ret = %{public}d!", ret);
}

void PasteboardServiceTest::CommonTest(PasteData &oldPasteData, PasteData &newPasteData)
{
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "start.");
    PasteboardClient::GetInstance()->Clear();
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData != true);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(oldPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData == true);
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGD(PASTEBOARD_MODULE_SERVICE, "end.");
}

string GetTime()
{
    time_t curtime;
    time(&curtime);
    tm *nowtime = localtime(&curtime);
    std::string targetTime = std::to_string(1900 + nowtime->tm_year) + "-" + std::to_string(1 + nowtime->tm_mon) + "-" +
        std::to_string(nowtime->tm_mday) + " " + std::to_string(nowtime->tm_hour) + ":" +
        std::to_string(nowtime->tm_min) + ":" + std::to_string(nowtime->tm_sec);
    return targetTime;
}

/**
 * @tc.name: PasteboardTest001
 * @tc.desc: Create paste board test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteboardTest001, TestSize.Level0)
{
    auto record = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record1");
    ASSERT_TRUE(record != nullptr);
    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(data != nullptr);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get.");
    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == plainText);
}

/**
 * @tc.name: PasteRecordTest001
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest001, TestSize.Level0)
{
    std::string plainText = "helloWorld";
    auto record = PasteboardClient::GetInstance()->CreatePlainTextRecord(plainText);
    ASSERT_TRUE(record != nullptr);
    auto newPlainText = record->GetPlainTextV0();
    ASSERT_TRUE(newPlainText != nullptr);
    ASSERT_TRUE(*newPlainText == plainText);
}

/**
 * @tc.name: PasteRecordTest002
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest002, TestSize.Level0)
{
    std::string htmlText = "<div class='disabled item tip user-programs'>";
    auto record = PasteboardClient::GetInstance()->CreateHtmlTextRecord(htmlText);
    ASSERT_TRUE(record != nullptr);
    auto newHtmlText = record->GetHtmlTextV0();
    ASSERT_TRUE(newHtmlText != nullptr);
    ASSERT_TRUE(*newHtmlText == htmlText);
}

/**
 * @tc.name: PasteRecordTest003
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest003, TestSize.Level0)
{
    using namespace OHOS::AAFwk;
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string key = "id";
    int32_t id = 456;
    Want wantIn = want->SetParam(key, id);
    auto record = PasteboardClient::GetInstance()->CreateWantRecord(want);
    ASSERT_TRUE(record != nullptr);
    auto newWant = record->GetWant();
    ASSERT_TRUE(newWant != nullptr);
    int32_t defaultValue = 333;
    ASSERT_TRUE(newWant->GetIntParam(key, defaultValue) == id);
}

/**
 * @tc.name: PasteRecordTest004
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest004, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto record = PasteboardClient::GetInstance()->CreateUriRecord(uri);
    ASSERT_TRUE(record != nullptr);
    auto newUri = record->GetUriV0();
    ASSERT_TRUE(newUri != nullptr);
    ASSERT_TRUE(newUri->ToString() == uri.ToString());
}

/**
 * @tc.name: PasteRecordTest005
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 * @tc.require: AR000H5GKU
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest005, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, 100, opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    auto newPixelMap = pasteDataRecord->GetPixelMapV0();
    ASSERT_TRUE(newPixelMap != nullptr);
    ImageInfo imageInfo = {};
    newPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == opts.size.height);
    ASSERT_TRUE(imageInfo.size.width == opts.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == opts.pixelFormat);
    pasteDataRecord->ClearPixelMap();
    ASSERT_TRUE(pasteDataRecord->GetPixelMapV0() == nullptr);
}

/**
 * @tc.name: PasteRecordTest006
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 * @tc.require: AR000H5GKU
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest006, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    InitializationOptions opts1 = { { 6, 9 }, PixelFormat::RGB_565, PixelFormat::RGB_565 };
    std::unique_ptr<PixelMap> pixelMap1 = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts1);
    std::shared_ptr<PixelMap> pixelMapIn1 = move(pixelMap1);
    pasteDataRecord = pasteDataRecord->NewPixelMapRecord(pixelMapIn1);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    auto newPixelMap = pasteDataRecord->GetPixelMapV0();
    ASSERT_TRUE(newPixelMap != nullptr);
    ImageInfo imageInfo = {};
    newPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == opts1.size.height);
    ASSERT_TRUE(imageInfo.size.width == opts1.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == opts1.pixelFormat);
}

/**
 * @tc.name: PasteRecordTest007
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest007, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    auto customData = pasteDataRecord->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_TRUE(itemData.size() == 1);
    auto item = itemData.find(mimeType);
    ASSERT_TRUE(item != itemData.end());
    ASSERT_TRUE(item->second == arrayBuffer);
}

/**
 * @tc.name: PasteRecordTest008
 * @tc.desc: Create paste board record test.
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest008, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    std::string mimeType1 = "img/png";
    std::vector<uint8_t> arrayBuffer1(46);
    arrayBuffer1 = { 2, 7, 6, 8, 9 };
    pasteDataRecord = pasteDataRecord->NewKvRecord(mimeType1, arrayBuffer1);
    auto customData = pasteDataRecord->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_TRUE(itemData.size() == 1);
    auto item = itemData.find(mimeType1);
    ASSERT_TRUE(item != itemData.end());
    ASSERT_TRUE(item->second == arrayBuffer1);
}

/**
 * @tc.name: PasteRecordTest009
 * @tc.desc: Create paste board html local url
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest009, TestSize.Level0)
{
    std::string htmlText = "<div class='item'><img data-ohos='clipboard' "
                           "src='file:///com.example.webview/data/storage/el1/base/test.png'></div>";
    auto data = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(data != nullptr);
    data->SetTag(g_webviewPastedataTag);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = newPasteData.GetPrimaryHtml();
    ASSERT_TRUE(record != nullptr);
}

/**
 * @tc.name: PasteRecordTest0010
 * @tc.desc: Create paste board html distributed uri.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest0010, TestSize.Level0)
{
    std::string htmlText = "<div class='item'><img data-ohos='clipboard' "
                           "src='file://com.byy.testdpb/data/storage/el2/distributedfiles/"
                           ".remote_share/data/storage/el2/base/haps/entry/cache/t1.jpg'></div>";
    auto data = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(data != nullptr);
    data->SetTag(g_webviewPastedataTag);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData newPasteData2;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData2);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = newPasteData2.GetPrimaryHtml();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(*record == htmlText);
}

/**
 * @tc.name: PasteRecordTest0011
 * @tc.desc: Create paste board html distributed uri.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest0011, TestSize.Level0)
{
    std::string htmlText = "<div class='item'><img "
                           "src='file://com.byy.testdpb/data/storage/el2/distributedfiles/"
                           ".remote_share/data/storage/el2/base/haps/entry/cache/t1.jpg'></div>";
    auto data = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(data != nullptr);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData newPasteData2;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData2);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = newPasteData2.GetPrimaryHtml();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(*record == htmlText);
}

/**
 * @tc.name: PasteRecordTest0012
 * @tc.desc: Create paste board html local url
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteRecordTest0012, TestSize.Level0)
{
    std::string htmlText = "<div class='item'><img data-ohos='clipboard' "
                           "src='file:///com.example.webview/data/storage/el1/base/test.png'></div>";
    auto data = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(data != nullptr);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = newPasteData.GetPrimaryHtml();
    ASSERT_TRUE(record != nullptr);
}

/**
 * @tc.name: PasteDataTest001
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest001, TestSize.Level0)
{
    using namespace OHOS::AAFwk;
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string key = "id";
    int32_t id = 456;
    Want wantIn = want->SetParam(key, id);
    auto data = PasteboardClient::GetInstance()->CreateWantData(std::make_shared<Want>(wantIn));
    ASSERT_TRUE(data != nullptr);
    PasteboardClient::GetInstance()->Clear();
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has != true);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = pasteData.GetPrimaryWant();
    ASSERT_TRUE(record != nullptr);
    int32_t defaultValue = 333;
    ASSERT_TRUE(record->GetIntParam(key, defaultValue) == id);
}

/**
 * @tc.name: PasteDataTest002
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest002, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto data = PasteboardClient::GetInstance()->CreateUriData(uri);
    ASSERT_TRUE(data != nullptr);
    PasteboardClient::GetInstance()->Clear();
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has != true);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = pasteData.GetPrimaryUri();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(record->ToString() == uri.ToString());
}

/**
 * @tc.name: PasteDataTest003
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest003, TestSize.Level0)
{
    std::string text = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(text);
    ASSERT_TRUE(data != nullptr);
    PasteboardClient::GetInstance()->Clear();
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has != true);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = pasteData.GetPrimaryText();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(*record == text);
}

/**
 * @tc.name: PasteDataTest004
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest004, TestSize.Level0)
{
    std::string htmlText = "<div class='disabled item tip user-programs'>";
    auto data = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(data != nullptr);
    PasteboardClient::GetInstance()->Clear();
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has != true);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = pasteData.GetPrimaryHtml();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(*record == htmlText);
}

/**
 * @tc.name: PasteDataTest005
 * @tc.desc: CreateHtmlData test.
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest005, TestSize.Level0)
{
    std::string htmlText = "<div class='disabled item tip user-programs'>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(pasteData != nullptr);
    std::string plainText = "plain text";
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_HTML);
    std::string mimeType = MIMETYPE_TEXT_PLAIN;
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType1 = "image/jpg";
    std::shared_ptr<MineCustomData> customData = std::make_shared<MineCustomData>();
    customData->AddItemData(mimeType1, arrayBuffer);
    std::shared_ptr<PasteDataRecord> pasteDataRecord = builder.SetMimeType(mimeType)
                                                           .SetPlainText(std::make_shared<std::string>(plainText))
                                                           .SetHtmlText(std::make_shared<std::string>(htmlText))
                                                           .SetCustomData(customData)
                                                           .Build();
    pasteData->AddRecord(pasteDataRecord);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto primaryHtml = newPasteData.GetPrimaryHtml();
    ASSERT_TRUE(primaryHtml != nullptr);
    ASSERT_TRUE(*primaryHtml == htmlText);
    auto firstRecord = newPasteData.GetRecordAt(0);
    ASSERT_TRUE(firstRecord != nullptr);
    ASSERT_TRUE(firstRecord->GetMimeType() == mimeType);
    auto newPlainText = firstRecord->GetPlainTextV0();
    ASSERT_TRUE(newPlainText != nullptr);
    ASSERT_TRUE(*newPlainText == plainText);
    auto newHtmlText = firstRecord->GetHtmlTextV0();
    ASSERT_TRUE(newHtmlText != nullptr);
    ASSERT_TRUE(*newHtmlText == htmlText);
    customData = pasteDataRecord->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_TRUE(itemData.size() == 1);
    auto item = itemData.find(mimeType1);
    ASSERT_TRUE(item != itemData.end());
    ASSERT_TRUE(item->second == arrayBuffer);
}

/**
 * @tc.name: PasteDataTest006
 * @tc.desc: CreatePlainTextData test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest006, TestSize.Level0)
{
    using namespace OHOS::AAFwk;
    std::string plainText = "helloWorld";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string key = "id";
    int32_t id = 456;
    Want wantIn = want->SetParam(key, id);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_WANT);
    std::shared_ptr<PasteDataRecord> pasteDataRecord =
        builder.SetWant(std::make_shared<Want>(wantIn)).SetPlainText(std::make_shared<std::string>(plainText)).Build();
    pasteData->AddRecord(pasteDataRecord);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto firstRecord = newPasteData.GetRecordAt(0);
    ASSERT_TRUE(firstRecord != nullptr);
    ASSERT_TRUE(firstRecord->GetMimeType() == MIMETYPE_TEXT_WANT);
    auto newWant = firstRecord->GetWant();
    ASSERT_TRUE(newWant != nullptr);
    int32_t defaultValue = 333;
    ASSERT_TRUE(newWant->GetIntParam(key, defaultValue) == id);
    auto newPlainText = firstRecord->GetPlainTextV0();
    ASSERT_TRUE(newPlainText != nullptr);
    ASSERT_TRUE(*newPlainText == plainText);
}

/**
 * @tc.name: PasteDataTest007
 * @tc.desc: PixelMap test.
 * @tc.type: FUNC
 * @tc.require: AR000H5GKU
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest007, TestSize.Level0)
{
    std::string htmlText = "<div class='disabled item tip user-programs'>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(htmlText);
    ASSERT_TRUE(pasteData != nullptr);
    OHOS::Uri uri("uri");
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::shared_ptr<PasteDataRecord> pasteDataRecord =
        builder.SetUri(std::make_shared<OHOS::Uri>(uri)).SetPixelMap(pixelMapIn).Build();
    pasteData->AddRecord(pasteDataRecord);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto firstRecord = newPasteData.GetRecordAt(0);
    ASSERT_TRUE(firstRecord != nullptr);
    ASSERT_TRUE(firstRecord->GetMimeType() == MIMETYPE_TEXT_URI);
    auto newUri = firstRecord->GetUriV0();
    ASSERT_TRUE(newUri != nullptr);
    ASSERT_TRUE(newUri->ToString() == uri.ToString());
    auto newPixelMap = firstRecord->GetPixelMapV0();
    ASSERT_TRUE(newPixelMap != nullptr);
    ImageInfo imageInfo = {};
    newPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == opts.size.height);
    ASSERT_TRUE(imageInfo.size.width == opts.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == opts.pixelFormat);
}

/**
 * @tc.name: PasteDataTest008
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 * @tc.require: AR000H5GKU
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest008, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto pasteData = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMapIn);
    ASSERT_TRUE(pasteData != nullptr);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto primaryPixelMap = newPasteData.GetPrimaryPixelMap();
    ASSERT_TRUE(primaryPixelMap != nullptr);
    ImageInfo imageInfo = {};
    primaryPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == opts.size.height);
    ASSERT_TRUE(imageInfo.size.width == opts.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == opts.pixelFormat);
}

/**
 * @tc.name: PasteDataTest009
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 * @tc.require: AR000H5GKU
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest009, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    pasteData->AddPixelMapRecord(pixelMapIn);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto primaryPlainText = newPasteData.GetPrimaryText();
    ASSERT_TRUE(primaryPlainText != nullptr);
    ASSERT_TRUE(*primaryPlainText == plainText);
    auto primaryPixelMap = newPasteData.GetPrimaryPixelMap();
    ASSERT_TRUE(primaryPixelMap != nullptr);
    ImageInfo imageInfo = {};
    primaryPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == opts.size.height);
    ASSERT_TRUE(imageInfo.size.width == opts.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == opts.pixelFormat);
}

/**
 * @tc.name: PasteDataTest0010
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0010, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteData = PasteboardClient::GetInstance()->CreateKvData(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteData != nullptr);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto firstRecord = newPasteData.GetRecordAt(0);
    auto customData = firstRecord->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_TRUE(itemData.size() == 1);
    auto item = itemData.find(mimeType);
    ASSERT_TRUE(item != itemData.end());
    ASSERT_TRUE(item->second == arrayBuffer);
}

/**
 * @tc.name: PasteDataTest0011
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0011, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    pasteData->AddKvRecord(mimeType, arrayBuffer);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto firstRecord = newPasteData.GetRecordAt(0);
    auto customData = firstRecord->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_TRUE(itemData.size() == 1);
    auto item = itemData.find(mimeType);
    ASSERT_TRUE(item != itemData.end());
    ASSERT_TRUE(item->second == arrayBuffer);
    auto primaryPlainText = newPasteData.GetPrimaryText();
    ASSERT_TRUE(primaryPlainText != nullptr);
    ASSERT_TRUE(*primaryPlainText == plainText);
}

/**
 * @tc.name: PasteDataTest0012
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0012, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    pasteData->AddKvRecord(mimeType, arrayBuffer);
    auto record = pasteData->GetRecordAt(0);
    ASSERT_TRUE(record != nullptr);
    std::string mimeType1 = "img/png";
    std::vector<uint8_t> arrayBuffer1(54);
    arrayBuffer1 = { 4, 7, 9, 8, 7 };
    auto customData = record->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    customData->AddItemData(mimeType1, arrayBuffer1);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    auto firstRecord = newPasteData.GetRecordAt(0);
    ASSERT_TRUE(firstRecord != nullptr);
    customData = firstRecord->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_EQ(itemData.size(), 1);
    auto item = itemData.find(mimeType);
    ASSERT_TRUE(item != itemData.end());
    ASSERT_TRUE(item->second == arrayBuffer);
    item = itemData.find(mimeType1);
    ASSERT_TRUE(item == itemData.end());
    auto primaryPlainText = newPasteData.GetPrimaryText();
    ASSERT_TRUE(primaryPlainText != nullptr);
    ASSERT_TRUE(*primaryPlainText == plainText);
    auto secondRecord = newPasteData.GetRecordAt(1);
    ASSERT_TRUE(secondRecord != nullptr);
    auto secondRecordMimeType = secondRecord->GetMimeType();
    ASSERT_TRUE(secondRecordMimeType == MIMETYPE_TEXT_PLAIN);
}

/**
 * @tc.name: PasteDataTest0014
 * @tc.desc: Create paste board data test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0014, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    auto shareOption = pasteData->GetShareOption();
    ASSERT_TRUE(shareOption == ShareOption::CrossDevice);
    pasteData->SetShareOption(ShareOption::InApp);
    auto tokenId = pasteData->GetTokenId();
    ASSERT_TRUE(tokenId == 0);
    pasteData->SetTokenId(1);
    PasteData newPasteData;
    PasteboardServiceTest::CommonTest(*pasteData, newPasteData);
    shareOption = newPasteData.GetShareOption();
    ASSERT_TRUE(shareOption == ShareOption::InApp);
    tokenId = pasteData->GetTokenId();
    ASSERT_TRUE(tokenId != 0);
}

/**
 * @tc.name: PasteDataTest0015
 * @tc.desc: isLocalPaste test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0015, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    auto isLocalPaste = pasteData->IsLocalPaste();
    ASSERT_FALSE(isLocalPaste);
    pasteData->SetLocalPasteFlag(true);
    isLocalPaste = pasteData->IsLocalPaste();
    ASSERT_TRUE(isLocalPaste);
    pasteData->SetLocalPasteFlag(false);
    isLocalPaste = pasteData->IsLocalPaste();
    ASSERT_FALSE(isLocalPaste);
    PasteboardClient::GetInstance()->Clear();
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    isLocalPaste = newPasteData.IsLocalPaste();
    ASSERT_TRUE(isLocalPaste);
}

/**
 * @tc.name: PasteDataTest0016
 * @tc.desc: RemoveRecordAt HasMimeType test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0016, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    pasteData->AddKvRecord(mimeType, arrayBuffer);
    pasteData->RemoveRecordAt(1);
    PasteboardClient::GetInstance()->Clear();
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_TRUE(newPasteData.HasMimeType(mimeType));
    ASSERT_TRUE(newPasteData.GetRecordCount() == 1);
    auto record = newPasteData.GetRecordAt(0);
    ASSERT_TRUE(record != nullptr);
    auto customData = record->GetCustomData();
    ASSERT_TRUE(customData != nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_EQ(itemData.size(), 1);
    auto item = itemData.find(mimeType);
    ASSERT_TRUE(item != itemData.end());
    EXPECT_EQ(item->second, arrayBuffer);
}

/**
 * @tc.name: PasteDataTest0017
 * @tc.desc: ReplaceRecordAt GetProperty GetTag test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0017, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto pasteData = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMapIn);
    ASSERT_TRUE(pasteData != nullptr);
    auto pixelMap1 = pasteData->GetPrimaryPixelMap();
    ASSERT_TRUE(pixelMap1 != nullptr);
    ImageInfo imageInfo = {};
    pixelMap1->GetImageInfo(imageInfo);
    ASSERT_EQ(imageInfo.size.height, opts.size.height);
    ASSERT_EQ(imageInfo.size.width, opts.size.width);
    ASSERT_EQ(imageInfo.pixelFormat, opts.pixelFormat);
    std::string plainText = "plain text";
    auto record = PasteboardClient::GetInstance()->CreatePlainTextRecord(plainText);
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(pasteData->ReplaceRecordAt(0, record));
    PasteboardClient::GetInstance()->Clear();
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_EQ(newPasteData.GetRecordCount(), 1);
    auto record1 = newPasteData.GetRecordAt(0);
    ASSERT_TRUE(record1 != nullptr);
    auto plainText1 = record1->GetPlainTextV0();
    ASSERT_TRUE(plainText1 != nullptr);
    EXPECT_EQ(*plainText1, plainText);
    auto property = newPasteData.GetProperty();
    EXPECT_TRUE(property.additions.IsEmpty());
    EXPECT_EQ(property.mimeTypes.size(), 1);
    EXPECT_EQ(property.mimeTypes[0], MIMETYPE_TEXT_PLAIN);
    EXPECT_TRUE(property.tag.empty());
    EXPECT_EQ(property.shareOption, ShareOption::CrossDevice);
    EXPECT_TRUE(property.tokenId != 0);
    auto tag = newPasteData.GetTag();
    EXPECT_TRUE(tag.empty());
}

/**
 * @tc.name: PasteDataTest0018
 * @tc.desc: AddPasteboardChangedObserver RemovePasteboardChangedObserver OnRemoteDied OnRemoteSaDied test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0018, TestSize.Level0)
{
    PasteboardClient::GetInstance()->RemovePasteboardChangedObserver(nullptr);
    if (PasteboardServiceTest::pasteboardObserver_ == nullptr) {
        PasteboardServiceTest::pasteboardObserver_ = sptr<PasteboardObserverCallback>::MakeSptr();
    }
    PasteboardServiceTest::pasteboardChangedFlag_ = false;
    ASSERT_TRUE(PasteboardServiceTest::pasteboardObserver_ != nullptr);
    PasteboardClient::GetInstance()->AddPasteboardChangedObserver(PasteboardServiceTest::pasteboardObserver_);
    ASSERT_FALSE(PasteboardServiceTest::pasteboardChangedFlag_);
    const wptr<IRemoteObject> object;
    PasteboardSaDeathRecipient death;
    death.OnRemoteDied(object);
    PasteboardServiceLoader::GetInstance().OnRemoteSaDied(object);
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto pasteData = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMapIn);
    ASSERT_TRUE(pasteData != nullptr);
    PasteboardClient::GetInstance()->Clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(PasteboardServiceTest::pasteboardChangedFlag_);
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_TRUE(PasteboardServiceTest::pasteboardChangedFlag_);
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    PasteboardServiceTest::pasteboardChangedFlag_ = false;
    PasteboardClient::GetInstance()->RemovePasteboardChangedObserver(PasteboardServiceTest::pasteboardObserver_);
    PasteboardClient::GetInstance()->Clear();
    ASSERT_FALSE(PasteboardServiceTest::pasteboardChangedFlag_);
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_FALSE(PasteboardServiceTest::pasteboardChangedFlag_);
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
}

/**
 * @tc.name: PasteDataTest0019
 * @tc.desc: AddPasteboardEventObserver RemovePasteboardEventObserver test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0019, TestSize.Level0)
{
    PasteboardClient::GetInstance()->RemovePasteboardEventObserver(nullptr);
    if (PasteboardServiceTest::pasteboardEventObserver_ == nullptr) {
        PasteboardServiceTest::pasteboardEventObserver_ = sptr<PasteboardEventObserverCallback>::MakeSptr();
    }
    PasteboardServiceTest::pasteboardEventStatus_ = -1;
    ASSERT_TRUE(PasteboardServiceTest::pasteboardEventObserver_ != nullptr);
    PasteboardClient::GetInstance()->AddPasteboardEventObserver(PasteboardServiceTest::pasteboardEventObserver_);
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData("hello");
    ASSERT_TRUE(pasteData != nullptr);
    PasteboardClient::GetInstance()->Clear();
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
    PasteboardClient::GetInstance()->Clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
    PasteboardServiceTest::pasteboardEventStatus_ = -1;
    PasteboardClient::GetInstance()->RemovePasteboardEventObserver(PasteboardServiceTest::pasteboardEventObserver_);
    PasteboardClient::GetInstance()->Clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
    PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_EQ(PasteboardServiceTest::pasteboardEventStatus_, -1);
}

/**
 * @tc.name: PasteDataTest0020
 * @tc.desc: Create paste board test set bundleName and time.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0020, TestSize.Level0)
{
    std::string text = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(text);
    ASSERT_TRUE(pasteData != nullptr);
    std::string bundleName = "ohos.acts.distributeddatamgr.pasteboard";
    pasteData->SetBundleInfo(bundleName, 0);
    std::string time = GetTime();
    pasteData->SetTime(time);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto has = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(has == true);
}

/**
 * @tc.name: PasteDataTest0021
 * @tc.desc: AddPasteboardEventObserver RemovePasteboardEventObserver test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0021, TestSize.Level0)
{
    PasteboardClient::GetInstance()->AddPasteboardEventObserver(sptr<PasteboardEventObserverCallback>::MakeSptr());
    PasteboardClient::GetInstance()->AddPasteboardEventObserver(sptr<PasteboardEventObserverCallback>::MakeSptr());
    PasteboardClient::GetInstance()->AddPasteboardEventObserver(sptr<PasteboardEventObserverCallback>::MakeSptr());
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData("hello");
    ASSERT_TRUE(pasteData != nullptr);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto hasData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasData == true);
    PasteData newPasteData;
    PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    PasteboardClient::GetInstance()->Clear();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    PasteboardClient::GetInstance()->RemovePasteboardEventObserver(nullptr);
    hasData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasData == false);
}

/**
 * @tc.name: PasteDataTest0022
 * @tc.desc: isDraggedData test.
 * @tc.type: FUNC
 * @tc.require: AROOOH5R5G
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0022, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    auto isDraggedData = pasteData->IsDraggedData();
    ASSERT_FALSE(isDraggedData);
    pasteData->SetDraggedDataFlag(true);
    ASSERT_TRUE(pasteData->IsDraggedData());
    PasteboardClient::GetInstance()->Clear();
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    PasteData newPasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(newPasteData);
    ASSERT_FALSE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    isDraggedData = newPasteData.IsDraggedData();
    ASSERT_FALSE(isDraggedData);
}

/**
 * @tc.name: PasteDataTest0023
 * @tc.desc: paste pastedata with 3 records
 * @tc.type: FUNC
 * @tc.require: RR2025020589956
 * @tc.author: wangchenghao
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0023, TestSize.Level0)
{
    std::string text = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(text);
    ASSERT_TRUE(data != nullptr);
    auto record1 = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record1");
    auto record2 = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record2");
    data->AddRecord(record1);
    data->AddRecord(record2);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = pasteData.GetPrimaryText();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(*record == "paste record2");
}

/**
 * @tc.name: PasteDataTest0024
 * @tc.desc: paste pastedata with 5 records
 * @tc.type: FUNC
 * @tc.require: RR2025020589956
 * @tc.author: wangchenghao
 */
HWTEST_F(PasteboardServiceTest, PasteDataTest0024, TestSize.Level0)
{
    std::string text = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(text);
    ASSERT_TRUE(data != nullptr);
    auto record1 = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record1");
    auto record2 = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record2");
    auto record3 = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record3");
    auto record4 = PasteboardClient::GetInstance()->CreatePlainTextRecord("paste record4");
    data->AddRecord(record1);
    data->AddRecord(record2);
    data->AddRecord(record3);
    data->AddRecord(record4);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*data);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    PasteData pasteData;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto record = pasteData.GetPrimaryText();
    ASSERT_TRUE(record != nullptr);
    ASSERT_TRUE(*record == "paste record4");
}

/**
 * @tc.name: BigPixelMap001
 * @tc.desc: paste big pixel map image
 * @tc.type: FUNC
 * @tc.require:AR000H5I1D
 * @tc.author: baoyayong
 */
HWTEST_F(PasteboardServiceTest, BigPixelMap001, TestSize.Level1)
{
    constexpr uint32_t COLOR_SIZE = 1024 * 1960;
    auto color = std::make_unique<uint32_t[]>(COLOR_SIZE);
    InitializationOptions opts = { { 1024, 1960 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color.get(), COLOR_SIZE, opts);
    auto pasteData1 = PasteboardClient::GetInstance()->CreatePixelMapData(std::move(pixelMap));
    PasteboardClient::GetInstance()->Clear();
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_FALSE(hasPasteData);
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData1);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    ASSERT_TRUE(hasPasteData);
    PasteData pasteData2;
    ret = PasteboardClient::GetInstance()->GetPasteData(pasteData2);
    ASSERT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto pixelMap2 = pasteData2.GetPrimaryPixelMap();
    ASSERT_TRUE(pixelMap2 != nullptr);
    ImageInfo imageInfo {};
    pixelMap2->GetImageInfo(imageInfo);
    EXPECT_TRUE(imageInfo.size.height == opts.size.height);
    EXPECT_TRUE(imageInfo.size.width == opts.size.width);
    EXPECT_TRUE(imageInfo.pixelFormat == opts.pixelFormat);
}

/**
 * @tc.name: GetPastedataFail001
 * @tc.desc: get paste data fail - SetValue()
 * @tc.type: FUNC
 * @tc.require: issuesI5WPTM
 * @tc.author: chenyu
 */
HWTEST_F(PasteboardServiceTest, GetPastedataFail001, TestSize.Level1)
{
    PasteboardClient::GetInstance()->Clear();
    PasteData data;
    auto ret = PasteboardClient::GetInstance()->GetPasteData(data);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::NO_DATA_ERROR));
}

/**
 * @tc.name: DumpDataTest001
 * @tc.desc: DumpData()-remote, CrossDevice
 * @tc.type: FUNC
 * @tc.require: issueshI5YDEV
 * @tc.author: chenyu
 */
HWTEST_F(PasteboardServiceTest, DumpDataTest001, TestSize.Level1)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    pasteData->SetRemote(true);
    pasteData->SetShareOption(ShareOption::CrossDevice);
    PasteboardClient::GetInstance()->Clear();
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    std::string result;
    ret = PasteboardServiceTest::ExecuteCmd(result);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(result.find("CrossDevice") != std::string::npos);
    EXPECT_TRUE(result.find("remote") != std::string::npos);
    PasteboardClient::GetInstance()->Clear();
}

/**
 * @tc.name: DumpDataTest002
 * @tc.desc: DumpData()-local, LocalDevice
 * @tc.type: FUNC
 * @tc.require: issueshI5YDEV
 * @tc.author: chenyu
 */
HWTEST_F(PasteboardServiceTest, DumpDataTest002, TestSize.Level1)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    pasteData->SetShareOption(ShareOption::LocalDevice);
    PasteboardClient::GetInstance()->Clear();
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    std::string result;
    ret = PasteboardServiceTest::ExecuteCmd(result);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(result.find("LocalDevice") != std::string::npos);
    EXPECT_TRUE(result.find("local") != std::string::npos);
    PasteboardClient::GetInstance()->Clear();
}

/**
 * @tc.name: DumpDataTest003
 * @tc.desc: DumpData()-local, InApp
 * @tc.type: FUNC
 * @tc.require: issueshI5YDEV
 * @tc.author: chenyu
 */
HWTEST_F(PasteboardServiceTest, DumpDataTest003, TestSize.Level1)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    pasteData->SetShareOption(ShareOption::InApp);
    PasteboardClient::GetInstance()->Clear();
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    std::string result;
    ret = PasteboardServiceTest::ExecuteCmd(result);
    EXPECT_TRUE(ret);
    EXPECT_TRUE(result.find("InAPP") != std::string::npos);
    EXPECT_TRUE(result.find("local") != std::string::npos);
    PasteboardClient::GetInstance()->Clear();
}

/**
 * @tc.name: DumpDataTest004
 * @tc.desc: DumpData()-no data
 * @tc.type: FUNC
 * @tc.require: issueshI5YDEV
 * @tc.author: chenyu
 */
HWTEST_F(PasteboardServiceTest, DumpDataTest004, TestSize.Level1)
{
    PasteboardClient::GetInstance()->Clear();

    std::string result;
    auto ret = PasteboardServiceTest::ExecuteCmd(result);
    EXPECT_TRUE(ret);
    EXPECT_EQ(result.find("Share"), std::string::npos);
    EXPECT_EQ(result.find("Option"), std::string::npos);
}

/**
 * @tc.name: HasPasteDataTest001
 * @tc.desc: if !pasteData->IsDraggedData()
 * @tc.type: FUNC
 * @tc.require: issueshI5YDEV
 * @tc.author: chenyu
 */
HWTEST_F(PasteboardServiceTest, HasPasteDataTest001, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteData = PasteboardClient::GetInstance()->CreateKvData(mimeType, arrayBuffer);
    PasteboardClient::GetInstance()->Clear();
    int32_t ret = PasteboardClient::GetInstance()->SetPasteData(*pasteData);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::E_OK));
    auto hasPasteData = PasteboardClient::GetInstance()->HasPasteData();
    // not DraggedData, not DefaultIME
    EXPECT_TRUE(hasPasteData);
    PasteboardClient::GetInstance()->Clear();
}

/**
 * @tc.name: SetAppShareOptions
 * @tc.desc: set app share options
 * @tc.type: FUNC
 * @tc.require: issuesIA7V62
 * @tc.author: caozhijun
 */
HWTEST_F(PasteboardServiceTest, SetAppShareOptions, TestSize.Level0)
{
    uint64_t tempTokenID = testAppTokenId_ | SYSTEM_APP_MASK;
    auto result = SetSelfTokenID(tempTokenID);
    AccessTokenKit::GrantPermission(
        tempTokenID, "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION", PERMISSION_USER_SET);
    PASTEBOARD_HILOGD(
        PASTEBOARD_MODULE_SERVICE, "testTokenId= 0x%{public}x, ret= %{public}d!", testAppTokenId_, result);
    ShareOption setting = ShareOption::InApp;
    int32_t ret = PasteboardClient::GetInstance()->SetAppShareOptions(setting);
    EXPECT_TRUE(ret == 0);
    ret = PasteboardClient::GetInstance()->SetAppShareOptions(setting);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::INVALID_OPERATION_ERROR));
    ret = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    EXPECT_TRUE(ret == 0);
    ret = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    EXPECT_TRUE(ret == 0);

    setuid(EDM_UID);
    std::map<uint32_t, ShareOption> globalShareOptions;
    setting = ShareOption::InApp;
    globalShareOptions.insert({tempTokenID, setting});
    PasteboardClient::GetInstance()->SetGlobalShareOption(globalShareOptions);
    ret = PasteboardClient::GetInstance()->SetAppShareOptions(setting);
    EXPECT_TRUE(ret == static_cast<int32_t>(PasteboardError::INVALID_OPERATION_ERROR));
    ret = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    EXPECT_TRUE(ret == 0);
    std::vector<uint32_t> tokenIds;
    tokenIds.push_back(tempTokenID);
    PasteboardClient::GetInstance()->RemoveGlobalShareOption(tokenIds);

    ret = PasteboardClient::GetInstance()->SetAppShareOptions(setting);
    EXPECT_TRUE(ret == 0);
    PasteboardClient::GetInstance()->SetGlobalShareOption(globalShareOptions);
    ret = PasteboardClient::GetInstance()->RemoveAppShareOptions();
    EXPECT_TRUE(ret == 0);
    PasteboardServiceTest::RestoreSelfTokenId();
}

} // namespace OHOS::MiscServices