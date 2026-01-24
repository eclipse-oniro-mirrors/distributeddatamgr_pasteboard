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