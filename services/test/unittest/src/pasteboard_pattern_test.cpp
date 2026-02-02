/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#include <gmock/gmock.h>
#include "pasteboard_client.h"
#include "paste_data_record.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_observer_callback.h"
#include "pasteboard_pattern.h"
#include "pixel_map.h"
#include "uri.h"
#include "want.h"
namespace OHOS::MiscServices {
using namespace testing;
using namespace testing::ext;
class PasteboardPatternTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardPatternTest::SetUpTestCase(void) { }

void PasteboardPatternTest::TearDownTestCase(void) { }

void PasteboardPatternTest::SetUp(void) { }

void PasteboardPatternTest::TearDown(void) { }

/**
 * @tc.name: PasteboardPatternTest001
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest001 start");
    bool hasPlain = true;
    bool hasHTML = true;

    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get.");

    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == plainText);

    const std::set<Pattern> patternsToCheck = {};

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest001 end");
}

/**
 * @tc.name: PasteboardPatternTest002
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest002 start");
    bool hasPlain = true;
    bool hasHTML = true;

    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get.");

    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == plainText);

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest002 end");
}

/**
 * @tc.name: PasteboardPatternTest003
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest003 start");
    bool hasPlain = true;
    bool hasHTML = false;

    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get.");

    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == plainText);

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest003 end");
}

/**
 * @tc.name: PasteboardPatternTest004
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest004, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest004 start");
    bool hasPlain = false;
    bool hasHTML = true;

    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get.");

    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == plainText);

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest004 end");
}

/**
 * @tc.name: PasteboardPatternTest005
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest005, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest005 start");
    bool hasPlain = false;
    bool hasHTML = false;

    std::string plainText = "plain text";
    auto data = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "get.");

    auto primaryText = pasteData.GetPrimaryText();
    ASSERT_TRUE(primaryText != nullptr);
    ASSERT_TRUE(*primaryText == plainText);

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest005 end");
}

/**
 * @tc.name: PasteboardPatternTest006
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest006, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest006 start");
    bool hasPlain = true;
    bool hasHTML = true;

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

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest006 end");
}

/**
 * @tc.name: PasteboardPatternTest007
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest007, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest007 start");
    bool hasPlain = true;
    bool hasHTML = false;

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

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest007 end");
}

/**
 * @tc.name: PasteboardPatternTest008
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest008, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest008 start");
    bool hasPlain = false;
    bool hasHTML = true;

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

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest008 end");
}

/**
 * @tc.name: PasteboardPatternTest009
 * @tc.desc: detect test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest009, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest009 start");
    bool hasPlain = false;
    bool hasHTML = true;

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

    const std::set<Pattern> patternsToCheck = { Pattern::URL };

    auto detect = PatternDetection::Detect(patternsToCheck, pasteData, hasHTML, hasPlain);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest009 end");
}

/**
 * @tc.name: PasteboardPatternTest010
 * @tc.desc: IsValid test.
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, PasteboardPatternTest010, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest010 start");

    std::set<Pattern> patterns = { Pattern::COUNT };
    bool isValid = PatternDetection::IsValid(patterns);
    EXPECT_FALSE(isValid);

    patterns = { Pattern::URL };
    isValid = PatternDetection::IsValid(patterns);
    EXPECT_TRUE(isValid);

    patterns = { Pattern::NUMBER };
    isValid = PatternDetection::IsValid(patterns);
    EXPECT_TRUE(isValid);

    patterns = { Pattern::EMAIL_ADDRESS };
    isValid = PatternDetection::IsValid(patterns);
    EXPECT_TRUE(isValid);

    patterns = { Pattern::URL, Pattern::COUNT };
    isValid = PatternDetection::IsValid(patterns);
    EXPECT_FALSE(isValid);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "PasteboardPatternTest010 end");
}

/**
 * @tc.name: DetectPlainTextTest001
 * @tc.desc: DetectPlainText covers branch: patternsOut.find(pattern) != patternsOut.end()
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectPlainTextTest001, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::URL, Pattern::NUMBER };
    PasteData pasteData;
    auto record1 = std::make_shared<PasteDataRecord>();
    std::string utdId1 = "utd_001";
    std::string plainText1 = "https://ohos.com";
    auto plainEntry1 = std::make_shared<PasteDataEntry>(
        utdId1,
        MIMETYPE_TEXT_PLAIN,
        plainText1
    );
    record1->AddEntry(utdId1, plainEntry1);
    pasteData.AddRecord(record1);

    auto record2 = std::make_shared<PasteDataRecord>();
    std::string utdId2 = "utd_002";
    std::string plainText2 = "https://ohos.com 123456";
    auto plainEntry2 = std::make_shared<PasteDataEntry>(
        utdId2,
        MIMETYPE_TEXT_PLAIN,
        plainText2
    );
    record2->AddEntry(utdId2, plainEntry2);
    pasteData.AddRecord(record2);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, true, false);
    ASSERT_EQ(result.count(Pattern::URL), 1);
    ASSERT_EQ(result.count(Pattern::NUMBER), 1);
}

/**
 * @tc.name: DetectPlainTextTest002
 * @tc.desc: DetectPlainText covers branch: if (it == patterns_.end())
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectPlainTextTest002, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { static_cast<Pattern>(0xFF) };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_003";
    std::string plainText = "test text";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, true, false);
    ASSERT_TRUE(result.empty());
}

/**
 * @tc.name: ExtractHtmlContentTest001
 * @tc.desc: Cover the branch of 'if (rootNode == nullptr)' in ExtractHtmlContent function
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, ExtractHtmlContentTest001, TestSize.Level1)
{
    std::string invalidHtml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    const std::set<Pattern> patternsToCheck = { Pattern::URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_html_001";
    auto htmlEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_HTML,
        invalidHtml
    );
    record->AddEntry(utdId, htmlEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_TRUE(result.empty());
}

/**
 * @tc.name: ExtractHtmlContentTest002
 * @tc.desc: Cover the branch of 'if (xmlStr == nullptr)' in ExtractHtmlContent function
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, ExtractHtmlContentTest002, TestSize.Level1)
{
    std::string emptyContentHtml = "<html></html>";
    const std::set<Pattern> patternsToCheck = { Pattern::URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_html_002";
    auto htmlEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_HTML,
        emptyContentHtml
    );
    record->AddEntry(utdId, htmlEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_TRUE(result.empty());
}
} // namespace OHOS::MiscServices