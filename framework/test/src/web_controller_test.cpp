/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "pasteboard_client.h"
#include "pasteboard_hilog.h"
#include "pasteboard_img_extractor.h"
#include "pasteboard_web_controller.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
class WebControllerTest : public testing::Test {
public:
    WebControllerTest() {};
    ~WebControllerTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void WebControllerTest::SetUpTestCase(void) { }

void WebControllerTest::TearDownTestCase(void) { }

void WebControllerTest::SetUp(void) { }

void WebControllerTest::TearDown(void) { }

EntryValue GetHtmlValue()
{
    return EntryValue(std::in_place_type<std::string>, "<html><body>Test</body></html>");
}
/**
 * @tc.name: UpdateHtmlRecordTest_001.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, UpdateHtmlRecordTest_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UpdateHtmlRecordTest_001 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteDataRecord> htmlRecord = nullptr;
    std::shared_ptr<std::string> htmlData = std::make_shared<std::string>("test data");

    webClipboardController.UpdateHtmlRecord(htmlRecord, htmlData);
    ASSERT_EQ(htmlRecord, nullptr);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UpdateHtmlRecordTest_001 start");
}

/**
 * @tc.name: UpdateHtmlRecordTest_002.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, UpdateHtmlRecordTest_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UpdateHtmlRecordTest_002 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteDataRecord> htmlRecord = std::make_shared<PasteDataRecord>();
    std::shared_ptr<std::string> htmlData = nullptr;
    webClipboardController.UpdateHtmlRecord(htmlRecord, htmlData);
    ASSERT_EQ(htmlData, nullptr);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UpdateHtmlRecordTest_002 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_001.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_001 start");

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = webClipboardController.SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_001 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_002.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_002 start");
    PasteData pasteData;
    PasteDataRecord record;
    record.SetRecordId(1);
    record.SetFrom(1);
    pasteData.AddRecord(std::make_shared<PasteDataRecord>(record));
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_002 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_003.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_003 start");
    PasteData pasteData;
    PasteDataRecord record;
    record.SetRecordId(1);
    record.SetFrom(2);
    pasteData.AddRecord(std::make_shared<PasteDataRecord>(record));
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_003 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_004.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_004 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("");
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_004 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_005.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_005 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<p>Hello world!<img src=\">file:///storage/local/files/Images/hello.png\"/><p>");
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_005 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_006.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_006 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<p>Hello world!<img src=\">https://storage/local/files/Images/hello.png\"/><p>");
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_006 start");
}

/**
* @tc.name: SplitWebviewPasteDataTest_007.
* @tc.desc:SplitWebviewPasteData_ShouldReturnTrue_WhenHtmlEntryIsValid
* @tc.type: FUNC.
* @tc.require:
* @tc.author:
*/
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_007 start");
    PasteData pasteData;
    pasteData.AddHtmlRecord("<p>Hello world!<img src=\"file:///storage/local/files/Images/hello.png\"/><p>");
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    EXPECT_NE(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_007 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_008.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_008 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    record->SetFrom(1);
    record->SetRecordId(1);
    pasteData.AddRecord(record);
    std::string bundleIndex;
    int32_t userId = 100;
    bool result = webClipboardController.SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_008 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_009.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_009 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    record->SetFrom(1);
    record->SetRecordId(1);
    auto htmlEntry = std::make_shared<PasteDataEntry>();
    htmlEntry->SetMimeType(MIMETYPE_TEXT_HTML);

    EntryValue htmlValue(std::in_place_index<4>, "<html><body>Hello</body></html>");
    htmlEntry->SetValue(htmlValue);

    record->AddEntry("text/html", htmlEntry);
    pasteData.AddRecord(record);

    std::string bundleIndex;
    int32_t userId = 100;
    bool result = webClipboardController.SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_009 start");
}

/**
 * @tc.name: SplitWebviewPasteDataTest_010.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitWebviewPasteDataTest_010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_010 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    record->SetFrom(1);
    record->SetRecordId(1);
    auto htmlEntry = std::make_shared<PasteDataEntry>();
    htmlEntry->SetMimeType(MIMETYPE_TEXT_HTML);

    EntryValue htmlValue(std::in_place_index<4>, "");
    htmlEntry->SetValue(htmlValue);

    record->AddEntry("text/html", htmlEntry);
    pasteData.AddRecord(record);

    std::string bundleIndex;
    int32_t userId = 100;
    bool result = webClipboardController.SplitWebviewPasteData(pasteData, bundleIndex, userId);
    EXPECT_FALSE(result);
    EXPECT_NE(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitWebviewPasteDataTest_010 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_001.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_001 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);
    ASSERT_EQ(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_001 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_002.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_002 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_002 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_003.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_003 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag("INVALID_TAG");
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_003 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_004.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_004 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = std::make_shared<PasteDataRecord>();
    record->SetUri(nullptr);
    pasteData.AddRecord(record);
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_004 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_005.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_005 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = std::make_shared<PasteDataRecord>();
    record->SetUri(std::make_shared<OHOS::Uri>("content://local/image.jpg"));
    record->SetFrom(1);
    record->SetRecordId(2);

    pasteData.AddRecord(record);
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_005 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_006.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_006 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = std::make_shared<PasteDataRecord>();
    record->SetUri(std::make_shared<OHOS::Uri>("content://local/docs/image.jpg"));
    record->SetFrom(1);
    record->SetRecordId(2);

    pasteData.AddRecord(record);
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_006 start");
}

/**
 * @tc.name: SetWebviewPasteDataTest_007.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SetWebviewPasteDataTest_007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_007 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = std::make_shared<PasteDataRecord>();
    record->SetUri(std::make_shared<OHOS::Uri>("content://local/images/image.jpg"));
    record->SetFrom(1);
    record->SetRecordId(2);

    pasteData.AddRecord(record);
    std::string bundleIndex;
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWebviewPasteDataTest_007 end");
}

/**
 * @tc.name: CheckAppUriPermissionTest_001.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, CheckAppUriPermissionTest_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_001 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_001 end");
}

/**
 * @tc.name: CheckAppUriPermissionTest_002.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, CheckAppUriPermissionTest_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_002 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.AddRecord(nullptr);
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_002 end");
}

/**
 * @tc.name: CheckAppUriPermissionTest_003.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, CheckAppUriPermissionTest_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_003 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    auto fileUri = std::make_shared<OHOS::Uri>("file://test");
    record->SetUri(fileUri);
    pasteData.AddRecord(record);
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_003 end");
}

/**
 * @tc.name: CheckAppUriPermissionTest_004.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, CheckAppUriPermissionTest_004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_004 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto validRecord = std::make_shared<PasteDataRecord>();
    auto fileUri = std::make_shared<OHOS::Uri>("file://test1");
    validRecord->SetUri(fileUri);
    pasteData.AddRecord(validRecord);

    auto invaliRecord = std::make_shared<PasteDataRecord>();
    validRecord->SetUri(nullptr);
    pasteData.AddRecord(invaliRecord);
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CheckAppUriPermissionTest_004 end");
}

/**
 * @tc.name: GetNeedCheckUrisTest_001.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, GetNeedCheckUrisTest_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNeedCheckUrisTest_001 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto validRecord = std::make_shared<PasteDataRecord>();
    auto fileUri = std::make_shared<OHOS::Uri>("file://test1");
    validRecord->SetUri(fileUri);
    pasteData.AddRecord(validRecord);

    auto invaliRecord = std::make_shared<PasteDataRecord>();
    pasteData.AddRecord(invaliRecord);

    std::vector<std::string> uris;
    std::vector<size_t> indexs;
    size_t uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, false);
    EXPECT_EQ(uriCount, 1);
    EXPECT_EQ(uris.size(), 1);
    EXPECT_EQ(indexs.size(), 1);

    uris.clear();
    indexs.clear();
    uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, true);
    EXPECT_EQ(uriCount, 1);
    EXPECT_EQ(uris.size(), 0);
    EXPECT_EQ(indexs.size(), 0);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNeedCheckUrisTest_001 end");
}

/**
 * @tc.name: GetNeedCheckUrisTest_002.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, GetNeedCheckUrisTest_002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNeedCheckUrisTest_002 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record1 = std::make_shared<PasteDataRecord>();
    record1->SetUri(nullptr);
    pasteData.AddRecord(record1);

    auto record2 = std::make_shared<PasteDataRecord>();
    record2->SetUri(nullptr);
    pasteData.AddRecord(record2);

    std::vector<std::string> uris;
    std::vector<size_t> indexs;
    size_t uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, false);
    EXPECT_EQ(uriCount, 0);
    EXPECT_EQ(uris.size(), 0);
    EXPECT_EQ(indexs.size(), 0);

    uris.clear();
    indexs.clear();
    uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, true);
    EXPECT_EQ(uriCount, 0);
    EXPECT_EQ(uris.size(), 0);
    EXPECT_EQ(indexs.size(), 0);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNeedCheckUrisTest_002 end");
}

/**
 * @tc.name: GetNeedCheckUrisTest_003.
 * @tc.desc:
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, GetNeedCheckUrisTest_003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNeedCheckUrisTest_003 start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record1 = std::make_shared<PasteDataRecord>();
    record1->SetUri(nullptr);

    auto fileUri = std::make_shared<OHOS::Uri>("file://test1");
    auto record2 = std::make_shared<PasteDataRecord>();
    record2->SetUri(fileUri);

    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record1);
    records.push_back(record2);
    records.push_back(nullptr);

    pasteData.records_ = records;

    std::vector<std::string> uris;
    std::vector<size_t> indexs;
    size_t uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, false);
    EXPECT_EQ(uriCount, 1);
    EXPECT_EQ(uris.size(), 1);
    EXPECT_EQ(indexs.size(), 1);

    uris.clear();
    indexs.clear();
    uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, true);
    EXPECT_EQ(uriCount, 1);
    EXPECT_EQ(uris.size(), 0);
    EXPECT_EQ(indexs.size(), 0);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "GetNeedCheckUrisTest_003 end");
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest001 start");
    PasteData data;
    data.SetLocalPasteFlag(false);
    data.AddUriRecord(OHOS::Uri("file://media/111.png"));
    data.AddUriRecord(OHOS::Uri("file://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file://com.pasteboard.test/111.png"));
    std::vector<std::string> expectUris = {
        "file://com.pasteboard.test/111.png",
        "file://docs/111.txt",
        "file://media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_NE(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest001 start");
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest002 start");
    PasteData data;
    data.SetLocalPasteFlag(false);
    data.AddUriRecord(OHOS::Uri("http://media/111.png"));
    data.AddUriRecord(OHOS::Uri("media://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("image://com.pasteboard.test/111.png"));
    std::vector<std::string> expectUris = {
        "image://com.pasteboard.test/111.png",
        "media://docs/111.txt",
        "http://media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest002 start");
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest003
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest003 start");
    PasteData data;
    data.SetLocalPasteFlag(false);
    data.AddUriRecord(OHOS::Uri("file:///media/111.png"));
    data.AddUriRecord(OHOS::Uri("file:///docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file:///com.pasteboard.test/111.png"));
    std::vector<std::string> expectUris = {
        "",
        "",
        "",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest003 start");
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest004
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest004 start");
    PasteData data;
    data.SetLocalPasteFlag(false);
    data.AddUriRecord(OHOS::Uri("/media/111.png"));
    data.AddUriRecord(OHOS::Uri("/docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("/data/111.png"));
    std::vector<std::string> expectUris = {
        "",
        "",
        "",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest004 start");
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest005
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest005 start");
    PasteData data;
    data.SetLocalPasteFlag(false);
    data.AddUriRecord(OHOS::Uri("file://media/111.png"));
    data.AddUriRecord(OHOS::Uri("file://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file://com.pasteboard.test/111.png"));
    data.AddUriRecord(OHOS::Uri("http://media/111.png"));
    data.AddUriRecord(OHOS::Uri("meida://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("image://com.pasteboard.test/111.png"));
    data.AddUriRecord(OHOS::Uri("file:///media/111.png"));
    data.AddUriRecord(OHOS::Uri("file:///docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file:///com.pasteboard.test/111.png"));
    data.AddUriRecord(OHOS::Uri("/media/111.png"));
    data.AddUriRecord(OHOS::Uri("/docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("/data/111.png"));
    std::vector<std::string> expectUris = {
        "",
        "",
        "",
        "",
        "",
        "",
        "image://com.pasteboard.test/111.png",
        "meida://docs/111.txt",
        "http://media/111.png",
        "file://com.pasteboard.test/111.png",
        "file://docs/111.txt",
        "file://media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_NE(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriCrossTest005 start");
}


/**
 * @tc.name: DataRemoveInvalidUriLocalTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest001 start");
    PasteData data;
    data.SetLocalPasteFlag(true);
    data.AddUriRecord(OHOS::Uri("file://media/111.png"));
    data.AddUriRecord(OHOS::Uri("file://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file://com.pasteboard.test/111.png"));
    std::vector<std::string> expectUris = {
        "file://com.pasteboard.test/111.png",
        "file://docs/111.txt",
        "file://media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest001 start");
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest002 start");
    PasteData data;
    data.SetLocalPasteFlag(true);
    data.AddUriRecord(OHOS::Uri("http://media/111.png"));
    data.AddUriRecord(OHOS::Uri("media://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("image://com.pasteboard.test/111.png"));
    std::vector<std::string> expectUris = {
        "image://com.pasteboard.test/111.png",
        "media://docs/111.txt",
        "http://media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest002 start");
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest003
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest003 start");
    PasteData data;
    data.SetLocalPasteFlag(true);
    data.AddUriRecord(OHOS::Uri("file:///media/111.png"));
    data.AddUriRecord(OHOS::Uri("file:///docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file:///com.pasteboard.test/111.png"));
    std::vector<std::string> expectUris = {
        "file:///com.pasteboard.test/111.png",
        "file:///docs/111.txt",
        "file:///media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest003 start");
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest004
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest004 start");
    PasteData data;
    data.SetLocalPasteFlag(true);
    data.AddUriRecord(OHOS::Uri("/media/111.png"));
    data.AddUriRecord(OHOS::Uri("/docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("/data/111.png"));
    std::vector<std::string> expectUris = {
        "/data/111.png",
        "/docs/111.txt",
        "/media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest004 start");
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest005
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest005 start");
    PasteData data;
    data.SetLocalPasteFlag(true);
    data.AddUriRecord(OHOS::Uri("file://media/111.png"));
    data.AddUriRecord(OHOS::Uri("file://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file://com.pasteboard.test/111.png"));
    data.AddUriRecord(OHOS::Uri("http://media/111.png"));
    data.AddUriRecord(OHOS::Uri("meida://docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("image://com.pasteboard.test/111.png"));
    data.AddUriRecord(OHOS::Uri("file:///media/111.png"));
    data.AddUriRecord(OHOS::Uri("file:///docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("file:///com.pasteboard.test/111.png"));
    data.AddUriRecord(OHOS::Uri("/media/111.png"));
    data.AddUriRecord(OHOS::Uri("/docs/111.txt"));
    data.AddUriRecord(OHOS::Uri("/data/111.png"));
    std::vector<std::string> expectUris = {
        "/data/111.png",
        "/docs/111.txt",
        "/media/111.png",
        "file:///com.pasteboard.test/111.png",
        "file:///docs/111.txt",
        "file:///media/111.png",
        "image://com.pasteboard.test/111.png",
        "meida://docs/111.txt",
        "http://media/111.png",
        "file://com.pasteboard.test/111.png",
        "file://docs/111.txt",
        "file://media/111.png",
    };

    PasteboardWebController::GetInstance().RemoveInvalidUri(data);
    std::vector<std::string> actualUris;
    for (uint32_t recordIndex = 0; recordIndex < data.GetRecordCount(); ++recordIndex) {
        auto record = data.GetRecordAt(recordIndex);
        ASSERT_NE(record, nullptr);
        auto uri = record->GetUri();
        ASSERT_NE(uri, nullptr);
        auto uriStr = uri->ToString();
        actualUris.push_back(uriStr);
    }
    EXPECT_EQ(actualUris, expectUris);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DataRemoveInvalidUriLocalTest005 start");
}

/**
 * @tc.name: EntryRemoveInvalidUriTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest001 start");
    PasteDataEntry entry;
    entry.SetUtdId("general.file-uri");
    entry.SetMimeType(MIMETYPE_TEXT_URI);

    entry.SetValue("file://media/111.png");
    bool ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);
    auto uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_STREQ(uri->ToString().c_str(), "file://media/111.png");

    entry.SetValue("file://docs/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_STREQ(uri->ToString().c_str(), "file://docs/111.png");

    entry.SetValue("file://com.pasteboard.test/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_STREQ(uri->ToString().c_str(), "file://com.pasteboard.test/111.png");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest001 start");
}

/**
 * @tc.name: EntryRemoveInvalidUriTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest002 start");
    PasteDataEntry entry;
    entry.SetUtdId("general.file-uri");
    entry.SetMimeType(MIMETYPE_TEXT_URI);

    entry.SetValue("http://media/111.png");
    bool ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);
    auto uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_STREQ(uri->ToString().c_str(), "http://media/111.png");

    entry.SetValue("https://docs/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_STREQ(uri->ToString().c_str(), "https://docs/111.png");

    entry.SetValue("media://com.pasteboard.test/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_STREQ(uri->ToString().c_str(), "media://com.pasteboard.test/111.png");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest002 start");
}

/**
 * @tc.name: EntryRemoveInvalidUriTest003
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest003 start");
    PasteDataEntry entry;
    entry.SetUtdId("general.file-uri");
    entry.SetMimeType(MIMETYPE_TEXT_URI);

    entry.SetValue("file:///media/111.png");
    bool ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_TRUE(ret);
    auto uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_TRUE(uri->ToString().empty());

    entry.SetValue("file:///docs/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_TRUE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_TRUE(uri->ToString().empty());

    entry.SetValue("file:///com.pasteboard.test/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_TRUE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_TRUE(uri->ToString().empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest003 start");
}

/**
 * @tc.name: EntryRemoveInvalidUriTest004
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest004 start");
    PasteDataEntry entry;
    bool ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);

    entry.SetUtdId("general.file-uri");
    entry.SetMimeType(MIMETYPE_TEXT_URI);
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_FALSE(ret);

    entry.SetValue("/media/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_TRUE(ret);
    auto uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_TRUE(uri->ToString().empty());

    entry.SetValue("/docs/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_TRUE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_TRUE(uri->ToString().empty());

    entry.SetValue("/com.pasteboard.test/111.png");
    ret = PasteboardWebController::GetInstance().RemoveInvalidUri(entry);
    EXPECT_TRUE(ret);
    uri = entry.ConvertToUri();
    ASSERT_NE(uri, nullptr);
    EXPECT_TRUE(uri->ToString().empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryRemoveInvalidUriTest004 start");
}

/**
 * @tc.name: MatchImgExtensionTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, MatchImgExtensionTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MatchImgExtensionTest001 start");
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension(""));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:////"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///./"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:////."));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///png"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///png."));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///png/"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///.png"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///png/bbb"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/.png"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/.png/ccc"));
    EXPECT_FALSE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/1.png/ccc"));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MatchImgExtensionTest001 start");
}

/**
 * @tc.name: MatchImgExtensionTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, MatchImgExtensionTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MatchImgExtensionTest002 start");
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///1.png"));
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///1.jpg"));
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/ccc.jpeg"));
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/ccc.gif"));
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/ccc.gif?query=aaa"));
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/ccc.gif;version=1"));
    EXPECT_TRUE(PasteboardImgExtractor::MatchImgExtension("file:///aaa/bbb/ccc.gif;version=1?query=aaa"));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MatchImgExtensionTest002 start");
}
