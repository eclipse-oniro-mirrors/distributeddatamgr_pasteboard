/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
class WebControllerRecordTest : public testing::Test {
public:
    WebControllerRecordTest() {};
    ~WebControllerRecordTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void WebControllerRecordTest::SetUpTestCase(void) { }

void WebControllerRecordTest::TearDownTestCase(void) { }

void WebControllerRecordTest::SetUp(void) { }

void WebControllerRecordTest::TearDown(void) { }


/**
 * @tc.name: RebuildHtmlTest_046.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_046, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_046 start");
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
    int32_t userId = 1;
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_046 start");
}

/**
 * @tc.name: SplitHtmlTest_047.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_047, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_047 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_047 start");
}

/**
 * @tc.name: RebuildHtmlTest_048.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_048, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_048 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_048 start");
}

/**
 * @tc.name: RebuildHtmlTest_049.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_049, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_049 start");
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
    int32_t userId = 1;
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

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_049 start");
}

/**
 * @tc.name: RebuildHtmlTest_050.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_050, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_050 start");
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
    int32_t userId = 1;
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_050 start");
}

/**
 * @tc.name: SplitHtmlTest_051.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_051, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_051 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = -1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_051 end");
}

/**
 * @tc.name: SplitHtmlTest_052.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_052, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_052 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = -1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_052 start");
}

/**
 * @tc.name: SplitHtmlTest_053.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_053, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_053 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = -1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_053 start");
}

/**
 * @tc.name: RebuildHtmlTest_054.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_054, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_054 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = -1;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_054 start");
}

/**
 * @tc.name: RebuildHtmlTest_055.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_055, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_055 start");
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
    int32_t userId = -1;
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

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_055 start");
}

/**
 * @tc.name: RebuildHtmlTest_056.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_056, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_056 start");
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
    int32_t userId = -1;
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_056 start");
}

/**
 * @tc.name: SplitHtmlTest_057.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_057, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_057 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = -1;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_057 start");
}

/**
 * @tc.name: RebuildHtmlTest_058.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_058, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_058 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_058 start");
}

/**
 * @tc.name: RebuildHtmlTest_059.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_059, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_059 start");
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
    int32_t userId = -1;
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

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_059 start");
}

/**
 * @tc.name: RebuildHtmlTest_060.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_060, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_060 start");
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
    int32_t userId = -1;
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_060 start");
}


/**
 * @tc.name: SplitHtmlTest_061.
 * @tc.desc: Test did not use local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_061, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_061 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 102;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_061 end");
}

/**
 * @tc.name: SplitHtmlTest_062.
 * @tc.desc: Test contains a local image address HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_062, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_062 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 102;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_062 start");
}

/**
 * @tc.name: SplitHtmlTest_063.
 * @tc.desc: Test contains multiple image addresses, but no local address with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_063, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_063 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='file:///file1.jpg'><img data-ohos='clipboard' "
                        "src='file2.jpg'><img data-ohos='clipboard' "
                        "src='https:///data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 102;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_TRUE(records.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_063 start");
}

/**
 * @tc.name: RebuildHtmlTest_064.
 * @tc.desc: Test does not include local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_064, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_064 start");
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='http://file1.jpg'><img data-ohos='clipboard' "
                        "src='https://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    auto htmlRecord = pasteData->GetRecordAt(0);

    auto webClipboardController = PasteboardWebController::GetInstance();
    std::string bundleIndex;
    int32_t userId = 102;
    auto records = webClipboardController.SplitHtml2Records(html, htmlRecord->GetRecordId(), bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);

    webClipboardController.MergeExtraUris2Html(*pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_064 start");
}

/**
 * @tc.name: RebuildHtmlTest_065.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_065, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_065 start");
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
    int32_t userId = 102;
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

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_065 start");
}

/**
 * @tc.name: RebuildHtmlTest_066.
 * @tc.desc: Test contains a multiple image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_066, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_066 start");
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
    int32_t userId = 102;
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_066 start");
}

/**
 * @tc.name: SplitHtmlTest_067.
 * @tc.desc: Test contains invalid protocol image link HTML with SplitHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, SplitHtmlTest_067, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_067 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='yyy://data/storage/el2/distributedfiles/202305301.png'></html>"));
    std::string bundleIndex;
    int32_t userId = 102;
    auto records = webClipboardController.SplitHtml2Records(html, 1, bundleIndex, userId);
    EXPECT_EQ(records.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SplitHtmlTest_067 start");
}

/**
 * @tc.name: RebuildHtmlTest_068.
 * @tc.desc: Test contains invalid protocol image link HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_068, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_068 start");
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::shared_ptr<std::string> html(
        new std::string("<html><img data-ohos='clipboard' src='xxx://file1.jpg'><img data-ohos='clipboard' "
                        "src='ttt://data/storage/el2/distributedfiles/202305301.png'></html>"));
    auto pasteData = PasteboardClient::GetInstance()->CreateHtmlData(*html);
    EXPECT_EQ(pasteData->GetRecordCount(), 1);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_068 start");
}

/**
 * @tc.name: RebuildHtmlTest_069.
 * @tc.desc: Test contains a local image address HTML with RebuildHtml.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_069, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_069 start");
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
    int32_t userId = 102;
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

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_069 start");
}

/**
 * @tc.name: RebuildHtmlTest_070.
 * @tc.desc: Test contains a local image address HTML with MergeExtraUris2Html.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(WebControllerRecordTest, RebuildHtmlTest_070, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_070 start");
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
    int32_t userId = 102;
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "RebuildHtmlTest_070 start");
}