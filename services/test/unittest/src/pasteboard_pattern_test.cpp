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

/**
 * @tc.name: DetectHttpLinkTest001
 * @tc.desc: Test HTTP_URL detection with valid URL (https:// with letter start)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest001, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_001";
    std::string plainText = "https://www.example.com/path";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest002
 * @tc.desc: Test HTTP_URL with digit start after protocol
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest002, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_002";
    std::string plainText = "http://123.example.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest003
 * @tc.desc: Test HTTP_URL with special character start (invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest003, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_003";
    std::string plainText = "https:/";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 0);
}

/**
 * @tc.name: DetectHttpLinkTest004
 * @tc.desc: Test HTTP_URL with hyphen start (invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest004, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_004";
    std::string plainText = "http://-example.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest005
 * @tc.desc: Test HTTP_URL detection with valid URL (length >= 7 with dot)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest005, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_005";
    std::string plainText = "http://a.b";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectFlightNumberTest007
 * @tc.desc: Test FLIGHT_NUMBER with 2 letters + 4 digits (6 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest001, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_001b";
    std::string plainText = "CZ5678 is my flight";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectFlightNumberTest002
 * @tc.desc: Test FLIGHT_NUMBER with 1 digit + 1 letter + 3 digits (5 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest002, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_002";
    std::string plainText = "1A123 departure time";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectFlightNumberTest003
 * @tc.desc: Test FLIGHT_NUMBER with 2 letters + 3 digits + 1 letter (6 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest003, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_003";
    std::string plainText = "CA123A booking";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectFlightNumberTest004
 * @tc.desc: Test FLIGHT_NUMBER with 2 letters + 4 digits + 1 letter (7 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest004, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_004";
    std::string plainText = "CA1234A flight";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectFlightNumberTest005
 * @tc.desc: Test FLIGHT_NUMBER with 1 digit + 1 letter + 4 digits (6 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest005, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_005";
    std::string plainText = "1A1234";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectFlightNumberTest006
 * @tc.desc: Test FLIGHT_NUMBER with 1 digit + 1 letter + 4 digits + 1 letter (7 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest006, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_006";
    std::string plainText = "1A1234A";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectHttpLinkTest006
 * @tc.desc: Test HTTP_URL and FLIGHT_NUMBER together with multiple valid patterns
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest006, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL, Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_mixed_001";
    std::string plainText = "Visit https://example.com and take CA1234";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectOtherPatternTest001
 * @tc.desc: Test that other patterns (URL, NUMBER, EMAIL_ADDRESS) still work correctly
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectOtherPatternTest001, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::URL, Pattern::NUMBER, Pattern::EMAIL_ADDRESS };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_other_001";
    std::string plainText = "Contact test@example.com or call +123.456. Visit http://files.server.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::URL), 1);
    ASSERT_EQ(result.count(Pattern::NUMBER), 1);
    ASSERT_EQ(result.count(Pattern::EMAIL_ADDRESS), 1);
}

/**
 * @tc.name: DetectRegexNoMatchTest001
 * @tc.desc: Test HTTP_URL with text that does not match regex at all
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectRegexNoMatchTest001, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_nomatch_001";
    std::string plainText = "This is just plain text with no URLs or http links";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 0);
}

/**
 * @tc.name: DetectRegexNoMatchTest002
 * @tc.desc: Test FLIGHT_NUMBER with text that does not match regex at all
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectRegexNoMatchTest002, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_nomatch_002";
    std::string plainText = "No flight numbers here, just some text like A1 or BC123";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 1);
}

/**
 * @tc.name: DetectHttpLinkTest007
 * @tc.desc: Test HTTP_URL with regex match but fails length <= 7 validation
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest007, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_007";
    std::string plainText = "http://ab.c";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectFlightNumberTest007
 * @tc.desc: Test FLIGHT_NUMBER with valid regex match but fails length validation (7 chars)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest007, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_007";
    std::string plainText = "CA123456 is too long";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
}

/**
 * @tc.name: DetectHttpLinkTest008
 * @tc.desc: Test HTTP_URL with valid regex match and length but no dot (invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest008, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_008";
    std::string plainText = "https://examplecom";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectMixedPatternsTest001
 * @tc.desc: Test multiple patterns with only some matching
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectMixedPatternsTest001, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL, Pattern::FLIGHT_NUMBER, Pattern::URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_mixed_002";
    std::string plainText = "Visit htp://example.com and no valid http links or flight numbers";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 0);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
    ASSERT_EQ(result.count(Pattern::URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest009
 * @tc.desc: Test HTTP_URL with http:// protocol
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest009, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_009";
    std::string plainText = "http://example.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectFlightNumberTest008
 * @tc.desc: Test FLIGHT_NUMBER with too short (4 chars, invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest008, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_008";
    std::string plainText = "CA12";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
}

/**
 * @tc.name: DetectFlightNumberTest009
 * @tc.desc: Test FLIGHT_NUMBER with too long (8 chars, invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest009, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_009";
    std::string plainText = "CA123456A";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
}

/**
 * @tc.name: DetectFlightNumberTest010
 * @tc.desc: Test FLIGHT_NUMBER with invalid format (3 letters)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest010, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_010";
    std::string plainText = "ABC123";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
}

/**
 * @tc.name: DetectFlightNumberTest011
 * @tc.desc: Test FLIGHT_NUMBER with invalid format (2 digits)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest011, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_011";
    std::string plainText = "12A123";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
}

/**
 * @tc.name: DetectFlightNumberTest012
 * @tc.desc: Test FLIGHT_NUMBER with 2 letters + 3 digits + 1 letter (6 chars, lowercase)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectFlightNumberTest012, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::FLIGHT_NUMBER };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_flight_012";
    std::string plainText = "ca123a booking";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::FLIGHT_NUMBER), 0);
}

/**
 * @tc.name: DetectHttpLinkTest010
 * @tc.desc: Test HTTP_URL with single character after protocol (http://s)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest010, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_010";
    std::string plainText = "http://s";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest011
 * @tc.desc: Test HTTP_URL with single character after protocol (https://s)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest011, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_011";
    std::string plainText = "https://s";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest012
 * @tc.desc: Test HTTP_URL with single digit after protocol (http://1)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest012, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_012";
    std::string plainText = "http://1";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest013
 * @tc.desc: Test HTTP_URL with prefix before protocol (invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest013, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_013";
    std::string plainText = "ceshihttp://aa.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 0);
}

/**
 * @tc.name: DetectHttpLinkTest014
 * @tc.desc: Test HTTP_URL with space before protocol
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest014, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_014";
    std::string plainText = "AC1234 http://ac.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 1);
}

/**
 * @tc.name: DetectHttpLinkTest015
 * @tc.desc: Test HTTP_URL with comma before protocol (invalid)
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardPatternTest, DetectHttpLinkTest015, TestSize.Level1)
{
    const std::set<Pattern> patternsToCheck = { Pattern::HTTP_URL };
    PasteData pasteData;
    auto record = std::make_shared<PasteDataRecord>();
    std::string utdId = "utd_http_015";
    std::string plainText = "text,http://example.com";
    auto plainEntry = std::make_shared<PasteDataEntry>(
        utdId,
        MIMETYPE_TEXT_PLAIN,
        plainText
    );
    record->AddEntry(utdId, plainEntry);
    pasteData.AddRecord(record);
    std::set<Pattern> result = PatternDetection::Detect(patternsToCheck, pasteData, false, true);
    ASSERT_EQ(result.count(Pattern::HTTP_URL), 0);
}
} // namespace OHOS::MiscServices