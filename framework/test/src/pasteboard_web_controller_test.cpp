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

#include "pasteboard_hilog.h"
#include "paste_data_record.h"
#include "pasteboard_web_controller.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MiscServices;
class PasteboardWebControllerTest : public testing::Test {
public:
    PasteboardWebControllerTest() {};
    ~PasteboardWebControllerTest() {};
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardWebControllerTest::SetUpTestCase(void) { }

void PasteboardWebControllerTest::TearDownTestCase(void) { }

void PasteboardWebControllerTest::SetUp(void) { }

void PasteboardWebControllerTest::TearDown(void) { }

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_001.
 * @tc.desc: htmlData is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_001, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
    EXPECT_EQ(records.size(), 0);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_002.
 * @tc.desc: item is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_002, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(nullptr);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_003.
 * @tc.desc: uri is nullptr and customData is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_003, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord("");
    records.push_back(record);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_004.
 * @tc.desc: uri is nullptr and customData is not nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_004, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<uint8_t> arrayBuffer(1, 1);
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewKvRecord(MIMETYPE_TEXT_HTML, arrayBuffer);
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}


/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_005.
 * @tc.desc: uri is not nullptr and customData is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_005, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    builder.SetCustomData(customData);
    auto record = builder.Build();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_006.
 * @tc.desc: htmlData is not nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_006, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(
        "<html><body>hello world</body></html>");
    records.push_back(record);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_007.
 * @tc.desc: itemData.second size is invalid.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_007, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto record = builder.Build();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_008.
 * @tc.desc: itemData.second size is valid.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_008, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03, 0x04};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto record = builder.Build();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    EXPECT_EQ(records.size(), 1);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_009.
 * @tc.desc: replaceUri offset invalid.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_009, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03, 0x04};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto uriRecord = builder.Build();
    std::shared_ptr<PasteDataRecord> htmlRecord = PasteDataRecord::NewHtmlRecord(
        "<html><body>hello world</body></html>");
    std::vector<std::shared_ptr<PasteDataRecord>> records{htmlRecord, uriRecord};
    EXPECT_EQ(records.size(), 2);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ReplaceHtmlRecordContentByExtraUrisTest_010.
 * @tc.desc: replaceUri offset is valid.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ReplaceHtmlRecordContentByExtraUrisTest_010, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x00, 0x00, 0x00};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto uriRecord = builder.Build();
    std::shared_ptr<PasteDataRecord> htmlRecord = PasteDataRecord::NewHtmlRecord(
        "<html><body>hello world</body></html>");
    std::vector<std::shared_ptr<PasteDataRecord>> records{htmlRecord, uriRecord};
    EXPECT_EQ(records.size(), 2);
    webClipboardController.ReplaceHtmlRecordContentByExtraUris(records);
}

/**
 * @tc.name: ExtractContent_001.
 * @tc.desc: item is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_001, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(nullptr);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    EXPECT_EQ(htmlRecord, nullptr);
    EXPECT_TRUE(replaceUris.empty());
}

/**
 * @tc.name: ExtractContent_002.
 * @tc.desc: uri is nullptr and customData is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_002, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord("");
    records.push_back(record);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    EXPECT_EQ(htmlRecord, nullptr);
    EXPECT_TRUE(replaceUris.empty());
}

/**
 * @tc.name: ExtractContent_003.
 * @tc.desc: uri is nullptr and customData is not nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_003, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<uint8_t> arrayBuffer(1, 1);
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewKvRecord(MIMETYPE_TEXT_HTML, arrayBuffer);
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    EXPECT_EQ(htmlRecord, nullptr);
    EXPECT_TRUE(replaceUris.empty());
}


/**
 * @tc.name: ExtractContent_004.
 * @tc.desc: uri is not nullptr and customData is nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_004, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    builder.SetCustomData(customData);
    auto record = builder.Build();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    EXPECT_EQ(htmlRecord, nullptr);
    EXPECT_TRUE(replaceUris.empty());
}

/**
 * @tc.name: ExtractContent_005.
 * @tc.desc: htmlData is not nullptr.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_005, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    std::string html = "<html><body>hello world</body></html>";
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    records.push_back(record);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    ASSERT_NE(htmlData, nullptr);
    EXPECT_EQ(*htmlData, html);
    EXPECT_TRUE(replaceUris.empty());
}

/**
 * @tc.name: ExtractContent_006.
 * @tc.desc: itemData.second size is invalid.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_006, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto record = builder.Build();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    EXPECT_EQ(htmlRecord, nullptr);
    EXPECT_TRUE(replaceUris.empty());
}

/**
 * @tc.name: ExtractContent_007.
 * @tc.desc: itemData.second size is valid.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_007, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///data/storage/el2/distributedfiles/temp.png");
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03, 0x04};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto record = builder.Build();
    std::vector<std::shared_ptr<PasteDataRecord>> records;
    records.push_back(record);
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    EXPECT_EQ(htmlRecord, nullptr);
    EXPECT_FALSE(replaceUris.empty());
}

/**
 * @tc.name: ExtractContent_008.
 * @tc.desc: functionality test.
 * @tc.type: FUNC.
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardWebControllerTest, ExtractContent_008, TestSize.Level1)
{
    auto webClipboardController = PasteboardWebController::GetInstance();
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::string uriStr = "file:///data/storage/el2/distributedfiles/temp.png";
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    builder.SetUri(uri);
    auto customData = std::make_shared<MineCustomData>();
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x00, 0x00, 0x00};
    customData->AddItemData(key, val);
    builder.SetCustomData(customData);
    auto uriRecord = builder.Build();
    std::string html = "<html><body>hello world</body></html>";
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    std::vector<std::shared_ptr<PasteDataRecord>> records{record, uriRecord};
    OffsetMap replaceUris;
    auto [htmlRecord, htmlData] = webClipboardController.ExtractContent(records, replaceUris);
    ASSERT_NE(htmlData, nullptr);
    EXPECT_EQ(*htmlData, html);
    ASSERT_FALSE(replaceUris.empty());
    auto it = replaceUris.find(1);
    ASSERT_NE(it, replaceUris.end());
    EXPECT_EQ(it->second.first, uriStr);
    EXPECT_EQ(it->second.second, key);
}

/**
 * @tc.name: SplitWebviewPasteData_001.
 * @tc.desc: record->GetRecordId() == record->GetFrom(), continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitWebviewPasteData_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html><body>test</body></html>");
    record->SetFrom(record->GetRecordId());
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitWebviewPasteData_002.
 * @tc.desc: htmlEntry == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitWebviewPasteData_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewPlainTextRecord("test text");
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitWebviewPasteData_003.
 * @tc.desc: html == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitWebviewPasteData_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("");
    record->SetFrom(1);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitWebviewPasteData_004.
 * @tc.desc: extraUriRecords.empty(), continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitWebviewPasteData_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body>no images</body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitWebviewPasteData_005.
 * @tc.desc: hasExtraRecord == true, set tag and return true.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitWebviewPasteData_005, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body><img src=\"file:///test.png\"></body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SetWebviewPasteData_001.
 * @tc.desc: pasteData.GetTag() != WEBVIEW_PASTEDATA_TAG, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SetWebviewPasteData_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    pasteData.AddRecord(record);
    controller.SetWebviewPasteData(pasteData, "bundleIndex");
    EXPECT_NE(pasteData.GetTag(), PasteData::WEBVIEW_PASTEDATA_TAG);
}

/**
 * @tc.name: SetWebviewPasteData_002.
 * @tc.desc: GetUriV0() == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SetWebviewPasteData_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetFrom(1);
    pasteData.AddRecord(record);
    controller.SetWebviewPasteData(pasteData, "bundleIndex");
    EXPECT_EQ(pasteData.GetRecordCount(), 1u);
}

/**
 * @tc.name: SetWebviewPasteData_003.
 * @tc.desc: GetFrom() == 0, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SetWebviewPasteData_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///test.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    record->SetFrom(0);
    pasteData.AddRecord(record);
    auto originalUri = record->GetUriV0()->ToString();
    controller.SetWebviewPasteData(pasteData, "bundleIndex");
    EXPECT_EQ(record->GetUriV0()->ToString(), originalUri);
}

/**
 * @tc.name: SetWebviewPasteData_004.
 * @tc.desc: GetRecordId() == GetFrom(), continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SetWebviewPasteData_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///test.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    record->SetFrom(record->GetRecordId());
    pasteData.AddRecord(record);
    auto originalUri = record->GetUriV0()->ToString();
    controller.SetWebviewPasteData(pasteData, "bundleIndex");
    EXPECT_EQ(record->GetUriV0()->ToString(), originalUri);
}

/**
 * @tc.name: SetWebviewPasteData_005.
 * @tc.desc: uriStr.find(IMG_LOCAL_URI) != 0, skip uri replace.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SetWebviewPasteData_005, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("http://example.com/image.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    record->SetFrom(1);
    pasteData.AddRecord(record);
    auto originalUri = record->GetUriV0()->ToString();
    controller.SetWebviewPasteData(pasteData, "bundleIndex");
    EXPECT_EQ(record->GetUriV0()->ToString(), originalUri);
}

/**
 * @tc.name: SetWebviewPasteData_006.
 * @tc.desc: uriStr.find(IMG_LOCAL_URI) == 0, replace uri.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SetWebviewPasteData_006, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    std::string imgUri = "file:///data/storage/el2/base/temp.png";
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>(imgUri);
    builder.SetUri(uri);
    auto record = builder.Build();
    record->SetFrom(1);
    pasteData.AddRecord(record);
    controller.SetWebviewPasteData(pasteData, "bundleIndex");
    auto newUri = record->GetUriV0()->ToString();
    EXPECT_EQ(newUri, imgUri);
    EXPECT_FALSE(newUri.find("bundleIndex") != std::string::npos);
}

/**
 * @tc.name: RetainUri_001.
 * @tc.desc: !pasteData.IsLocalPaste(), return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RetainUri_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetConvertUri("test_convert_uri");
    pasteData.AddRecord(record);
    controller.RetainUri(pasteData);
    EXPECT_EQ(record->GetConvertUri(), "test_convert_uri");
}

/**
 * @tc.name: RetainUri_002.
 * @tc.desc: record != nullptr, clear convert uri.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RetainUri_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetConvertUri("test_convert_uri");
    pasteData.AddRecord(record);
    pasteData.SetLocalPasteFlag(true);
    controller.RetainUri(pasteData);
    EXPECT_EQ(record->GetConvertUri(), "");
}

/**
 * @tc.name: RetainUri_003.
 * @tc.desc: record == nullptr, skip clearing.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RetainUri_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///test.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    record->SetConvertUri("test_convert_uri");
    pasteData.AddRecord(record);
    pasteData.AddRecord(nullptr);
    pasteData.SetLocalPasteFlag(true);
    controller.RetainUri(pasteData);
    EXPECT_EQ(record->GetConvertUri(), "");
}

/**
 * @tc.name: RemoveInvalidUri_PasteData_001.
 * @tc.desc: data.IsLocalPaste(), return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_PasteData_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///test.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    pasteData.AddRecord(record);
    pasteData.SetLocalPasteFlag(true);
    auto originalUri = record->GetUriV0()->ToString();
    controller.RemoveInvalidUri(pasteData);
    EXPECT_EQ(record->GetUriV0()->ToString(), originalUri);
}

/**
 * @tc.name: RemoveInvalidUri_PasteData_002.
 * @tc.desc: record == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_PasteData_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.AddRecord(nullptr);
    auto validRecord = PasteDataRecord::NewHtmlRecord("<html></html>");
    pasteData.AddRecord(validRecord);
    controller.RemoveInvalidUri(pasteData);
    EXPECT_EQ(pasteData.GetRecordCount(), 1u);
    EXPECT_EQ(pasteData.GetRecordAt(0), validRecord);
}

/**
 * @tc.name: RemoveInvalidUri_PasteData_003.
 * @tc.desc: uriPtr == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_PasteData_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.RemoveInvalidUri(pasteData);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RemoveInvalidUri_PasteData_004.
 * @tc.desc: IsValidUri returns true, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_PasteData_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("http://example.com/file.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    pasteData.AddRecord(record);
    auto originalUri = record->GetUriV0()->ToString();
    controller.RemoveInvalidUri(pasteData);
    EXPECT_EQ(record->GetUriV0()->ToString(), originalUri);
}

/**
 * @tc.name: RemoveInvalidUri_PasteData_005.
 * @tc.desc: IsValidUri returns false, remove uri.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_PasteData_005, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file://");
    builder.SetUri(uri);
    auto record = builder.Build();
    pasteData.AddRecord(record);
    controller.RemoveInvalidUri(pasteData);
    EXPECT_EQ(record->GetUriV0()->ToString(), "");
}

/**
 * @tc.name: RemoveInvalidUri_Entry_001.
 * @tc.desc: entry.GetMimeType() != MIMETYPE_TEXT_URI, return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_Entry_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteDataEntry entry(MIMETYPE_TEXT_HTML, "<html></html>");
    bool result = controller.RemoveInvalidUri(entry);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: RemoveInvalidUri_Entry_002.
 * @tc.desc: uriPtr == nullptr, return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_Entry_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteDataEntry entry(MIMETYPE_TEXT_URI, "invalid_uri_string");
    bool result = controller.RemoveInvalidUri(entry);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: RemoveInvalidUri_Entry_003.
 * @tc.desc: IsValidUri returns true, return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_Entry_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteDataEntry entry(MIMETYPE_TEXT_URI, "http://example.com/file.png");
    bool result = controller.RemoveInvalidUri(entry);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: RemoveInvalidUri_Entry_004.
 * @tc.desc: IsValidUri returns false, clear value and return true.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveInvalidUri_Entry_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteDataEntry entry(MIMETYPE_TEXT_URI, "file://");
    bool result = controller.RemoveInvalidUri(entry);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidUri_001.
 * @tc.desc: uriPtr == nullptr, return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsValidUri_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    std::shared_ptr<OHOS::Uri> uriPtr = nullptr;
    bool result = controller.IsValidUri(uriPtr, true);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidUri_002.
 * @tc.desc: scheme.empty(), return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsValidUri_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto uriPtr = std::make_shared<OHOS::Uri>("not_a_valid_uri");
    bool result = controller.IsValidUri(uriPtr, true);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidUri_003.
 * @tc.desc: scheme == FILE_SCHEME && authority.empty(), return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsValidUri_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto uriPtr = std::make_shared<OHOS::Uri>("file://");
    bool result = controller.IsValidUri(uriPtr, true);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidUri_004.
 * @tc.desc: scheme == FILE_SCHEME && !hasPermission, return false.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsValidUri_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto uriPtr = std::make_shared<OHOS::Uri>("file:///data/test.png");
    bool result = controller.IsValidUri(uriPtr, false);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsValidUri_005.
 * @tc.desc: valid http uri with permission, return true.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsValidUri_005, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto uriPtr = std::make_shared<OHOS::Uri>("http://example.com/file.png");
    bool result = controller.IsValidUri(uriPtr, true);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsValidUri_006.
 * @tc.desc: valid file uri with permission, return true.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsValidUri_006, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto uriPtr = std::make_shared<OHOS::Uri>("file:///data/storage/el2/base/temp.png");
    bool result = controller.IsValidUri(uriPtr, true);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: RebuildWebviewPasteData_001.
 * @tc.desc: pasteData.GetTag() != WEBVIEW_PASTEDATA_TAG, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RebuildWebviewPasteData_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.RebuildWebviewPasteData(pasteData, "bundleIndex", 0);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RebuildWebviewPasteData_002.
 * @tc.desc: item->GetFrom() == 0, no merge operation.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RebuildWebviewPasteData_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetFrom(0);
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.RebuildWebviewPasteData(pasteData, "bundleIndex", 0);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RebuildWebviewPasteData_003.
 * @tc.desc: item->GetFrom() > 0, merge extra uris.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RebuildWebviewPasteData_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetFrom(1);
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.RebuildWebviewPasteData(pasteData, "bundleIndex", 0);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: CheckAppUriPermission_001.
 * @tc.desc: uris.empty(), return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, CheckAppUriPermission_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.CheckAppUriPermission(pasteData);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: CheckAppUriPermission_002.
 * @tc.desc: has originUri but empty check results.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, CheckAppUriPermission_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("http://example.com/file.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.CheckAppUriPermission(pasteData);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: CheckAppUriPermission_003.
 * @tc.desc: item == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, CheckAppUriPermission_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("http://example.com/file.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    pasteData.AddRecord(record);
    pasteData.AddRecord(nullptr);
    auto countBefore = pasteData.GetRecordCount();
    controller.CheckAppUriPermission(pasteData);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: CheckAppUriPermission_004.
 * @tc.desc: item->GetOriginUri() == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, CheckAppUriPermission_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("http://example.com/file.png");
    builder.SetUri(uri);
    auto record = builder.Build();
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.CheckAppUriPermission(pasteData);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: SplitHtml2Records_001.
 * @tc.desc: html == nullptr, return empty records.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitHtml2Records_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitHtml2Records_002.
 * @tc.desc: html has no img tags, return empty records.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitHtml2Records_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body>no images here</body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitHtml2Records_003.
 * @tc.desc: html has img tags but no src, return empty records.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitHtml2Records_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body><img></body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitHtml2Records_004.
 * @tc.desc: html has img with remote src, return empty records.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitHtml2Records_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body><img src=\"http://remote.com/img.png\"></body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SplitHtml2Records_005.
 * @tc.desc: html has img with local src, return records.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, SplitHtml2Records_005, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body><img src=\"file:///data/test.png\"></body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: UpdateHtmlRecord_001.
 * @tc.desc: htmlRecord == nullptr, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, UpdateHtmlRecord_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteDataRecord> htmlRecord = nullptr;
    std::shared_ptr<std::string> htmlData = std::make_shared<std::string>("<html></html>");
    controller.UpdateHtmlRecord(htmlRecord, htmlData);
    EXPECT_EQ(htmlRecord, nullptr);
}

/**
 * @tc.name: UpdateHtmlRecord_002.
 * @tc.desc: htmlData == nullptr, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, UpdateHtmlRecord_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteDataRecord> htmlRecord = PasteDataRecord::NewHtmlRecord("<html></html>");
    std::shared_ptr<std::string> htmlData = nullptr;
    controller.UpdateHtmlRecord(htmlRecord, htmlData);
    EXPECT_EQ(htmlData, nullptr);
}

/**
 * @tc.name: UpdateHtmlRecord_003.
 * @tc.desc: entry == nullptr, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, UpdateHtmlRecord_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto htmlRecord = PasteDataRecord::NewPlainTextRecord("text");
    std::shared_ptr<std::string> htmlData = std::make_shared<std::string>("<html></html>");
    controller.UpdateHtmlRecord(htmlRecord, htmlData);
    EXPECT_NE(htmlData, nullptr);
}

/**
 * @tc.name: UpdateHtmlRecord_004.
 * @tc.desc: holds_alternative<string>, update string value.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, UpdateHtmlRecord_004, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    std::string oldHtml = "<html>old</html>";
    auto htmlRecord = PasteDataRecord::NewHtmlRecord(oldHtml);
    std::shared_ptr<std::string> htmlData = std::make_shared<std::string>("<html>new</html>");
    controller.UpdateHtmlRecord(htmlRecord, htmlData);
    EXPECT_EQ(htmlRecord->GetFrom(), 0);
}

/**
 * @tc.name: RemoveRecordById_001.
 * @tc.desc: GetRecordAt(i) == nullptr, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveRecordById_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.AddRecord(nullptr);
    pasteData.AddRecord(PasteDataRecord::NewHtmlRecord("<html></html>"));
    auto countBefore = pasteData.GetRecordCount();
    controller.RemoveRecordById(pasteData, 999);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RemoveRecordById_002.
 * @tc.desc: GetRecordId() != recordId, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveRecordById_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    uint32_t targetId = record->GetRecordId() + 100;
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.RemoveRecordById(pasteData, targetId);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RemoveRecordById_003.
 * @tc.desc: RemoveRecordAt() success, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveRecordById_003, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    uint32_t targetId = record->GetRecordId();
    pasteData.AddRecord(record);
    auto countBefore = pasteData.GetRecordCount();
    controller.RemoveRecordById(pasteData, targetId);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RemoveAllRecord_001.
 * @tc.desc: pasteData == nullptr, return early.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveAllRecord_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    std::shared_ptr<PasteData> pasteData = nullptr;
    controller.RemoveAllRecord(pasteData);
    EXPECT_EQ(pasteData, nullptr);
}

/**
 * @tc.name: RemoveAllRecord_002.
 * @tc.desc: RemoveRecordAt(0) success, remove all.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveAllRecord_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    auto pasteData = std::make_shared<PasteData>();
    pasteData->AddRecord(PasteDataRecord::NewHtmlRecord("<html></html>"));
    pasteData->AddRecord(PasteDataRecord::NewPlainTextRecord("text"));
    controller.RemoveAllRecord(pasteData);
    EXPECT_EQ(pasteData->GetRecordCount(), 0u);
}

/**
 * @tc.name: GroupRecordWithFrom_001.
 * @tc.desc: record->GetFrom() == 0, continue loop.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, GroupRecordWithFrom_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetFrom(0);
    pasteData.AddRecord(record);
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto countBefore = pasteData.GetRecordCount();
    controller.RebuildWebviewPasteData(pasteData, "bundleIndex", 0);
    EXPECT_EQ(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: GroupRecordWithFrom_002.
 * @tc.desc: record->GetFrom() > 0, add to groupMap.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, GroupRecordWithFrom_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    auto record = PasteDataRecord::NewHtmlRecord("<html></html>");
    record->SetFrom(1);
    pasteData.AddRecord(record);
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto countBefore = pasteData.GetRecordCount();
    controller.RebuildWebviewPasteData(pasteData, "bundleIndex", 0);
    EXPECT_LE(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: RemoveExtraUris_001.
 * @tc.desc: GetFrom() > 0 && MimeType == URI, remove.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, RemoveExtraUris_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    pasteData.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    auto htmlRecord = PasteDataRecord::NewHtmlRecord("<html></html>");
    htmlRecord->SetFrom(0);
    pasteData.AddRecord(htmlRecord);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>("file:///test.png");
    builder.SetUri(uri);
    auto uriRecord = builder.Build();
    uriRecord->SetFrom(1);
    pasteData.AddRecord(uriRecord);
    auto countBefore = pasteData.GetRecordCount();
    controller.RebuildWebviewPasteData(pasteData, "bundleIndex", 0);
    EXPECT_LT(pasteData.GetRecordCount(), countBefore);
}

/**
 * @tc.name: IsLocalURI_001.
 * @tc.desc: relative path without scheme is treated as local.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsLocalURI_001, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body><img src=\"/data/test.png\"></body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsLocalURI_002.
 * @tc.desc: absolute path with file:// scheme is treated as local.
 * @tc.type: FUNC.
 */
HWTEST_F(PasteboardWebControllerTest, IsLocalURI_002, TestSize.Level1)
{
    auto controller = PasteboardWebController::GetInstance();
    PasteData pasteData;
    std::string html = "<html><body><img src=\"file:///data/test.png\"></body></html>";
    auto record = PasteDataRecord::NewHtmlRecord(html);
    record->SetFrom(0);
    pasteData.AddRecord(record);
    bool result = controller.SplitWebviewPasteData(pasteData, "bundleIndex", 100);
    EXPECT_FALSE(result);
}