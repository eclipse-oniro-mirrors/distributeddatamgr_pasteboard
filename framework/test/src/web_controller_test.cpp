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
 * @tc.name: SplitHtmlTest_001.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_001, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    auto records = webClipboardController.SplitHtml2Records(html, 1);
    EXPECT_EQ(records.size(), 0);
}

/**
 * @tc.name: SplitHtmlTest_002.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_002, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    auto records = webClipboardController.SplitHtml2Records(html, 1);
    EXPECT_EQ(records.size(), 1);
}

/**
 * @tc.name: SplitHtmlTest_003.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_003, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='file://file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    auto records = webClipboardController.SplitHtml2Records(html, 1);
    EXPECT_FALSE(records.empty());
}

/**
 * @tc.name: RebuildHtmlTest_004.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_004, TestSize.Level1)
{
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId());
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
}

/**
 * @tc.name: RebuildHtmlTest_005.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_005, TestSize.Level1)
{
    const int32_t splitRecordCount = 1;
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'>";
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId());
    EXPECT_EQ(records.size(), splitRecordCount);
    pasteData->AddRecord(records[0]);

    std::shared_ptr<PasteData> newPasteData = std::make_shared<PasteData>();
    std::vector<std::shared_ptr<PasteDataRecord>> pasteDataRecords = pasteData->AllRecords();
    EXPECT_EQ(*(pasteDataRecords[pasteData->GetRecordCount() - 1]->GetHtmlTextV0()), *html);

    newPasteData->AddHtmlRecord(*html);
    for (auto i = 0; i < pasteData->GetRecordCount() - 1; i++) {
        PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
        auto newUri = std::make_shared<OHOS::Uri>(uri);
        builder.SetUri(newUri);
        builder.SetCustomData(pasteDataRecords[i]->GetCustomData());
        auto record = builder.Build();
        newPasteData->AddRecord(record);
    }
    EXPECT_EQ(newPasteData->GetRecordCount(), 2);
    std::shared_ptr<std::string> newHtml = webClipboardController.RebuildHtml(newPasteData);
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    const char* newHtmlStr = newHtml.get()->c_str();
    EXPECT_STREQ(newHtmlStr, expectHtml);
}

/**
 * @tc.name: RebuildHtmlTest_006.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_006, TestSize.Level1)
{
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src=\"file2.jpg\"><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src=\"file:///data/storage/el2/distributedfiles/temp.png\"><img data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'>";
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId());
    EXPECT_EQ(records.size(), 2);
    EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
    pasteData->AddRecord(records[0]);
    pasteData->AddRecord(records[1]);

    std::shared_ptr<PasteData> newPasteData = std::make_shared<PasteData>();
    std::vector<std::shared_ptr<PasteDataRecord>> pasteDataRecords = pasteData->AllRecords();
    EXPECT_EQ(*(pasteDataRecords[pasteData->GetRecordCount() - 1]->GetHtmlTextV0()), *html);

    newPasteData->AddHtmlRecord(*html);
    auto newHtmlRecord = newPasteData->GetRecordAt(0);
    newHtmlRecord->SetFrom(newHtmlRecord->GetRecordId());
    for (auto i = 0; i < pasteData->GetRecordCount() - 1; i++) {
        PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
        auto newUri = std::make_shared<OHOS::Uri>(uri);
        builder.SetUri(newUri);
        builder.SetCustomData(pasteDataRecords[i]->GetCustomData());
        auto record = builder.Build();
        record->SetFrom(newHtmlRecord->GetRecordId());
        newPasteData->AddRecord(record);
    }
    EXPECT_EQ(newPasteData->GetRecordCount(), 3);
    webClipboardController.MergeExtraUris2Html(*newPasteData);
    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_STREQ(newHtmlStr->c_str(), expectHtml);
}

/**
 * @tc.name: SplitHtmlTest_007.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_007, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'>"));
    auto records = webClipboardController.SplitHtml2Records(html, 1);
    EXPECT_EQ(records.size(), 0);
}

/**
 * @tc.name: RebuildHtmlTest_008.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_008, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    std::shared_ptr<std::string> newHtml = webClipboardController.RebuildHtml(pasteData);
    EXPECT_EQ(*newHtml, *html);
}

/**
 * @tc.name: RebuildHtmlTest_009.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_009, TestSize.Level1)
{
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    const char *expectHtml =
        "<img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        ""
        "src=\"file:///data/storage/el2/distributedfiles/temp.png\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId());
    EXPECT_EQ(records.size(), 2);
    pasteData->AddRecord(records[0]);
    pasteData->AddRecord(records[1]);

    std::shared_ptr<PasteData> newPasteData = std::make_shared<PasteData>();
    std::vector<std::shared_ptr<PasteDataRecord>> pasteDataRecords = pasteData->AllRecords();
    EXPECT_EQ(*(pasteDataRecords[pasteData->GetRecordCount() - 1]->GetHtmlTextV0()), *html);

    newPasteData->AddHtmlRecord(*html);
    for (auto i = 0; i < pasteData->GetRecordCount() - 1; i++) {
        PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
        auto newUri = std::make_shared<OHOS::Uri>(uri);
        builder.SetUri(newUri);
        builder.SetCustomData(pasteDataRecords[i]->GetCustomData());
        auto record = builder.Build();
        newPasteData->AddRecord(record);
    }
    EXPECT_EQ(newPasteData->GetRecordCount(), 3);
    std::shared_ptr<std::string> newHtml = webClipboardController.RebuildHtml(newPasteData);
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    const char* newHtmlStr = newHtml.get()->c_str();
    EXPECT_STREQ(newHtmlStr, expectHtml);
}

/**
 * @tc.name: RebuildHtmlTest_010.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_010, TestSize.Level1)
{
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'>"));
    const char *expectHtml =
        "<img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "src=\"file:///data/storage/el2/distributedfiles/temp.png\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId());
    ASSERT_EQ(records.size(), 2);
    EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
    pasteData->AddRecord(records[0]);
    pasteData->AddRecord(records[1]);

    std::shared_ptr<PasteData> newPasteData = std::make_shared<PasteData>();
    std::vector<std::shared_ptr<PasteDataRecord>> pasteDataRecords = pasteData->AllRecords();
    EXPECT_EQ(*(pasteDataRecords[pasteData->GetRecordCount() - 1]->GetHtmlTextV0()), *html);

    newPasteData->AddHtmlRecord(*html);
    auto newHtmlRecord = newPasteData->GetRecordAt(0);
    newHtmlRecord->SetFrom(newHtmlRecord->GetRecordId());
    for (auto i = 0; i < pasteData->GetRecordCount() - 1; i++) {
        PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
        auto newUri = std::make_shared<OHOS::Uri>(uri);
        builder.SetUri(newUri);
        builder.SetCustomData(pasteDataRecords[i]->GetCustomData());
        auto record = builder.Build();
        record->SetFrom(newHtmlRecord->GetRecordId());
        newPasteData->AddRecord(record);
    }
    EXPECT_EQ(newPasteData->GetRecordCount(), 3);
    webClipboardController.MergeExtraUris2Html(*newPasteData);

    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_STREQ(newHtmlStr->c_str(), expectHtml);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteDataRecord> htmlRecord = nullptr;
    std::shared_ptr<std::string> htmlData = std::make_shared<std::string>("test data");

    webClipboardController.UpdateHtmlRecord(htmlRecord, htmlData);
    ASSERT_EQ(htmlRecord, nullptr);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteDataRecord> htmlRecord = std::make_shared<PasteDataRecord>();
    std::shared_ptr<std::string> htmlData = nullptr;
    webClipboardController.UpdateHtmlRecord(htmlRecord, htmlData);
    ASSERT_EQ(htmlData, nullptr);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    bool result = webClipboardController.SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PasteData pasteData;
    PasteDataRecord record;
    record.SetRecordId(1);
    record.SetFrom(1);
    pasteData.AddRecord(std::make_shared<PasteDataRecord>(record));
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);
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
    PasteData pasteData;
    PasteDataRecord record;
    record.SetRecordId(1);
    record.SetFrom(2);
    pasteData.AddRecord(std::make_shared<PasteDataRecord>(record));
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);
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
    PasteData pasteData;
    pasteData.AddHtmlRecord("");
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);
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
    PasteData pasteData;
    pasteData.AddHtmlRecord("<p>Hello world!<img src=\">file:///storage/local/files/Images/hello.png\"/><p>");
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);
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
    PasteData pasteData;
    pasteData.AddHtmlRecord("<p>Hello world!<img src=\">https://storage/local/files/Images/hello.png\"/><p>");
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);
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
    PasteData pasteData;
    pasteData.AddHtmlRecord("<p>Hello world!<img src=\"file:///storage/local/files/Images/hello.png\"/><p>");
    bool result = PasteboardWebController::GetInstance().SplitWebviewPasteData(pasteData);
    EXPECT_TRUE(result);
    EXPECT_EQ(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    record->SetFrom(1);
    record->SetRecordId(1);
    pasteData.AddRecord(record);
    bool result = webClipboardController.SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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

    bool result = webClipboardController.SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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

    bool result = webClipboardController.SplitWebviewPasteData(pasteData);
    EXPECT_FALSE(result);
    EXPECT_NE(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    std::string bundleName = "testBundle";
    int32_t appIndex = 0;
    webClipboardController.SetWebviewPasteData(pasteData, { bundleName, appIndex });
    ASSERT_EQ(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    std::pair<std::string, int32_t> bundleIndex = std::make_pair("bundleName", 1);
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag("INVALID_TAG");
    std::pair<std::string, int32_t> bundleIndex = std::make_pair("bundleName", 1);
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = std::make_shared<PasteDataRecord>();
    record->SetUri(nullptr);
    pasteData.AddRecord(record);
    std::pair<std::string, int32_t> bundleIndex = std::make_pair("bundleName", 1);
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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
    std::pair<std::string, int32_t> bundleIndex = std::make_pair("bundleName", 1);
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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
    std::pair<std::string, int32_t> bundleIndex = std::make_pair("bundleName", 1);
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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
    std::pair<std::string, int32_t> bundleIndex = std::make_pair("bundleName", 1);
    webClipboardController.SetWebviewPasteData(pasteData, bundleIndex);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.AddRecord(nullptr);
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

    auto tempPasteboard = std::make_shared<PasteboardWebController>();
    EXPECT_NE(tempPasteboard, nullptr);

    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    auto fileUri = std::make_shared<OHOS::Uri>("file://test");
    record->SetUri(fileUri);
    pasteData.AddRecord(record);
    webClipboardController.CheckAppUriPermission(pasteData);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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
    int32_t uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, false);
    EXPECT_EQ(uriCount, 1);

    uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, true);
    EXPECT_EQ(uriCount, 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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
    int32_t uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, false);
    EXPECT_EQ(uriCount, 0);

    uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, true);
    EXPECT_EQ(uriCount, 0);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "start");

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
    int32_t uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, false);
    EXPECT_EQ(uriCount, 1);

    uriCount = webClipboardController.GetNeedCheckUris(pasteData, uris, indexs, true);
    EXPECT_EQ(uriCount, 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "end");
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest001, TestSize.Level1)
{
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
    EXPECT_EQ(actualUris, expectUris);
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest002, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest003
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest003, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest004
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest004, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriCrossTest005
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriCrossTest005, TestSize.Level1)
{
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
    EXPECT_EQ(actualUris, expectUris);
}


/**
 * @tc.name: DataRemoveInvalidUriLocalTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest001, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest002, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest003
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest003, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest004
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest004, TestSize.Level1)
{
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
}

/**
 * @tc.name: DataRemoveInvalidUriLocalTest005
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, DataRemoveInvalidUriLocalTest005, TestSize.Level1)
{
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
}

/**
 * @tc.name: EntryRemoveInvalidUriTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest001, TestSize.Level1)
{
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
}

/**
 * @tc.name: EntryRemoveInvalidUriTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest002, TestSize.Level1)
{
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
}

/**
 * @tc.name: EntryRemoveInvalidUriTest003
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest003, TestSize.Level1)
{
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
}

/**
 * @tc.name: EntryRemoveInvalidUriTest004
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(WebControllerTest, EntryRemoveInvalidUriTest004, TestSize.Level1)
{
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
}
