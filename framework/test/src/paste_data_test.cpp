/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <gtest/gtest.h>

#include "clip/clip_plugin.h"
#include "clip_factory.h"
#include "common/block_object.h"
#include "int_wrapper.h"
#include "pasteboard_client.h"
#include "pasteboard_service_loader.h"
#include "want_params_wrapper.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::AAFwk;
using namespace OHOS::Media;
constexpr const int32_t INVALID_FD = -1;
constexpr const char *FILE_URI = "/data/test/resource/pasteboardTest.txt";
constexpr const char *REMOTE_FILE_SIZE_LONG = "remoteFileSizeLong";
constexpr const char *REMOTE_FILE_SIZE = "remoteFileSize";
class PasteDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteDataTest::SetUpTestCase(void) { }

void PasteDataTest::TearDownTestCase(void) { }

void PasteDataTest::SetUp(void) { }

void PasteDataTest::TearDown(void) { }

ClipFactory::ClipFactory()
{
    ClipPlugin::RegCreator("distributed_clip", this);
}

/**
 * @tc.name: AddRecord001
 * @tc.desc: PasteDataRecord AddRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, AddRecord001, TestSize.Level0)
{
    PasteData data;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::string uriStr = FILE_URI;
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    builder.SetUri(uri);
    auto record = builder.Build();
    EXPECT_TRUE(record != nullptr);
    data.AddRecord(nullptr);
    auto count = data.GetRecordCount();
    EXPECT_TRUE(count == 0);
}

/**
 * @tc.name: AddRecord002
 * @tc.desc: PasteDataRecord AddRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, AddRecord002, TestSize.Level0)
{
    PasteData data;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::string uriStr = FILE_URI;
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    builder.SetUri(uri);
    auto record = builder.Build();
    data.AddRecord(*record);
    auto count = data.GetRecordCount();
    EXPECT_TRUE(count == 1);
}

/**
 * @tc.name: AddRecord003
 * @tc.desc: PasteDataRecord AddRecord, add multi records and check the order of records of data.
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, AddRecord003, TestSize.Level0)
{
    std::vector<std::string> testTextVec = { "text001", "text002", "text003" };
    std::vector<std::string> expectTextVec = { "text003", "text002", "text001" };
    PasteData data;
    for (const auto &itText : testTextVec) {
        PasteDataRecord::Builder builder(MIMETYPE_TEXT_PLAIN);
        auto text = std::make_shared<std::string>(itText);
        auto record = builder.SetPlainText(text).Build();
        ASSERT_TRUE(record != nullptr);
        data.AddRecord(*record);
    }

    auto count = data.GetRecordCount();
    EXPECT_TRUE(count == testTextVec.size());

    std::vector<std::string> result;
    for (const auto &itRec : data.AllRecords()) {
        ASSERT_TRUE(itRec != nullptr);
        EXPECT_TRUE(itRec->GetMimeType() == MIMETYPE_TEXT_PLAIN);
        auto plainText = itRec->GetPlainTextV0();
        ASSERT_TRUE(plainText != nullptr);
        result.emplace_back(*plainText);
    }

    EXPECT_TRUE(result.size() == expectTextVec.size());
    EXPECT_TRUE(std::equal(result.begin(), result.end(), expectTextVec.begin(), expectTextVec.end()));
}

/**
 * @tc.name: AddRecord004
 * @tc.desc: PasteDataRecord AddHtmlRecord, add multi records and check the order of records of data.
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, AddRecord004, TestSize.Level0)
{
    std::vector<std::string> testHtmlVec = {
        "<div class='disable'>html001</div>",
        "<div class='disable'>html002</div>",
        "<div class='disable'>html003</div>"
    };
    std::vector<std::string> expectHtmlVec = {
        "<div class='disable'>html003</div>",
        "<div class='disable'>html002</div>",
        "<div class='disable'>html001</div>"
    };
    PasteData data;
    for (const auto &itHtml: testHtmlVec) {
        data.AddHtmlRecord(itHtml);
    }

    auto count = data.GetRecordCount();
    EXPECT_TRUE(count == testHtmlVec.size());

    std::vector<std::string> result;
    for (const auto &itRec : data.AllRecords()) {
        ASSERT_TRUE(itRec != nullptr);
        EXPECT_TRUE(itRec->GetMimeType() == MIMETYPE_TEXT_HTML);
        auto htmlText = itRec->GetHtmlTextV0();
        ASSERT_TRUE(htmlText != nullptr);
        result.emplace_back(*htmlText);
    }

    EXPECT_TRUE(result.size() == expectHtmlVec.size());
    EXPECT_TRUE(std::equal(result.begin(), result.end(), expectHtmlVec.begin(), expectHtmlVec.end()));
}

/**
 * @tc.name: AddRecord005
 * @tc.desc: PasteDataRecord AddTextRecord, add multi records and check the order of records of data.
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, AddRecord005, TestSize.Level0)
{
    std::vector<std::string> testTextVec = { "text101", "text102", "text103" };
    std::vector<std::string> expectTextVec = { "text103", "text102", "text101" };
    PasteData data;
    for (const auto &itText: testTextVec) {
        data.AddTextRecord(itText);
    }

    auto count = data.GetRecordCount();
    EXPECT_TRUE(count == testTextVec.size());

    std::vector<std::string> result;
    for (const auto &itRec : data.AllRecords()) {
        ASSERT_TRUE(itRec != nullptr);
        EXPECT_TRUE(itRec->GetMimeType() == MIMETYPE_TEXT_PLAIN);
        auto plainText = itRec->GetPlainTextV0();
        ASSERT_TRUE(plainText != nullptr);
        result.emplace_back(*plainText);
    }

    EXPECT_TRUE(result.size() == expectTextVec.size());
    EXPECT_TRUE(std::equal(result.begin(), result.end(), expectTextVec.begin(), expectTextVec.end()));
}

/**
 * @tc.name: AddRecord006
 * @tc.desc: PasteDataRecord AddUriRecord, add multi records and check the order of records of data.
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, AddRecord006, TestSize.Level0)
{
    std::vector<std::string> testUriVec = {
        "file://pasteboard_service/test_uri_001",
        "file://pasteboard_service/test_uri_002",
        "file://pasteboard_service/test_uri_003"
    };
    std::vector<std::string> expectUriVec = {
        "file://pasteboard_service/test_uri_003",
        "file://pasteboard_service/test_uri_002",
        "file://pasteboard_service/test_uri_001"
    };
    PasteData data;
    for (const auto &itUri: testUriVec) {
        data.AddUriRecord(OHOS::Uri(itUri));
    }

    auto count = data.GetRecordCount();
    EXPECT_TRUE(count == testUriVec.size());

    std::vector<std::string> result;
    for (const auto &itRec : data.AllRecords()) {
        ASSERT_TRUE(itRec != nullptr);
        EXPECT_TRUE(itRec->GetMimeType() == MIMETYPE_TEXT_URI);
        auto uri = itRec->GetUriV0();
        ASSERT_TRUE(uri != nullptr);
        result.emplace_back(uri->ToString());
    }

    EXPECT_TRUE(result.size() == expectUriVec.size());
    EXPECT_TRUE(std::equal(result.begin(), result.end(), expectUriVec.begin(), expectUriVec.end()));
}

/**
 * @tc.name: Marshalling001
 * @tc.desc: PasteData Marshalling
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, Marshalling001, TestSize.Level0)
{
    PasteData data1;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::string uriStr = FILE_URI;
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    builder.SetUri(uri);
    auto record = builder.Build();
    data1.AddRecord(*record);
    auto count = data1.GetRecordCount();
    EXPECT_TRUE(count == 1);
    Parcel parcel;
    data1.Marshalling(parcel);

    auto data2 = PasteData::Unmarshalling(parcel);
    auto count2 = data2->GetRecordCount();
    EXPECT_TRUE(count == count2);
    std::shared_ptr<OHOS::Uri> uri2 = data2->GetPrimaryUri();
    std::string uriStr2 = uri2->ToString();
    EXPECT_TRUE(uriStr == uriStr2);
}

/**
 * @tc.name: MaxLength001
 * @tc.desc: PasteDataRecord: maxLength NewHtmlRecord
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, MaxLength001, TestSize.Level0)
{
    int maxLength = 100 * 1024 * 1024 + 1;
    std::string res = "hello";
    std::string temp = "world";
    for (int i = 0; i < maxLength; i++) {
        res += temp;
    }
    std::string htmlText = "<div class='disabled'>" + res + "</div>";
    auto record = PasteboardClient::GetInstance()->CreateHtmlTextRecord(htmlText);
    ASSERT_TRUE(record == nullptr);
}

/**
 * @tc.name: MaxLength002
 * @tc.desc: PasteDataRecord: maxLength NewPlainTextRecord
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, MaxLength002, TestSize.Level0)
{
    int maxLength = 100 * 1024 * 1024 + 1;
    std::string plainText = "hello";
    std::string temp = "world";
    for (int i = 0; i < maxLength; i++) {
        plainText += temp;
    }
    auto record = PasteboardClient::GetInstance()->CreatePlainTextRecord(plainText);
    ASSERT_TRUE(record == nullptr);
}

/**
 * @tc.name: ConvertToText001
 * @tc.desc: PasteDataRecord: ConvertToText htmlText
 * @tc.type: FUNC
 * @tc.require: AR000HEECD
 * @tc.author: chenyu
 */
HWTEST_F(PasteDataTest, ConvertToText001, TestSize.Level0)
{
    std::string htmlText = "<div class='disabled item tip user-programs'>";
    auto record = PasteboardClient::GetInstance()->CreateHtmlTextRecord(htmlText);
    ASSERT_TRUE(record != nullptr);
    auto text = record->ConvertToText();
    EXPECT_EQ(text, htmlText);
}

/**
 * @tc.name: ConvertToText002
 * @tc.desc: PasteDataRecord: ConvertToText plainText
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ConvertToText002, TestSize.Level0)
{
    std::string plainText = "paste record test";
    auto record = PasteboardClient::GetInstance()->CreatePlainTextRecord(plainText);
    ASSERT_TRUE(record != nullptr);
    auto text = record->ConvertToText();
    EXPECT_EQ(text, plainText);
}

/**
 * @tc.name: ConvertToText003
 * @tc.desc: PasteDataRecord: ConvertToText uri
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ConvertToText003, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto record = PasteboardClient::GetInstance()->CreateUriRecord(uri);
    ASSERT_TRUE(record != nullptr);
    auto text = record->ConvertToText();
    EXPECT_EQ(text, uri.ToString());
}

/**
 * @tc.name: ConvertToText004
 * @tc.desc: PasteDataRecord: ConvertToText uri
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ConvertToText004, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, 100, opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto record = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(record != nullptr);
    auto text = record->ConvertToText();
    EXPECT_EQ(text, "");
}

/**
 * @tc.name: ConvertToText005
 * @tc.desc: PasteDataRecord: ConvertToText plain
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ConvertToText005, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, 100, opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto record = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(record != nullptr);
    std::string plainText = "hello";
    auto plainUtdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, MIMETYPE_TEXT_PLAIN);
    record->AddEntryByMimeType(MIMETYPE_TEXT_PLAIN, std::make_shared<PasteDataEntry>(plainUtdId, plainText));
    auto text = record->ConvertToText();
    EXPECT_EQ(text, plainText);
}

/**
 * @tc.name: ConvertToText006
 * @tc.desc: PasteDataRecord: ConvertToText html
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ConvertToText006, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, 100, opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto record = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(record != nullptr);
    std::string htmlText = "<div class='disabled item tip'>";
    auto htmlUtdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, MIMETYPE_TEXT_HTML);
    record->AddEntryByMimeType(MIMETYPE_TEXT_HTML, std::make_shared<PasteDataEntry>(htmlUtdId, htmlText));
    auto text = record->ConvertToText();
    EXPECT_EQ(text, htmlText);
}

/**
 * @tc.name: ConvertToText007
 * @tc.desc: PasteDataRecord: ConvertToText uri
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ConvertToText007, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, 100, opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto record = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(record != nullptr);
    std::string uri = "file://123.txt";
    auto uriUtdId = CommonUtils::Convert2UtdId(UDMF::UDType::UD_BUTT, MIMETYPE_TEXT_URI);
    record->AddEntryByMimeType(MIMETYPE_TEXT_URI, std::make_shared<PasteDataEntry>(uriUtdId, uri));
    auto text = record->ConvertToText();
    EXPECT_EQ(text, uri);
}

/**
 * @tc.name: GetPasteDataMsg001
 * @tc.desc: PasteData: GetPrimaryMimeType is nullptr and so on
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetPasteDataMsg001, TestSize.Level0)
{
    std::string plainText1 = "helloWorld";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText1);
    ASSERT_TRUE(pasteData != nullptr);
    auto newPrimaryPixelMap = pasteData->GetPrimaryPixelMap();
    ASSERT_TRUE(newPrimaryPixelMap == nullptr);
    auto newPrimaryMimeType = pasteData->GetPrimaryMimeType();
    ASSERT_TRUE(newPrimaryMimeType != nullptr);
    auto newPasteData = std::make_shared<PasteData>();
    auto newPrimaryMimeType2 = newPasteData->GetPrimaryMimeType();
    ASSERT_TRUE(newPrimaryMimeType2 == nullptr);
    std::string plainText2 = "plain text";
    auto record = PasteboardClient::GetInstance()->CreatePlainTextRecord(plainText2);
    ASSERT_TRUE(record != nullptr);
    ASSERT_FALSE(pasteData->ReplaceRecordAt(1000, record));
}

/**
 * @tc.name: GetPasteDataMsg002
 * @tc.desc: PasteData: GetPrimaryWant is nullptr and so on
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetPasteDataMsg002, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto newPasteData = PasteboardClient::GetInstance()->CreatePixelMapData(pixelMapIn);
    ASSERT_TRUE(newPasteData != nullptr);
    auto pixMap = newPasteData->GetPrimaryPixelMap();
    ASSERT_TRUE(pixMap != nullptr);
    auto primaryWant = newPasteData->GetPrimaryWant();
    ASSERT_TRUE(primaryWant == nullptr);
    auto primaryText = newPasteData->GetPrimaryText();
    ASSERT_TRUE(primaryText == nullptr);
    auto primaryUri = newPasteData->GetPrimaryUri();
    ASSERT_TRUE(primaryUri == nullptr);
    auto record = newPasteData->GetRecordAt(1);
    ASSERT_TRUE(record == nullptr);
    auto res1 = newPasteData->RemoveRecordAt(1);
    ASSERT_FALSE(res1);
    std::string mimeType = "text/plain";
    ASSERT_FALSE(newPasteData->HasMimeType(mimeType));
}

/**
 * @tc.name: ShareOptionToString001
 * @tc.desc: PasteData: ShareOptionToString
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ShareOptionToString001, TestSize.Level0)
{
    std::string shareOption1;
    PasteData::ShareOptionToString(ShareOption::InApp, shareOption1);
    ASSERT_TRUE(shareOption1 == "InAPP");
    std::string shareOption2;
    PasteData::ShareOptionToString(ShareOption::LocalDevice, shareOption2);
    ASSERT_TRUE(shareOption2 == "LocalDevice");
    std::string shareOption3;
    PasteData::ShareOptionToString(ShareOption::CrossDevice, shareOption3);
    ASSERT_TRUE(shareOption3 == "CrossDevice");
}

/**
 * @tc.name: SetInvalid001
 * @tc.desc: PasteData: SetInvalid001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetInvalid001, TestSize.Level0)
{
    bool result = true;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->SetInvalid();
    result = pasteData->IsValid();
    ASSERT_FALSE(result);
}

/**
 * @tc.name: SetLocalOnly001
 * @tc.desc: PasteData: SetLocalOnly
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetLocalOnly001, TestSize.Level0)
{
    bool result = false;
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    pasteData->SetLocalOnly(true);
    result = pasteData->GetLocalOnly();
    ASSERT_TRUE(result);
}

/**
 * @tc.name: SetAddition001
 * @tc.desc: PasteData: SetAddition
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetAddition001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    int64_t fileSize = 0L;
    pasteData->SetFileSize(fileSize);
    AAFwk::WantParams additions;
    pasteData->SetAdditions(additions);
    ASSERT_TRUE(pasteData != nullptr);
}

/**
 * @tc.name: SetRemote001
 * @tc.desc: PasteData: SetRemote
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetRemote001, TestSize.Level0)
{
    bool isRemote = false;
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    isRemote = true;
    pasteData->SetRemote(isRemote);
    bool result = pasteData->IsRemote();
    ASSERT_TRUE(result);
}

/**
 * @tc.name: SetOriginAuthority001
 * @tc.desc: PasteData: SetOriginAuthority
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetOriginAuthority001, TestSize.Level0)
{
    std::string plainText = "plain text";
    std::string bundleName = "com.example.myapplication";
    int32_t appIndex = 1;
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    auto bundleIndex = std::make_pair(bundleName, appIndex);
    pasteData->SetBundleInfo(bundleName, appIndex);
    pasteData->SetOriginAuthority(bundleIndex);
    std::string getBundleName = pasteData->GetBundleName();
    int32_t getAppIndex = pasteData->GetAppIndex();
    auto getOriginAuthority = pasteData->GetOriginAuthority();
    ASSERT_TRUE(getBundleName == bundleName);
    ASSERT_TRUE(getAppIndex == appIndex);
    ASSERT_TRUE(getOriginAuthority == bundleIndex);
    std::string time = "2023-08-09";
    pasteData->SetTime(time);
    std::string getTime = pasteData->GetTime();
    ASSERT_TRUE(getTime == time);
}

/**
 * @tc.name: GetConvertUri001
 * @tc.desc: PasteDataRecord: GetConvertUri
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetConvertUri001, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    std::string convertUri_ = "/mnt/hmdfs/";
    pasteDataRecord->SetConvertUri(convertUri_);
    std::string result = pasteDataRecord->GetConvertUri();
    ASSERT_TRUE(result == convertUri_);
    std::string newUriStr = "/mnt/hmdfs/test";
    pasteDataRecord->SetUri(std::make_shared<OHOS::Uri>(newUriStr));
    std::shared_ptr<Uri> uri = pasteDataRecord->GetUriV0();
    ASSERT_TRUE(uri != nullptr);
    std::shared_ptr<Uri> getOriginUri = pasteDataRecord->GetOriginUri();
    ASSERT_TRUE(getOriginUri != nullptr);
}

/**
 * @tc.name: GetConvertUri002
 * @tc.desc: PasteDataRecord: GetConvertUri
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetConvertUri002, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    std::string convertUri_ = "";
    pasteDataRecord->SetConvertUri(convertUri_);
    std::string result = pasteDataRecord->GetConvertUri();
    ASSERT_TRUE(result == convertUri_);
}

/**
 * @tc.name: HasGrantUriPermission001
 * @tc.desc: PasteDataRecord: HasGrantUriPermission
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, HasGrantUriPermission001, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 1, 2, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
    pasteDataRecord->SetGrantUriPermission(true);
    auto hasGrantUriPermission_ = pasteDataRecord->HasGrantUriPermission();
    ASSERT_TRUE(hasGrantUriPermission_);
}

/**
 * @tc.name: LoadSystemAbilityFail001
 * @tc.desc: PasteDataRecord: LoadSystemAbilityFail
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, LoadSystemAbilityFail001, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    std::string mimeType = "image/jpg";
    arrayBuffer = { 1, 2, 3, 4, 6 };
    PasteboardServiceLoader::GetInstance().LoadSystemAbilityFail();
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
}

/**
 * @tc.name: LoadSystemAbilitySuccess001
 * @tc.desc: PasteDataRecord: LoadSystemAbilitySuccess
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, LoadSystemAbilitySuccess001, TestSize.Level0)
{
    std::vector<uint8_t> arrayBuffer(46);
    std::string mimeType = "image/jpg";
    arrayBuffer = { 1, 2, 3, 4, 6 };
    sptr<IRemoteObject> remoteObject = nullptr;
    PasteboardServiceLoader::GetInstance().LoadSystemAbilitySuccess(remoteObject);
    auto pasteDataRecord = PasteboardClient::GetInstance()->CreateKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(pasteDataRecord != nullptr);
}

/**
 * @tc.name: SetInterval001
 * @tc.desc: BlockObject: SetInterval
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetInterval001, TestSize.Level0)
{
    uint32_t POPUP_INTERVAL = 1000;
    auto block = std::make_shared<BlockObject<std::shared_ptr<PasteData>>>(POPUP_INTERVAL);
    std::shared_ptr<PasteData> pasteData = std::make_shared<PasteData>();
    block->SetValue(pasteData);
    block->SetInterval(POPUP_INTERVAL);
    auto value = block->GetValue();
    EXPECT_TRUE(value != nullptr);
}

/**
 * @tc.name: ClipPlugin001
 * @tc.desc: API_EXPORT: ClipPlugin
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ClipPlugin001, TestSize.Level0)
{
    std::string PLUGIN_NAME_VAL = "distributed_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    auto clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    ClipPlugin::Factory *factory = nullptr;
    auto result = ClipPlugin::RegCreator(PLUGIN_NAME_VAL, factory);
    EXPECT_FALSE(result);
    auto userId = 10000;
    auto events1 = clipPlugin_->GetTopEvents(1, userId);
    EXPECT_TRUE(events1.size() == 0);
    auto events2 = clipPlugin_->GetTopEvents(1);
    EXPECT_TRUE(events2.size() == 0);
    clipPlugin_->Clear();
    clipPlugin_->Clear(userId);
}

/**
 * @tc.name: ClipPlugin002
 * @tc.desc: API_EXPORT: ClipPlugin
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ClipPlugin002, TestSize.Level0)
{
    std::string PLUGIN_NAME_VAL = "distributed_clip";
    auto release = [&PLUGIN_NAME_VAL, this](ClipPlugin *plugin) {
        ClipPlugin::DestroyPlugin(PLUGIN_NAME_VAL, plugin);
    };
    auto clipPlugin_ = std::shared_ptr<ClipPlugin>(ClipPlugin::CreatePlugin(PLUGIN_NAME_VAL), release);
    ClipPlugin::Factory *factory = new ClipFactory();
    auto result = ClipPlugin::RegCreator(PLUGIN_NAME_VAL, factory);
    EXPECT_FALSE(result);
    auto userId = 3701;
    auto events1 = clipPlugin_->GetTopEvents(1, userId);
    EXPECT_TRUE(events1.size() == 0);
    clipPlugin_->Clear(userId);
}

/**
 * @tc.name: ClipPlugin003
 * @tc.desc: API_EXPORT: ClipPlugin
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ClipPlugin003, TestSize.Level0)
{
    ClipPlugin::GlobalEvent event1;
    event1.seqId = 0;
    event1.deviceId = "test_device_id";
    event1.user = 0;
    ClipPlugin::GlobalEvent event2;
    event2.seqId = 0;
    event2.deviceId = "test_device_id";
    event2.user = 1;
    EXPECT_TRUE(event1 == event2);
}

/**
 * @tc.name: ClipPlugin004
 * @tc.desc: API_EXPORT: ClipPlugin
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ClipPlugin004, TestSize.Level0)
{
    ClipPlugin::GlobalEvent event1;
    event1.seqId = 0;
    event1.deviceId = "test_device_id";
    event1.user = 0;
    ClipPlugin::GlobalEvent event2;
    event2.seqId = 0;
    event2.deviceId = "test_device_id1";
    event2.user = 1;
    EXPECT_FALSE(event1 == event2);
}

/**
 * @tc.name: ClipPlugin005
 * @tc.desc: API_EXPORT: ClipPlugin
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ClipPlugin005, TestSize.Level0)
{
    ClipPlugin::GlobalEvent event1;
    event1.seqId = 0;
    event1.deviceId = "test_device_id";
    event1.user = 0;
    ClipPlugin::GlobalEvent event2;
    event2.seqId = 1;
    event2.deviceId = "test_device_id";
    event2.user = 1;
    EXPECT_FALSE(event1 == event2);
}

/**
 * @tc.name: PasteDataOperator001
 * @tc.desc: PasteData: operator
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, PasteDataOperator001, TestSize.Level0)
{
    PasteData data1;
    PasteDataRecord::Builder builder1(MIMETYPE_TEXT_URI);
    std::string uriStr1 = FILE_URI;
    auto uri1 = std::make_shared<OHOS::Uri>(uriStr1);
    builder1.SetUri(uri1);
    auto record1 = builder1.Build();
    data1.AddRecord(record1);
    std::string bundleName1 = "com.example.myapplication";
    int32_t appIndex1 = 1;
    data1.SetOriginAuthority({bundleName1, appIndex1});
    PasteData data2;
    PasteDataRecord::Builder builder2(MIMETYPE_TEXT_URI);
    std::string uriStr2 = FILE_URI;
    auto uri2 = std::make_shared<OHOS::Uri>(uriStr2);
    builder2.SetUri(uri2);
    auto record2 = builder2.Build();
    data2.AddRecord(record2);
    std::string bundleName2 = "com.example.myapplication";
    int32_t appIndex2 = 1;
    data2.SetOriginAuthority({bundleName2, appIndex2});
    ASSERT_TRUE(data1.GetBundleName() == data2.GetBundleName());
    ASSERT_TRUE(data1.GetAppIndex() == data2.GetAppIndex());
}

/**
 * @tc.name: GetShareOption001
 * @tc.desc: GetShareOption call
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetShareOption001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    ShareOption option = InApp;
    pasteData->SetShareOption(option);
    auto result = pasteData->GetShareOption();
    ASSERT_TRUE(result == InApp);
}

/**
 * @tc.name: AddKvRecord001
 * @tc.desc: AddKvRecord call
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, AddKvRecord001, TestSize.Level0)
{
    PasteData data;
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType = "image/jpg";
    data.AddKvRecord(mimeType, arrayBuffer);
    ASSERT_TRUE(data.GetRecordCount() > 0);
}

/**
 * @tc.name: GetProperty001
 * @tc.desc: GetProperty call
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetProperty001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    PasteDataProperty property = pasteData->GetProperty();
    ASSERT_TRUE(property.tokenId == 0);
}

/**
 * @tc.name: SetProperty001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetProperty001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    PasteDataProperty property;
    property.tokenId = 1;
    pasteData->SetProperty(property);
    PasteDataProperty pasteDataProperty = pasteData->GetProperty();
    ASSERT_TRUE(pasteDataProperty.tokenId == 1);
}

/**
 * @tc.name: SetShareOption001
 * @tc.desc: SetShareOption call
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetShareOption001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    ShareOption option = LocalDevice;
    pasteData->SetShareOption(option);
    auto result = pasteData->GetShareOption();
    ASSERT_TRUE(result == LocalDevice);
}

/**
 * @tc.name: SetTokenId001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetTokenId001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    uint32_t tokenId = 1;
    pasteData->SetTokenId(tokenId);
    auto result = pasteData->GetTokenId();
    ASSERT_TRUE(result == 1);
}

/**
 * @tc.name: IsDraggedData001
 * @tc.desc: IsDraggedData call
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, IsDraggedData001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    bool isDraggedData = false;
    pasteData->SetDraggedDataFlag(isDraggedData);
    auto result = pasteData->IsDraggedData();
    ASSERT_FALSE(result);
}

/**
 * @tc.name: SetDraggedDataFlag001
 * @tc.desc: SetDraggedDataFlag call
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetDraggedDataFlag001, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ASSERT_TRUE(pasteData != nullptr);
    bool isDraggedData = true;
    pasteData->SetDraggedDataFlag(isDraggedData);
    auto result = pasteData->IsDraggedData();
    ASSERT_TRUE(result);
}

/**
 * @tc.name: SetScreenStatus
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetScreenStatus, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    ScreenEvent event = ScreenEvent::ScreenLocked;
    pasteData->SetScreenStatus(event);
    ScreenEvent ret = pasteData->GetScreenStatus();
    ASSERT_TRUE(ret == ScreenEvent::ScreenLocked);
}

/**
 * @tc.name: GetMimeTypes
 * @tc.desc: PasteData GetMimeTypes
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetMimeTypes, TestSize.Level0)
{
    PasteData data;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    std::string uriStr = FILE_URI;
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    auto record = builder.SetUri(uri).Build();
    data.AddRecord(*record);

    PasteDataRecord::Builder builder1(MIMETYPE_TEXT_PLAIN);
    std::string plainText = "plain text";
    auto text = std::make_shared<std::string>(plainText);
    auto record1 = builder1.SetPlainText(text).Build();
    data.AddRecord(*record1);

    auto mimeType = data.GetMimeTypes();
    EXPECT_TRUE((strcmp(MIMETYPE_TEXT_PLAIN, mimeType.at(0).c_str()) == 0 &&
                    strcmp(MIMETYPE_TEXT_URI, mimeType.at(1).c_str()) == 0) ||
        (strcmp(MIMETYPE_TEXT_PLAIN, mimeType.at(1).c_str()) == 0 &&
            strcmp(MIMETYPE_TEXT_URI, mimeType.at(0).c_str()) == 0));
}

/**
 * @tc.name: GetReportMimeTypes
 * @tc.desc: PasteData GetReportMimeTypes
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetReportMimeTypes, TestSize.Level0)
{
    PasteData data;
    std::string uriStr = FILE_URI;
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_URI);
    auto uri = std::make_shared<OHOS::Uri>(uriStr);
    auto record = builder.SetUri(uri).Build();
    data.AddRecord(*record);

    PasteDataRecord::Builder builder1(MIMETYPE_TEXT_PLAIN);
    std::string plainText = "plain text";
    auto text = std::make_shared<std::string>(plainText);
    auto record1 = builder1.SetPlainText(text).Build();
    data.AddRecord(*record1);

    auto mimeType = data.GetReportMimeTypes();
    EXPECT_TRUE((strcmp(MIMETYPE_TEXT_PLAIN, mimeType.at(0).c_str()) == 0 &&
                    strcmp(MIMETYPE_TEXT_URI, mimeType.at(1).c_str()) == 0) ||
        (strcmp(MIMETYPE_TEXT_PLAIN, mimeType.at(1).c_str()) == 0 &&
            strcmp(MIMETYPE_TEXT_URI, mimeType.at(0).c_str()) == 0));
}

/**
 * @tc.name: GetDeviceId
 * @tc.desc: Get DeviceId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetDeviceId, TestSize.Level0)
{
    PasteData data;
    data.GetDeviceId();
    ASSERT_TRUE(true);
}

/**
 * @tc.name: GetRecordByIdTest001
 * @tc.desc: GetRecordByIdTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetRecordByIdTest001, TestSize.Level0)
{
    //Arrange
    PasteData pasteData;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    record->SetRecordId(1);
    pasteData.AddRecord(record);

    //Act
    std::shared_ptr<PasteDataRecord> result = pasteData.GetRecordById(1);

    //Assert
    EXPECT_EQ(result, record);
}

/**
 * @tc.name: GetRecordByIdTest002
 * @tc.desc: GetRecordByIdTest002
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetRecordByIdTest002, TestSize.Level0)
{
    //Arrange
    PasteData pasteData;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    record->SetRecordId(1);
    pasteData.AddRecord(record);

    //Act
    std::shared_ptr<PasteDataRecord> result = pasteData.GetRecordById(2);

    //Assert
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: GetRecordByIdTest003
 * @tc.desc: GetRecordByIdTest003
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetRecordByIdTest003, TestSize.Level0)
{
    //Arrange
    PasteData pasteData;

    //Act
    std::shared_ptr<PasteDataRecord> result = pasteData.GetRecordById(1);

    //Assert
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: ReplaceRecordAtTest001
 * @tc.desc: ReplaceRecordAtTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, ReplaceRecordAtTest001, TestSize.Level0)
{
    //Arrange
    PasteData pasteData;

    //Act
    std::shared_ptr<PasteDataRecord> record = nullptr;

    //Assert
    EXPECT_FALSE(pasteData.ReplaceRecordAt(0, record));
}

/**
 * @tc.name: GetFileSizeTest001
 * @tc.desc: GetFileSizeTest001
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, GetFileSizeTest001, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto pasteData = PasteboardClient::GetInstance()->CreateUriData(uri);
    ASSERT_TRUE(pasteData != nullptr);
    PasteDataProperty property = pasteData->GetProperty();
    property.tokenId = 1;

    AAFwk::ILong *ao = nullptr;
    int64_t fileSize = pasteData->GetFileSize();
    EXPECT_EQ(fileSize, property.additions.GetIntParam(REMOTE_FILE_SIZE, -1));

    int64_t fileSize2 = 0L;
    pasteData->SetFileSize(fileSize2);
    int64_t fileSize3 = pasteData->GetFileSize();
    EXPECT_EQ(fileSize3, fileSize2);
}

/**
 * @tc.name: SetDataIdTest001
 * @tc.desc: SetDataId
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataTest, SetDataIdTest001, TestSize.Level0)
{
    PasteData pasteData;
    pasteData.SetDataId(0);
    EXPECT_EQ(0, pasteData.GetDataId());
}


/**
 * @tc.name: GenerateDataType001
 * @tc.desc: Test GenerateDataType with empty PasteData
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType001, TestSize.Level0)
{
    PasteData data;
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 0);
}

/**
 * @tc.name: GenerateDataType002
 * @tc.desc: Test GenerateDataType with plain text record
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType002, TestSize.Level0)
{
    PasteData data;
    data.AddTextRecord("plain text content");
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 1);
}

/**
 * @tc.name: GenerateDataType003
 * @tc.desc: Test GenerateDataType with html record
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType003, TestSize.Level0)
{
    PasteData data;
    data.AddHtmlRecord("<div>html content</div>");
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 2);
}

/**
 * @tc.name: GenerateDataType004
 * @tc.desc: Test GenerateDataType with uri record
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType004, TestSize.Level0)
{
    PasteData data;
    data.AddUriRecord(OHOS::Uri("file://test.txt"));
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 4);
}

/**
 * @tc.name: GenerateDataType005
 * @tc.desc: Test GenerateDataType with want record
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType005, TestSize.Level0)
{
    PasteData data;
    std::shared_ptr<OHOS::AAFwk::Want> want = std::make_shared<OHOS::AAFwk::Want>();
    data.AddWantRecord(want);
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 8);
}

/**
 * @tc.name: GenerateDataType006
 * @tc.desc: Test GenerateDataType with pixel map record
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType006, TestSize.Level0)
{
    PasteData data;
    PasteDataRecord::Builder builder("pixelMap");
    auto record = builder.Build();
    data.AddRecord(record);
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 16);
}

/**
 * @tc.name: GenerateDataType007
 * @tc.desc: Test GenerateDataType with multiple records
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType007, TestSize.Level0)
{
    PasteData data;
    data.AddTextRecord("plain text");
    data.AddHtmlRecord("<div>html</div>");
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 3);
}

/**
 * @tc.name: GenerateDataType008
 * @tc.desc: Test GenerateDataType with webview tag and html
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType008, TestSize.Level0)
{
    PasteData data;
    data.SetTag(PasteData::WEBVIEW_PASTEDATA_TAG);
    data.AddHtmlRecord("<div>webview html</div>");
    data.AddTextRecord("associated text");
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 2);
}

/**
 * @tc.name: GenerateDataType009
 * @tc.desc: Test GenerateDataType with unknown mime type
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType009, TestSize.Level0)
{
    PasteData data;
    PasteDataRecord::Builder builder("unknown/mime-type");
    auto record = builder.Build();
    data.AddRecord(record);
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 0);
}

/**
 * @tc.name: GenerateDataType010
 * @tc.desc: Test GenerateDataType with all supported types
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GenerateDataType010, TestSize.Level0)
{
    PasteData data;
    data.AddTextRecord("plain text");
    data.AddHtmlRecord("<div>html</div>");
    data.AddUriRecord(OHOS::Uri("file://test.txt"));
    std::shared_ptr<OHOS::AAFwk::Want> want = std::make_shared<OHOS::AAFwk::Want>();
    data.AddWantRecord(want);
    PasteDataRecord::Builder builder("pixelMap");
    auto record = builder.Build();
    data.AddRecord(record);
    uint8_t result = data.GenerateDataType();
    EXPECT_TRUE(result == 31);
}

/**
 * @tc.name: GetReportDescriptionTest001
 * @tc.desc: GetReportDescription
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetReportDescriptionTest001, TestSize.Level0)
{
    PasteData pasteData;
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    ASSERT_TRUE(record != nullptr);
    pasteData.AddRecord(record);
    DataDescription description = pasteData.GetReportDescription();
    EXPECT_EQ(description.recordNum, 1);
}

/**
 * @tc.name: GetReportDescriptionTest002
 * @tc.desc: GetReportDescription
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetReportDescriptionTest002, TestSize.Level0)
{
    PasteData pasteData;
    DataDescription description = pasteData.GetReportDescription();
    EXPECT_EQ(description.recordNum, 0);
}

/**
 * @tc.name: GetReportDescriptionTest003
 * @tc.desc: GetReportDescription
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetReportDescriptionTest003, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record1 = nullptr;
    std::shared_ptr<PasteDataRecord> record2 = std::make_shared<PasteDataRecord>();
    std::vector<std::shared_ptr<PasteDataRecord>> records = {record1, record2};
    PasteData pasteData(records);
    DataDescription description = pasteData.GetReportDescription();
    EXPECT_EQ(description.recordNum, 2);
}

/**
 * @tc.name: GetPrimaryHtmlTest001
 * @tc.desc: GetPrimaryHtml
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryHtmlTest001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = nullptr;
    std::vector<std::shared_ptr<PasteDataRecord>> records = {record};
    PasteData pasteData;
    pasteData.AddRecord(record);
    auto primary = pasteData.GetPrimaryHtml();
    EXPECT_EQ(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryHtmlTest002
 * @tc.desc: GetPrimaryHtml
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryHtmlTest002, TestSize.Level0)
{
    std::vector<std::string> testHtmlVec = {
        "<div class='disable'>html001</div>",
        "<div class='disable'>html002</div>",
        "<div class='disable'>html003</div>"
    };
    PasteData pasteData;

    for (const auto &itHtml: testHtmlVec) {
        pasteData.AddHtmlRecord(itHtml);
    }
    auto count = pasteData.GetRecordCount();
    EXPECT_TRUE(count == testHtmlVec.size());

    auto primary = pasteData.GetPrimaryHtml();
    EXPECT_NE(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryPixelMapTest001
 * @tc.desc: GetPrimaryPixelMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryPixelMapTest001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = nullptr;
    std::vector<std::shared_ptr<PasteDataRecord>> records = {record};
    PasteData pasteData;
    pasteData.AddRecord(record);
    auto primary = pasteData.GetPrimaryPixelMap();
    EXPECT_EQ(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryPixelMapTest002
 * @tc.desc: GetPrimaryPixelMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryPixelMapTest002, TestSize.Level0)
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, 100, opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    auto record = PasteboardClient::GetInstance()->CreatePixelMapRecord(pixelMapIn);
    ASSERT_TRUE(record != nullptr);
    PasteData pasteData;

    pasteData.AddRecord(record);

    auto primary = pasteData.GetPrimaryPixelMap();
    EXPECT_NE(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryTextTest001
 * @tc.desc: GetPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryTextTest001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = nullptr;
    std::vector<std::shared_ptr<PasteDataRecord>> records = {record};
    PasteData pasteData;
    pasteData.AddRecord(record);
    auto primary = pasteData.GetPrimaryText();
    EXPECT_EQ(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryTextTest002
 * @tc.desc: GetPrimaryText
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryTextTest002, TestSize.Level0)
{
    std::vector<std::string> testTextVec = { "text101", "text102", "text103" };
    PasteData pasteData;
    
    for (const auto &itText: testTextVec) {
        pasteData.AddTextRecord(itText);
    }

    auto count = pasteData.GetRecordCount();
    EXPECT_TRUE(count == testTextVec.size());

    auto primary = pasteData.GetPrimaryText();
    EXPECT_NE(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryUriTest001
 * @tc.desc: GetPrimaryUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryUriTest001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = nullptr;
    std::vector<std::shared_ptr<PasteDataRecord>> records = {record};
    PasteData pasteData;
    pasteData.AddRecord(record);
    auto primary = pasteData.GetPrimaryUri();
    EXPECT_EQ(primary, nullptr);
}

/**
 * @tc.name: GetPrimaryUriTest002
 * @tc.desc: GetPrimaryUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetPrimaryUriTest002, TestSize.Level0)
{
    std::vector<std::string> testUriVec = {
        "file://pasteboard_service/test_uri_001",
        "file://pasteboard_service/test_uri_002",
        "file://pasteboard_service/test_uri_003"
    };

    PasteData pasteData;
    for (const auto &itUri: testUriVec) {
        pasteData.AddUriRecord(OHOS::Uri(itUri));
    }

    auto count = pasteData.GetRecordCount();
    EXPECT_TRUE(count == testUriVec.size());

    auto primary = pasteData.GetPrimaryUri();
    EXPECT_NE(primary, nullptr);
}

/**
 * @tc.name: IsValidShareOptionTest001
 * @tc.desc: IsValidShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidShareOptionTest001, TestSize.Level0)
{
    int32_t shareOption = -1;
    auto isValid = PasteData::IsValidShareOption(shareOption);
    EXPECT_FALSE(isValid);
}

/**
 * @tc.name: IsValidShareOptionTest002
 * @tc.desc: IsValidShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidShareOptionTest002, TestSize.Level0)
{
    int32_t shareOption = 0;
    auto isValid = PasteData::IsValidShareOption(shareOption);
    EXPECT_TRUE(isValid);
}

/**
 * @tc.name: IsValidShareOptionTest003
 * @tc.desc: IsValidShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidShareOptionTest003, TestSize.Level0)
{
    int32_t shareOption = 1;
    auto isValid = PasteData::IsValidShareOption(shareOption);
    EXPECT_TRUE(isValid);
}

/**
 * @tc.name: IsValidShareOptionTest004
 * @tc.desc: IsValidShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidShareOptionTest004, TestSize.Level0)
{
    int32_t shareOption = 2;
    auto isValid = PasteData::IsValidShareOption(shareOption);
    EXPECT_TRUE(isValid);
}

/**
 * @tc.name: IsValidShareOptionTest005
 * @tc.desc: IsValidShareOption
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidShareOptionTest005, TestSize.Level0)
{
    int32_t shareOption = 3;
    auto isValid = PasteData::IsValidShareOption(shareOption);
    EXPECT_FALSE(isValid);
}

/**
 * @tc.name: IsValidPasteIdTest001
 * @tc.desc: IsValidPasteId
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidPasteIdTest001, TestSize.Level0)
{
    std::string pasteId = "";
    auto isValid = PasteData::IsValidPasteId(pasteId);
    EXPECT_FALSE(isValid);
}

/**
 * @tc.name: IsValidPasteIdTest002
 * @tc.desc: IsValidPasteId
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidPasteIdTest002, TestSize.Level0)
{
    std::string pasteId = "test";
    auto isValid = PasteData::IsValidPasteId(pasteId);
    EXPECT_FALSE(isValid);
}

/**
 * @tc.name: IsValidPasteIdTest003
 * @tc.desc: IsValidPasteId
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidPasteIdTest003, TestSize.Level0)
{
    std::string pasteId = "test_test_test";
    auto isValid = PasteData::IsValidPasteId(pasteId);
    EXPECT_FALSE(isValid);
}

/**
 * @tc.name: IsValidPasteIdTest004
 * @tc.desc: IsValidPasteId
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, IsValidPasteIdTest004, TestSize.Level0)
{
    pid_t pid = getpid();
    std::string currentPid = std::to_string(pid);
    std::string pasteId = "test_" + currentPid + "_001";
    auto isValid = PasteData::IsValidPasteId(pasteId);
    EXPECT_TRUE(isValid);
}

/**
 * @tc.name: RemoveEmptyEntryTest001
 * @tc.desc: should remove empty entry if empty entry exist
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, RemoveEmptyEntryTest001, TestSize.Level0)
{
    PasteData data;
    auto record = std::make_shared<PasteDataRecord>();
    auto entryText = std::make_shared<PasteDataEntry>("general.plain-text", "plain text");
    auto entryPixelMap = std::make_shared<PasteDataEntry>();
    entryPixelMap->SetUtdId("openharmony.pixel-map");
    record->SetDelayRecordFlag(true);
    record->AddEntry("general.plain-text", entryText);
    record->AddEntry("openharmony.pixel-map", entryPixelMap);
    data.AddRecord(record);
    EXPECT_TRUE(record->HasEmptyEntry());
    auto mimeTypes = data.GetMimeTypes();
    EXPECT_EQ(mimeTypes.size(), 2);

    data.RemoveEmptyEntry();
    EXPECT_FALSE(record->HasEmptyEntry());

    mimeTypes = data.GetMimeTypes();
    ASSERT_EQ(mimeTypes.size(), 1);
    EXPECT_STREQ(mimeTypes[0].c_str(), MIMETYPE_TEXT_PLAIN);
}

/**
 * @tc.name: GetOriginTokenIdTest001
 * @tc.desc: GetOriginTokenId
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetOriginTokenIdTest001, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto pasteData = PasteboardClient::GetInstance()->CreateUriData(uri);
    auto originTokenId = pasteData->GetOriginTokenId();
    EXPECT_EQ(originTokenId, PasteData::INVALID_TOKEN_ID);
}

/**
 * @tc.name: GetOriginTokenIdTest002
 * @tc.desc: GetOriginTokenId
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, GetOriginTokenIdTest002, TestSize.Level0)
{
    OHOS::Uri uri("uri");
    auto pasteData = PasteboardClient::GetInstance()->CreateUriData(uri);
    int32_t tokenId = 123456;
    AAFwk::WantParams originInfo;
    originInfo.SetParam("originTokenId", AAFwk::Integer::Box(tokenId));
    pasteData->SetAddition("originInfo", AAFwk::WantParamWrapper::Box(originInfo));
    auto originTokenId = pasteData->GetOriginTokenId();
    EXPECT_EQ(originTokenId, tokenId);
}

/**
 * @tc.name: SetTextSize001
 * @tc.desc: Test SetTextSize and GetTextSize functions
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, SetTextSize001, TestSize.Level0)
{
    PasteData data;
    size_t testSize = 1024;
    data.SetTextSize(testSize);
    EXPECT_EQ(data.GetTextSize(), testSize);
}

/**
 * @tc.name: SetTextSize002
 * @tc.desc: Test SetTextSize and GetTextSize functions with zero size
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, SetTextSize002, TestSize.Level0)
{
    PasteData data;
    size_t testSize = 0;
    data.SetTextSize(testSize);
    EXPECT_EQ(data.GetTextSize(), testSize);
}

/**
 * @tc.name: SetTextSize003
 * @tc.desc: Test SetTextSize and GetTextSize functions with large size
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataTest, SetTextSize003, TestSize.Level0)
{
    PasteData data;
    size_t testSize = SIZE_MAX;
    data.SetTextSize(testSize);
    EXPECT_EQ(data.GetTextSize(), testSize);
}

} // namespace OHOS::MiscServices
