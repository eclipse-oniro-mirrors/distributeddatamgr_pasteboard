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
 * @tc.name: SplitHtmlTest_001.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_001 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_001 end");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_002 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_002 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_003 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_003 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_004 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_004 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_005 start");
    const int32_t splitRecordCount = 1;
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    if (records.size() > 0) {
        EXPECT_EQ(records.size(), splitRecordCount);
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_005 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_006 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src=\"file2.jpg\"><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src=\"file2.jpg\"><img data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);
    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_006 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_007 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_007 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_008 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_008 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_009 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        ""
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_009 start");
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_010 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 100;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);

    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_010 start");
}

/**
 * @tc.name: SplitHtmlTest_011.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_011, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_011 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_011 end");
}

/**
 * @tc.name: SplitHtmlTest_012.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_012, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_012 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_012 start");
}

/**
 * @tc.name: SplitHtmlTest_013.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_013, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_013 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_013 start");
}

/**
 * @tc.name: RebuildHtmlTest_014.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_014, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_014 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_014 start");
}

/**
 * @tc.name: RebuildHtmlTest_015.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_015, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_015 start");
    const int32_t splitRecordCount = 1;
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    if (records.size() > 0) {
        EXPECT_EQ(records.size(), splitRecordCount);
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_015 start");
}

/**
 * @tc.name: RebuildHtmlTest_016.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_016, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_016 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src=\"file2.jpg\"><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src=\"file2.jpg\"><img data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);
    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_016 start");
}

/**
 * @tc.name: SplitHtmlTest_017.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_017, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_017 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_017 start");
}

/**
 * @tc.name: RebuildHtmlTest_018.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_018, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_018 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_018 start");
}

/**
 * @tc.name: RebuildHtmlTest_019.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_019, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_019 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        ""
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_019 start");
}

/**
 * @tc.name: RebuildHtmlTest_020.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_020, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_020 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 101;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);

    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_020 start");
}

/**
 * @tc.name: SplitHtmlTest_021.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_021, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_021 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_021 end");
}

/**
 * @tc.name: SplitHtmlTest_022.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_022, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_022 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_022 start");
}

/**
 * @tc.name: SplitHtmlTest_023.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_023, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_023 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_023 start");
}

/**
 * @tc.name: RebuildHtmlTest_024.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_024, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_024 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_024 start");
}

/**
 * @tc.name: RebuildHtmlTest_025.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_025, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_025 start");
    const int32_t splitRecordCount = 1;
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    if (records.size() > 0) {
        EXPECT_EQ(records.size(), splitRecordCount);
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_025 start");
}

/**
 * @tc.name: RebuildHtmlTest_026.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_026, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_026 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src=\"file2.jpg\"><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src=\"file2.jpg\"><img data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);
    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_026 start");
}

/**
 * @tc.name: SplitHtmlTest_027.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_027, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_027 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_027 start");
}

/**
 * @tc.name: RebuildHtmlTest_028.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_028, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_028 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_028 start");
}

/**
 * @tc.name: RebuildHtmlTest_029.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_029, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_029 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        ""
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_029 start");
}

/**
 * @tc.name: RebuildHtmlTest_030.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_030, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_030 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);

    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_030 start");
}

/**
 * @tc.name: SplitHtmlTest_031.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_031, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_031 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_031 end");
}

/**
 * @tc.name: SplitHtmlTest_032.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_032, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_032 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_032 start");
}

/**
 * @tc.name: SplitHtmlTest_033.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_033, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_033 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_033 start");
}

/**
 * @tc.name: RebuildHtmlTest_034.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_034, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_034 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_034 start");
}

/**
 * @tc.name: RebuildHtmlTest_035.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_035, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_035 start");
    const int32_t splitRecordCount = 1;
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    if (records.size() > 0) {
        EXPECT_EQ(records.size(), splitRecordCount);
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_035 start");
}

/**
 * @tc.name: RebuildHtmlTest_036.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_036, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_036 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src=\"file2.jpg\"><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src=\"file2.jpg\"><img data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);
    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_036 start");
}

/**
 * @tc.name: SplitHtmlTest_037.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_037, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_037 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_037 start");
}

/**
 * @tc.name: RebuildHtmlTest_038.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_038, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_038 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_038 start");
}

/**
 * @tc.name: RebuildHtmlTest_039.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_039, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_039 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        ""
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_039 start");
}

/**
 * @tc.name: RebuildHtmlTest_040.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_040, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_040 start");
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img src='file:///file1.jpg'><img src=\"file2.jpg\"><img "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    const char *expectHtml =
        "<html><img src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "src=\"file2.jpg\"><img "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());
    std::string bundleIndex;
    int32_t userId = 0;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    ASSERT_EQ(records.size(), 0);
    if (records.size() > 0) {
        EXPECT_EQ(records[0]->GetFrom(), htmlRecord->GetRecordId());
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);
    webClipboardController.MergeExtraUris2Html(*newPasteData);

    ASSERT_EQ(newPasteData->GetRecordCount(), 1);
    auto recordGet = newPasteData->GetRecordAt(0);
    auto newHtmlStr = recordGet->GetHtmlTextV0();
    ASSERT_NE(newHtmlStr, nullptr);
    EXPECT_NE(newHtmlStr->c_str(), expectHtml);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_040 start");
}

/**
 * @tc.name: SplitHtmlTest_041.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_041, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_041 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_041 end");
}

/**
 * @tc.name: SplitHtmlTest_042.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_042, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_042 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_042 start");
}

/**
 * @tc.name: SplitHtmlTest_043.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, SplitHtmlTest_043, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_043 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_043 start");
}

/**
 * @tc.name: RebuildHtmlTest_044.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_044, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_044 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = 1;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_044 start");
}

/**
 * @tc.name: RebuildHtmlTest_045.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerTest, RebuildHtmlTest_045, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_045 start");
    const int32_t splitRecordCount = 1;
    const std::string uri = "file:///data/storage/el2/distributedfiles/temp.png";
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);
    htmlRecord->SetFrom(htmlRecord->GetRecordId());

    const char *expectHtml =
        "<html><img data-ohos='clipboard' src='file:///data/storage/el2/distributedfiles/temp.png'><img "
        "data-ohos='clipboard' "
        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>";
    std::string bundleIndex;
    int32_t userId = 1;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    if (records.size() > 0) {
        EXPECT_EQ(records.size(), splitRecordCount);
        pasteData->AddRecord(records[0]);
    }

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
    EXPECT_EQ(newPasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_045 start");
}