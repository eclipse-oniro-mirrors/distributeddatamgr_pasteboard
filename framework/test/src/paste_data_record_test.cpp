/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "entry_getter.h"
#include "paste_data_record.h"
#include "unified_meta.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
class PasteDataRecordTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    Details details_;
    std::vector<uint8_t> rawData_;
    std::string text_ = "test";
    std::string extraText_ = "extr";
    std::string uri_ = "file://123.txt";
    std::string html_ = "<div class='disable'>helloWorld</div>";
    std::string link_ = "http://abc.com";
    int32_t width_ = 5;
    int32_t height_ = 7;

    void CheckEntries(const std::vector<std::shared_ptr<PasteDataEntry>> &entries);
    void CheckPlainUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckFileUriUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckPixelMapUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckHtmlUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckLinkUds(const std::shared_ptr<PasteDataEntry> entry);

    void AddPlainUdsEntry(PasteDataRecord &record);
    void AddFileUriUdsEntry(PasteDataRecord &record);
    void AddHtmlUdsEntry(PasteDataRecord &record);
    void AddPixelMapUdsEntry(PasteDataRecord &record);
    void AddLinkUdsEntry(PasteDataRecord &record);
};

void PasteDataRecordTest::SetUpTestCase(void) { }

void PasteDataRecordTest::TearDownTestCase(void) { }

void PasteDataRecordTest::SetUp(void)
{
    rawData_ = { 1, 2, 3, 4, 5, 6, 7, 8 };
    details_.insert({ "keyString", "string_test" });
    details_.insert({ "keyInt32", 1 });
    details_.insert({ "keyBool", true });
    details_.insert({ "KeyU8Array", rawData_ });
    details_.insert({ "KeyDouble", 1.234 });
}

void PasteDataRecordTest::TearDown(void) { }

void PasteDataRecordTest::AddPlainUdsEntry(PasteDataRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::CONTENT] = text_;
    record.AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
}

void PasteDataRecordTest::AddFileUriUdsEntry(PasteDataRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::FILE_URI_PARAM] = uri_;
    object->value_[UDMF::FILE_TYPE] = "";
    record.AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
}

void PasteDataRecordTest::AddHtmlUdsEntry(PasteDataRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::HTML_CONTENT] = html_;
    record.AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
}

void PasteDataRecordTest::AddLinkUdsEntry(PasteDataRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    object->value_[UDMF::URL] = link_;
    record.AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
}

void PasteDataRecordTest::AddPixelMapUdsEntry(PasteDataRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {width_, height_},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    object->value_[UDMF::PIXEL_MAP] = pixelMapIn;
    record.AddEntry(utdId, std::make_shared<PasteDataEntry>(utdId, object));
}

void PasteDataRecordTest::CheckEntries(const std::vector<std::shared_ptr<PasteDataEntry>> &entries)
{
    for (auto const &entry : entries) {
        if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT)) {
            CheckPlainUds(entry);
        }
        if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI)) {
            CheckFileUriUds(entry);
        }
        if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP)) {
            CheckPixelMapUds(entry);
        }
        if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK)) {
            CheckLinkUds(entry);
        }
        if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML)) {
            CheckHtmlUds(entry);
        }
    }
}

void PasteDataRecordTest::CheckPlainUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_EQ(MIMETYPE_TEXT_PLAIN, entry->GetMimeType());
    auto decodeValue = entry->GetValue();
    auto object = std::get_if<std::shared_ptr<Object>>(&decodeValue);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    auto typeValue = std::get_if<std::string>(&objectValue[UDMF::UNIFORM_DATA_TYPE]);
    ASSERT_NE(typeValue, nullptr);
    ASSERT_EQ(*typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT));
    auto value = std::get_if<std::string>(&objectValue[UDMF::CONTENT]);
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(*value, text_);
}

void PasteDataRecordTest::CheckFileUriUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_EQ(MIMETYPE_TEXT_URI, entry->GetMimeType());
    auto decodeValue = entry->GetValue();
    auto object = std::get_if<std::shared_ptr<Object>>(&decodeValue);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    auto typeValue = std::get_if<std::string>(&objectValue[UDMF::UNIFORM_DATA_TYPE]);
    ASSERT_NE(typeValue, nullptr);
    ASSERT_EQ(*typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI));
    auto value = std::get_if<std::string>(&objectValue[UDMF::FILE_URI_PARAM]);
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(*value, uri_);
}

void PasteDataRecordTest::CheckHtmlUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_EQ(MIMETYPE_TEXT_HTML, entry->GetMimeType());
    auto decodeValue = entry->GetValue();
    auto object = std::get_if<std::shared_ptr<Object>>(&decodeValue);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    auto typeValue = std::get_if<std::string>(&objectValue[UDMF::UNIFORM_DATA_TYPE]);
    ASSERT_NE(typeValue, nullptr);
    ASSERT_EQ(*typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML));
    auto value = std::get_if<std::string>(&objectValue[UDMF::HTML_CONTENT]);
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(*value, html_);
}

void PasteDataRecordTest::CheckPixelMapUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_EQ(MIMETYPE_PIXELMAP, entry->GetMimeType());
    auto decodeValue = entry->GetValue();
    auto object = std::get_if<std::shared_ptr<Object>>(&decodeValue);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    auto typeValue = std::get_if<std::string>(&objectValue[UDMF::UNIFORM_DATA_TYPE]);
    ASSERT_NE(typeValue, nullptr);
    ASSERT_EQ(*typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP));
    auto value = std::get_if<std::shared_ptr<PixelMap>>(&objectValue[UDMF::PIXEL_MAP]);
    ASSERT_NE(value, nullptr);
    ImageInfo imageInfo = {};
    (*value)->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == height_);
    ASSERT_TRUE(imageInfo.size.width == width_);
    ASSERT_TRUE(imageInfo.pixelFormat == PixelFormat::ARGB_8888);
}

void PasteDataRecordTest::CheckLinkUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_EQ(MIMETYPE_TEXT_PLAIN, entry->GetMimeType());
    auto decodeValue = entry->GetValue();
    auto object = std::get_if<std::shared_ptr<Object>>(&decodeValue);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    auto typeValue = std::get_if<std::string>(&objectValue[UDMF::UNIFORM_DATA_TYPE]);
    ASSERT_NE(typeValue, nullptr);
    ASSERT_EQ(*typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK));
    auto value = std::get_if<std::string>(&objectValue[UDMF::URL]);
    ASSERT_NE(value, nullptr);
    ASSERT_EQ(*value, link_);
}

/**
 * @tc.name: GetValidTypesTest001
 * @tc.desc: GetValidTypesTest001;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataRecordTest, GetValidTypesTest001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    AddPlainUdsEntry(*record);
    AddHtmlUdsEntry(*record);

    std::vector<std::string> inputTypes;
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI));
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML));

    auto validTypes = record->GetValidTypes(inputTypes);
    ASSERT_EQ(validTypes.size(), 1);
    ASSERT_EQ(validTypes[0], UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML));
}

/**
 * @tc.name: AddEntryTest001
 * @tc.desc: Add entry test
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataRecordTest, AddEntryTest001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    AddPlainUdsEntry(*record);
    AddHtmlUdsEntry(*record);
    std::set<std::string> mimeTypes = record->GetMimeTypes();
    EXPECT_NE(mimeTypes.find(MIMETYPE_TEXT_PLAIN), mimeTypes.end());
    EXPECT_NE(mimeTypes.find(MIMETYPE_TEXT_HTML), mimeTypes.end());
}

/**
 * @tc.name: GetEntries001
 * @tc.desc: convert to plainText;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataRecordTest, GetEntries001, TestSize.Level0)
{
    std::vector<std::string> inputTypes;
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI));
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML));
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI));
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK));
    inputTypes.emplace_back(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP));

    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    AddPlainUdsEntry(*record);
    auto types = record->GetValidTypes(inputTypes);
    ASSERT_EQ(types.size(), 0);

    AddHtmlUdsEntry(*record);
    types = record->GetValidTypes(inputTypes);
    auto it = std::find(types.begin(), types.end(), UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML));
    ASSERT_NE(it, types.end());

    AddFileUriUdsEntry(*record);
    types = record->GetValidTypes(inputTypes);
    it = std::find(types.begin(), types.end(), UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI));
    ASSERT_NE(it, types.end());

    AddLinkUdsEntry(*record);
    types = record->GetValidTypes(inputTypes);
    it = std::find(types.begin(), types.end(), UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK));
    ASSERT_NE(it, types.end());

    AddPixelMapUdsEntry(*record);
    types = record->GetValidTypes(inputTypes);
    it = std::find(types.begin(), types.end(), UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP));
    ASSERT_NE(it, types.end());

    auto entries = record->GetEntries();
    CheckEntries(entries);
}

/**
 * @tc.name: BuilderTest001
 * @tc.desc: PasteDataRecord::Builder
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, BuilderTest001, TestSize.Level0)
{
    PasteDataRecord::Builder builder("");
    builder.SetHtmlText(nullptr);
    builder.SetWant(nullptr);
    builder.SetPlainText(nullptr);
    builder.SetUri(nullptr);
    builder.SetPixelMap(nullptr);
    builder.SetCustomData(nullptr);
    builder.SetMimeType("");

    auto record = builder.Build();
    EXPECT_NE(record, nullptr);
}

/**
 * @tc.name: SetUriTest001
 * @tc.desc: SetUri & GetUriV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, SetUriTest001, TestSize.Level0)
{
    PasteDataRecord record;
    record.SetUri(nullptr);

    auto uri = record.GetUriV0();
    EXPECT_EQ(uri, nullptr);
}

/**
 * @tc.name: NewMultiTypeRecordTest001
 * @tc.desc: NewMultiTypeRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, NewMultiTypeRecordTest001, TestSize.Level0)
{
    PasteDataRecord record;

    auto ret = record.NewMultiTypeRecord(nullptr, "");
    EXPECT_NE(ret, nullptr);

    ret = record.NewMultiTypeRecord(nullptr, "0");
    EXPECT_NE(ret, nullptr);

    auto values = std::make_shared<std::map<std::string, std::shared_ptr<EntryValue>>>();
    ASSERT_NE(values, nullptr);

    ret = record.NewMultiTypeRecord(nullptr, "0");
    EXPECT_NE(ret, nullptr);

    (*values)["0"] = nullptr;
    ret = record.NewMultiTypeRecord(nullptr, "0");
    EXPECT_NE(ret, nullptr);

    (*values)["0"] = std::make_shared<EntryValue>();
    (*values)["1"] = std::make_shared<EntryValue>();
    ret = record.NewMultiTypeRecord(nullptr, "0");
    EXPECT_NE(ret, nullptr);

    ret = record.NewMultiTypeRecord(nullptr, "");
    EXPECT_NE(ret, nullptr);

    ret = record.NewMultiTypeRecord(nullptr, "2");
    EXPECT_NE(ret, nullptr);
}

class EntryGetterImplement : public UDMF::EntryGetter {
public:
    UDMF::ValueType GetValueByType(const std::string &utdId) override
    {
        return nullptr;
    }
};

/**
 * @tc.name: GetEntryGetterTest001
 * @tc.desc: SetEntryGetter & GetEntryGetter
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryGetterTest001, TestSize.Level0)
{
    PasteDataRecord record;
    std::shared_ptr<UDMF::EntryGetter> entryGetter = nullptr;
    record.SetEntryGetter(entryGetter);
    auto ret = record.GetEntryGetter();
    EXPECT_EQ(ret, nullptr);

    entryGetter = std::make_shared<EntryGetterImplement>();
    record.SetEntryGetter(entryGetter);
    ret = record.GetEntryGetter();
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: NewMultiTypeDelayRecordTest001
 * @tc.desc: NewMultiTypeDelayRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, NewMultiTypeDelayRecordTest001, TestSize.Level0)
{
    PasteDataRecord record;
    std::vector<std::string> mimeTypes = { "0", "1", "2" };
    auto ret = record.NewMultiTypeDelayRecord(mimeTypes, nullptr);
    EXPECT_NE(ret, nullptr);

    auto entryGetter = std::make_shared<EntryGetterImplement>();
    ret = record.NewMultiTypeDelayRecord(mimeTypes, entryGetter);
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: ClearPixelMapTest001
 * @tc.desc: ClearPixelMap & GetPixelMapV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, ClearPixelMapTest001, TestSize.Level0)
{
    auto pixelMap = std::make_shared<OHOS::Media::PixelMap>();
    auto record = PasteDataRecord::NewPixelMapRecord(pixelMap);
    ASSERT_NE(record, nullptr);

    pixelMap = record->GetPixelMapV0();
    EXPECT_NE(pixelMap, nullptr);

    record->ClearPixelMap();
    pixelMap = record->GetPixelMapV0();
    EXPECT_EQ(pixelMap, nullptr);
}

/**
 * @tc.name: GetHtmlTextV0Test001
 * @tc.desc: GetHtmlTextV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetHtmlTextV0Test001, TestSize.Level0)
{
    std::string html = html_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    ASSERT_NE(record, nullptr);

    auto htmlText = record->GetHtmlTextV0();
    EXPECT_NE(htmlText, nullptr);
}

/**
 * @tc.name: GetHtmlTextV0Test002
 * @tc.desc: GetHtmlTextV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetHtmlTextV0Test002, TestSize.Level0)
{
    std::string text = text_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPlainTextRecord(text);
    ASSERT_NE(record, nullptr);

    auto htmlText = record->GetHtmlTextV0();
    EXPECT_EQ(htmlText, nullptr);
}

/**
 * @tc.name: GetHtmlTextV0Test003
 * @tc.desc: GetHtmlTextV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetHtmlTextV0Test003, TestSize.Level0)
{
    std::string html = nullptr;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    ASSERT_NE(record, nullptr);

    auto htmlText = record->GetHtmlTextV0();
    EXPECT_EQ(htmlText, nullptr);
}

/**
 * @tc.name: GetHtmlTextTest001
 * @tc.desc: GetHtmlText
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetHtmlTextTest001, TestSize.Level0)
{
    PasteDataRecord record;

    auto htmlText = record.GetHtmlText();
    EXPECT_EQ(htmlText, nullptr);
}

/**
 * @tc.name: GetHtmlTextTest002
 * @tc.desc: GetHtmlText
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetHtmlTextTest002, TestSize.Level0)
{
    std::string html = html_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    ASSERT_NE(record, nullptr);

    auto htmlText = record->GetHtmlText();
    EXPECT_NE(htmlText, nullptr);
}

/**
 * @tc.name: GetPlainTextV0Test001
 * @tc.desc: GetPlainTextV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPlainTextV0Test001, TestSize.Level0)
{
    std::string text = text_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPlainTextRecord(text);
    ASSERT_NE(record, nullptr);

    auto plainText = record->GetPlainTextV0();
    EXPECT_NE(plainText, nullptr);
}

/**
 * @tc.name: GetPlainTextV0Test002
 * @tc.desc: GetPlainTextV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPlainTextV0Test002, TestSize.Level0)
{
    std::string html = html_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    ASSERT_NE(record, nullptr);

    auto plainText = record->GetPlainTextV0();
    EXPECT_EQ(plainText, nullptr);
}

/**
 * @tc.name: GetPlainTextV0Test003
 * @tc.desc: GetPlainTextV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPlainTextV0Test003, TestSize.Level0)
{
    std::string text = nullptr;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPlainTextRecord(text);
    ASSERT_NE(record, nullptr);

    auto plainText = record->GetPlainTextV0();
    EXPECT_EQ(plainText, nullptr);
}

/**
 * @tc.name: GetPlainTextTest001
 * @tc.desc: GetPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPlainTextTest001, TestSize.Level0)
{
    PasteDataRecord record;

    auto plainText = record.GetPlainText();
    EXPECT_EQ(plainText, nullptr);
}

/**
 * @tc.name: GetPlainTextTest002
 * @tc.desc: GetPlainText
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPlainTextTest002, TestSize.Level0)
{
    std::string text = text_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPlainTextRecord(text);
    ASSERT_NE(record, nullptr);

    auto plainText = record->GetPlainText();
    EXPECT_NE(plainText, nullptr);
}

/**
 * @tc.name: GetPixelMapTest001
 * @tc.desc: GetPixelMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPixelMapTest001, TestSize.Level0)
{
    PasteDataRecord record;

    auto pixelMap = record.GetPixelMap();
    EXPECT_EQ(pixelMap, nullptr);
}

/**
 * @tc.name: GetPixelMapTest002
 * @tc.desc: GetPixelMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetPixelMapTest002, TestSize.Level0)
{
    const uint32_t color[] = { 0x80, 0x02, 0x04, 0x08, 0x40, 0x02, 0x04, 0x08 };
    uint32_t len = sizeof(color) / sizeof(color[0]);
    Media::InitializationOptions opts;
    opts.size.width = 2;
    opts.size.height = 3;
    opts.pixelFormat = Media::PixelFormat::UNKNOWN;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    std::shared_ptr<Media::PixelMap> pixelMap = Media::PixelMap::Create(color, len, 0, opts.size.width, opts);

    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPixelMapRecord(pixelMap);
    ASSERT_NE(record, nullptr);

    pixelMap = record->GetPixelMap();
    EXPECT_NE(pixelMap, nullptr);
}

/**
 * @tc.name: GetUriV0Test001
 * @tc.desc: GetUriV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUriV0Test001, TestSize.Level0)
{
    OHOS::Uri uri(uri_);
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewUriRecord(uri);
    ASSERT_NE(record, nullptr);

    auto tempUri = record->GetUriV0();
    EXPECT_NE(tempUri, nullptr);
}

/**
 * @tc.name: GetUriV0Test002
 * @tc.desc: GetUriV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUriV0Test002, TestSize.Level0)
{
    std::string html = html_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    ASSERT_NE(record, nullptr);

    auto tempUri = record->GetUriV0();
    EXPECT_EQ(tempUri, nullptr);
}

/**
 * @tc.name: GetUriV0Test003
 * @tc.desc: GetUriV0
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUriV0Test003, TestSize.Level0)
{
    PasteDataRecord record;
    record.SetUri(nullptr);

    auto tempUri = record.GetUriV0();
    EXPECT_EQ(tempUri, nullptr);
}

/**
 * @tc.name: GetUriTest001
 * @tc.desc: GetUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUriTest001, TestSize.Level0)
{
    OHOS::Uri uri(uri_);
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewUriRecord(uri);
    ASSERT_NE(record, nullptr);

    auto tempUri = record->GetUri();
    EXPECT_NE(tempUri, nullptr);
}

/**
 * @tc.name: GetUriTest002
 * @tc.desc: GetUri
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUriTest002, TestSize.Level0)
{
    PasteDataRecord record;
    record.SetUri(nullptr);

    auto tempUri = record.GetUri();
    EXPECT_EQ(tempUri, nullptr);
}

/**
 * @tc.name: ConstructorTest001
 * @tc.desc: PasteDataRecord::PasteDataRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, ConstructorTest001, TestSize.Level0)
{
    PasteDataRecord record("", nullptr, nullptr, nullptr, nullptr);
    auto pixelMap = record.GetPixelMapV0();
    EXPECT_EQ(pixelMap, nullptr);
}

/**
 * @tc.name: AddEntryTest002
 * @tc.desc: AddEntry & GetEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryTest002, TestSize.Level0)
{
    PasteDataRecord record;
    record.AddEntry("1", nullptr);

    auto plainTextUtdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = plainTextUtdId;
    object->value_[UDMF::CONTENT] = text_;
    auto fileUriUtdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    auto entry = std::make_shared<PasteDataEntry>(plainTextUtdId, object);
    record.AddEntry(fileUriUtdId, entry);

    auto ret = record.GetEntry(plainTextUtdId);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: AddEntryTest003
 * @tc.desc: AddEntry & GetEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryTest003, TestSize.Level0)
{
    PasteDataRecord record(MIMETYPE_TEXT_WANT, nullptr, nullptr, nullptr, nullptr);
    auto wantUtdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::OPENHARMONY_WANT);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = wantUtdId;
    auto entry = std::make_shared<PasteDataEntry>(wantUtdId, object);
    record.AddEntry(wantUtdId, entry);

    auto ret = record.GetEntry(wantUtdId);
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: AddEntryTest004
 * @tc.desc: AddEntry & GetEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryTest004, TestSize.Level0)
{
    PasteDataRecord record(MIMETYPE_TEXT_WANT, nullptr, nullptr, nullptr, nullptr);
    AddLinkUdsEntry(record);
    auto ret = record.GetEntry(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK));
    EXPECT_NE(ret, nullptr);

    ret = record.GetEntry(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE));
    EXPECT_EQ(ret, nullptr);

    AddFileUriUdsEntry(record);
    ret = record.GetEntry(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE));
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: SetTextContentTest001
 * @tc.desc: SetTextContent & GetTextContent
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, SetTextContentTest001, TestSize.Level0)
{
    PasteDataRecord record;
    std::string ret = record.GetTextContent();
    EXPECT_STREQ(ret.c_str(), "");

    record.SetTextContent(text_);
    ret = record.GetTextContent();
    EXPECT_STREQ(ret.c_str(), text_.c_str());
}

/**
 * @tc.name: GetValidMimeTypesTest001
 * @tc.desc: GetValidMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetValidMimeTypesTest001, TestSize.Level0)
{
    PasteDataRecord record(MIMETYPE_TEXT_WANT, nullptr, nullptr, nullptr, nullptr);
    std::vector<std::string> mimeTypes = { MIMETYPE_TEXT_WANT, MIMETYPE_TEXT_URI };
    std::vector<std::string> validTypes = record.GetValidMimeTypes(mimeTypes);
    EXPECT_NE(validTypes.size(), 0);
}

/**
 * @tc.name: AddEntryByMimeTypeTest001
 * @tc.desc: AddEntryByMimeType & GetEntryByMimeType
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryByMimeTypeTest001, TestSize.Level0)
{
    PasteDataRecord record;
    std::string mimeType = "";
    record.AddEntryByMimeType(mimeType, nullptr);

    auto ret = record.GetEntryByMimeType(mimeType);
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name: AddEntryByMimeTypeTest002
 * @tc.desc: AddEntryByMimeType & GetEntryByMimeType
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryByMimeTypeTest002, TestSize.Level0)
{
    PasteDataRecord record;
    auto entry = std::make_shared<PasteDataEntry>();
    record.AddEntryByMimeType(MIMETYPE_TEXT_PLAIN, entry);

    auto ret = record.GetEntryByMimeType(MIMETYPE_TEXT_PLAIN);
    EXPECT_NE(ret, nullptr);
}

/**
 * @tc.name: AddEntryByMimeTypeTest003
 * @tc.desc: AddEntryByMimeType & GetMimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryByMimeTypeTest003, TestSize.Level0)
{
    PasteDataRecord record(MIMETYPE_TEXT_PLAIN, nullptr, nullptr, nullptr, nullptr);
    AddPlainUdsEntry(record);

    std::set<std::string> mimeTypes = record.GetMimeTypes();
    bool isExist = mimeTypes.find(MIMETYPE_TEXT_PLAIN) == mimeTypes.end();
    EXPECT_FALSE(isExist);
}

/**
 * @tc.name: AddEntryByMimeTypeTest004
 * @tc.desc: AddEntryByMimeType & GetEntryByMimeType
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, AddEntryByMimeTypeTest004, TestSize.Level0)
{
    PasteDataRecord record(MIMETYPE_TEXT_PLAIN, nullptr, nullptr, nullptr, nullptr);
    auto plainTextUtdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    auto object = std::make_shared<Object>();
    object->value_[UDMF::UNIFORM_DATA_TYPE] = plainTextUtdId;
    object->value_[UDMF::CONTENT] = text_;
    auto entry = std::make_shared<PasteDataEntry>(plainTextUtdId, object);

    record.AddEntryByMimeType(MIMETYPE_TEXT_PLAIN, entry);
    auto ret = record.GetEntryByMimeType(MIMETYPE_TEXT_PLAIN);
    EXPECT_NE(ret, nullptr);

    AddHtmlUdsEntry(record);
    std::set<std::string> mimeTypes = record.GetMimeTypes();
    bool isExist = mimeTypes.find(MIMETYPE_TEXT_PLAIN) == mimeTypes.end();
    EXPECT_FALSE(isExist);
}

/**
 * @tc.name: HasEmptyEntryTest001
 * @tc.desc: HasEmptyEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, HasEmptyEntryTest001, TestSize.Level0)
{
    PasteDataRecord record;
    auto entries = record.GetEntries();
    EXPECT_EQ(entries.size(), 0);

    bool ret = record.HasEmptyEntry();
    EXPECT_FALSE(ret);
}

/**
 * @tc.name: HasEmptyEntryTest002
 * @tc.desc: HasEmptyEntry
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, HasEmptyEntryTest002, TestSize.Level0)
{
    PasteDataRecord record;
    auto entry = std::make_shared<PasteDataEntry>();
    std::string utdId = "general.hyperlink";
    entry->SetUtdId(utdId);
    record.AddEntry(utdId, entry);
    auto entries = record.GetEntries();
    EXPECT_EQ(entries.size(), 1);

    bool ret = record.HasEmptyEntry();
    EXPECT_TRUE(ret);
}

/**
 * @tc.name: GetUDMFValueTest001
 * @tc.desc: SetUDMFValue & GetUDMFValue
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUDMFValueTest001, TestSize.Level0)
{
    PasteDataRecord record1("1", nullptr, nullptr, nullptr, nullptr);
    auto entries = record1.GetEntries();
    EXPECT_EQ(entries.size(), 0);

    auto customData = std::make_shared<MineCustomData>();
    PasteDataRecord::Builder builder("1");
    builder.SetCustomData(customData);
    auto record2 = builder.Build();
    ASSERT_NE(record2, nullptr);
    entries = record2->GetEntries();
    EXPECT_EQ(entries.size(), 0);
}

/**
 * @tc.name: GetUDMFValueTest002
 * @tc.desc: GetUDMFValue
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetUDMFValueTest002, TestSize.Level0)
{
    PasteDataRecord record1(MIMETYPE_PIXELMAP, nullptr, nullptr, nullptr, nullptr);
    auto entries = record1.GetEntries();
    EXPECT_EQ(entries.size(), 0);

    const uint32_t color[] = { 0x80, 0x02, 0x04, 0x08, 0x40, 0x02, 0x04, 0x08 };
    uint32_t len = sizeof(color) / sizeof(color[0]);
    Media::InitializationOptions opts;
    opts.size.width = 2;
    opts.size.height = 3;
    opts.pixelFormat = Media::PixelFormat::UNKNOWN;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    std::shared_ptr<OHOS::Media::PixelMap> pixelMap = Media::PixelMap::Create(color, len, 0, opts.size.width, opts);

    PasteDataRecord::Builder builder(MIMETYPE_PIXELMAP);
    builder.SetPixelMap(pixelMap);
    auto record2 = builder.Build();
    ASSERT_NE(record2, nullptr);
    entries = record2->GetEntries();
    EXPECT_EQ(entries.size(), 1);
}

/**
 * @tc.name: IsDelayRecordTest
 * @tc.desc: IsDelayRecord
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, IsDelayRecordTest, TestSize.Level0)
{
    PasteDataRecord record(MIMETYPE_TEXT_WANT, nullptr, nullptr, nullptr, nullptr);
    record.SetDelayRecordFlag(true);
    EXPECT_TRUE(record.IsDelayRecord());
    record.SetDelayRecordFlag(false);
    EXPECT_FALSE(record.IsDelayRecord());
}

/**
 * @tc.name: GetEntryByMimeType001
 * @tc.desc: GetEntryByMimeType(MIMETYPE_TEXT_WANT)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType001, TestSize.Level0)
{
    std::shared_ptr<AAFwk::Want> want = std::make_shared<AAFwk::Want>();
    want->SetUri(uri_);
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewWantRecord(want);
    ASSERT_NE(record, nullptr);
    std::shared_ptr<PasteDataEntry> entry = record->GetEntryByMimeType(MIMETYPE_TEXT_WANT);
    ASSERT_NE(entry, nullptr);
    std::shared_ptr<AAFwk::Want> converted = entry->ConvertToWant();
    ASSERT_NE(converted, nullptr);
    EXPECT_EQ(converted->ToString(), want->ToString());
}

/**
 * @tc.name: GetEntryByMimeType002
 * @tc.desc: GetEntryByMimeType(MIMETYPE_TEXT_PLAIN)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType002, TestSize.Level0)
{
    std::string text = text_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPlainTextRecord(text);
    ASSERT_NE(record, nullptr);
    std::shared_ptr<PasteDataEntry> entry = record->GetEntryByMimeType(MIMETYPE_TEXT_PLAIN);
    ASSERT_NE(entry, nullptr);
    std::shared_ptr<std::string> converted = entry->ConvertToPlainText();
    ASSERT_NE(converted, nullptr);
    EXPECT_EQ(*converted, text);
}

/**
 * @tc.name: GetEntryByMimeType003
 * @tc.desc: GetEntryByMimeType(MIMETYPE_TEXT_HTML)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType003, TestSize.Level0)
{
    std::string html = html_;
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewHtmlRecord(html);
    ASSERT_NE(record, nullptr);
    std::shared_ptr<PasteDataEntry> entry = record->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    ASSERT_NE(entry, nullptr);
    std::shared_ptr<std::string> converted = entry->ConvertToHtml();
    ASSERT_NE(converted, nullptr);
    EXPECT_EQ(*converted, html);
}

/**
 * @tc.name: GetEntryByMimeType004
 * @tc.desc: GetEntryByMimeType(MIMETYPE_TEXT_URI)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType004, TestSize.Level0)
{
    OHOS::Uri uri(uri_);
    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewUriRecord(uri);
    ASSERT_NE(record, nullptr);
    std::shared_ptr<PasteDataEntry> entry = record->GetEntryByMimeType(MIMETYPE_TEXT_URI);
    ASSERT_NE(entry, nullptr);
    std::shared_ptr<OHOS::Uri> converted = entry->ConvertToUri();
    ASSERT_NE(converted, nullptr);
    EXPECT_EQ(converted->ToString(), uri.ToString());
}

/**
 * @tc.name: GetEntryByMimeType005
 * @tc.desc: GetEntryByMimeType(MIMETYPE_PIXELMAP)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType005, TestSize.Level0)
{
    const uint32_t color[] = { 0x80, 0x02, 0x04, 0x08, 0x40, 0x02, 0x04, 0x08 };
    uint32_t len = sizeof(color) / sizeof(color[0]);
    Media::InitializationOptions opts;
    opts.size.width = 2;
    opts.size.height = 3;
    opts.pixelFormat = Media::PixelFormat::UNKNOWN;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    std::shared_ptr<Media::PixelMap> pixelMap = Media::PixelMap::Create(color, len, 0, opts.size.width, opts);

    std::shared_ptr<PasteDataRecord> record = PasteDataRecord::NewPixelMapRecord(pixelMap);
    ASSERT_NE(record, nullptr);
    std::shared_ptr<PasteDataEntry> entry = record->GetEntryByMimeType(MIMETYPE_PIXELMAP);
    ASSERT_NE(entry, nullptr);
    std::shared_ptr<Media::PixelMap> converted = entry->ConvertToPixelMap();
    ASSERT_NE(converted, nullptr);
    EXPECT_EQ(converted->GetWidth(), pixelMap->GetWidth());
    EXPECT_EQ(converted->GetHeight(), pixelMap->GetHeight());
}

/**
 * @tc.name: GetEntryByMimeType006
 * @tc.desc: GetEntryByMimeType(custom)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType006, TestSize.Level0)
{
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03};
    auto customData = std::make_shared<MineCustomData>();
    customData->AddItemData(key, val);

    std::string htmlStr = "<p>hello</p>";
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_HTML);
    builder.SetHtmlText(std::make_shared<std::string>(htmlStr));
    builder.SetCustomData(customData);

    auto record = builder.Build();
    ASSERT_NE(record, nullptr);

    auto entry = record->GetEntryByMimeType(MIMETYPE_TEXT_HTML);
    ASSERT_NE(entry, nullptr);
    std::shared_ptr<std::string> html = entry->ConvertToHtml();
    ASSERT_NE(html, nullptr);
    EXPECT_STREQ(html->c_str(), htmlStr.c_str());

    entry = record->GetEntryByMimeType(key);
    ASSERT_NE(entry, nullptr);
    customData = entry->ConvertToCustomData();
    ASSERT_NE(customData, nullptr);
    auto itemData = customData->GetItemData();
    auto item = itemData.find(key);
    ASSERT_NE(item, itemData.end());
    std::vector<uint8_t> dataArray = item->second;
    EXPECT_EQ(dataArray, val);
}

/**
 * @tc.name: GetEntryByMimeType007
 * @tc.desc: GetEntryByMimeType(custom)
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, GetEntryByMimeType007, TestSize.Level0)
{
    std::string key = "openharmony.styled-string";
    std::vector<uint8_t> val = {0x01, 0x02, 0x03};
    auto customData = std::make_shared<MineCustomData>();
    customData->AddItemData(key, val);

    PasteDataRecord::Builder builder(key);
    builder.SetCustomData(customData);
    auto record = builder.Build();
    ASSERT_NE(record, nullptr);

    auto entry = record->GetEntryByMimeType(key);
    ASSERT_NE(entry, nullptr);
    customData = entry->ConvertToCustomData();
    ASSERT_NE(customData, nullptr);
    auto itemData = customData->GetItemData();
    auto item = itemData.find(key);
    ASSERT_NE(item, itemData.end());
    std::vector<uint8_t> dataArray = item->second;
    EXPECT_EQ(dataArray, val);
}

/**
 * @tc.name: GetPassUriTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataRecordTest, GetPassUriTest001, TestSize.Level0)
{
    PasteDataRecord record;
    record.convertUri_ = "convertUri";
    EXPECT_EQ(record.GetPassUri(), "convertUri");
}

/**
 * @tc.name: GetPassUriTest002
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteDataRecordTest, GetPassUriTest002, TestSize.Level0)
{
    PasteDataRecord record;
    EXPECT_EQ(record.GetPassUri(), "");
}

/**
 * @tc.name: EncodeRemoteTest001
 * @tc.desc: test encode & decode remote plain-text
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, EncodeRemoteTest001, TestSize.Level0)
{
    PasteDataRecord obj1;
    auto udmfObject = std::make_shared<Object>();
    std::string utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    udmfObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udmfObject->value_[UDMF::CONTENT] = "content";
    auto entry1 = std::make_shared<PasteDataEntry>();
    entry1->SetValue(udmfObject);
    entry1->SetUtdId(utdId);
    entry1->SetMimeType(MIMETYPE_TEXT_PLAIN);
    obj1.AddEntry(utdId, entry1);

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer, true);
    ASSERT_TRUE(ret);

    PasteDataRecord obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetMimeType(), obj2.GetMimeType());
    auto entry2 = obj2.GetEntry(utdId);
    ASSERT_NE(entry2, nullptr);
    auto entryValue = entry2->GetValue();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Object>>(entryValue));
    auto udmfObject2 = std::get<std::shared_ptr<Object>>(entryValue);
    ASSERT_NE(udmfObject2, nullptr);
    EXPECT_EQ(udmfObject->value_, udmfObject2->value_);
}

/**
 * @tc.name: EncodeRemoteTest002
 * @tc.desc: test encode & decode remote html
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, EncodeRemoteTest002, TestSize.Level0)
{
    PasteDataRecord obj1;
    auto udmfObject = std::make_shared<Object>();
    std::string utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    udmfObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udmfObject->value_[UDMF::HTML_CONTENT] = "html content";
    udmfObject->value_[UDMF::PLAIN_CONTENT] = "text content";
    auto entry1 = std::make_shared<PasteDataEntry>();
    entry1->SetValue(udmfObject);
    entry1->SetUtdId(utdId);
    entry1->SetMimeType(MIMETYPE_TEXT_HTML);
    obj1.AddEntry(utdId, entry1);

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer, true);
    ASSERT_TRUE(ret);

    PasteDataRecord obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetMimeType(), obj2.GetMimeType());
    auto entry2 = obj2.GetEntry(utdId);
    ASSERT_NE(entry2, nullptr);
    auto entryValue = entry2->GetValue();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Object>>(entryValue));
    auto udmfObject2 = std::get<std::shared_ptr<Object>>(entryValue);
    ASSERT_NE(udmfObject2, nullptr);
    EXPECT_EQ(udmfObject->value_, udmfObject2->value_);
}

/**
 * @tc.name: EncodeRemoteTest003
 * @tc.desc: test encode & decode remote uri
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, EncodeRemoteTest003, TestSize.Level0)
{
    std::string uri = "file://content";
    PasteDataRecord obj1;
    auto udmfObject = std::make_shared<Object>();
    std::string utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    udmfObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udmfObject->value_[UDMF::FILE_URI_PARAM] = uri;
    auto entry1 = std::make_shared<PasteDataEntry>();
    entry1->SetValue(udmfObject);
    entry1->SetUtdId(utdId);
    entry1->SetMimeType(MIMETYPE_TEXT_URI);
    obj1.AddEntry(utdId, entry1);

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer, true);
    ASSERT_TRUE(ret);

    PasteDataRecord obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetMimeType(), obj2.GetMimeType());
    auto entry2 = obj2.GetEntry(utdId);
    ASSERT_NE(entry2, nullptr);
    auto uri2 = entry2->ConvertToUri();
    ASSERT_NE(uri2, nullptr);
    EXPECT_STREQ(uri.c_str(), uri2->ToString().c_str());
}

/**
 * @tc.name: EncodeRemoteTest004
 * @tc.desc: test encode & decode remote pixelMap
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, EncodeRemoteTest004, TestSize.Level0)
{
    const uint32_t color[] = { 0x80, 0x02, 0x04, 0x08, 0x40, 0x02, 0x04, 0x08 };
    uint32_t len = sizeof(color) / sizeof(color[0]);
    Media::InitializationOptions opts;
    opts.size.width = 2;
    opts.size.height = 3;
    opts.pixelFormat = Media::PixelFormat::UNKNOWN;
    opts.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    std::shared_ptr<OHOS::Media::PixelMap> pixelMap = Media::PixelMap::Create(color, len, 0, opts.size.width, opts);

    PasteDataRecord obj1;
    auto udmfObject = std::make_shared<Object>();
    std::string utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    udmfObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udmfObject->value_[UDMF::PIXEL_MAP] = pixelMap;
    auto entry1 = std::make_shared<PasteDataEntry>();
    entry1->SetValue(udmfObject);
    entry1->SetUtdId(utdId);
    entry1->SetMimeType(MIMETYPE_PIXELMAP);
    obj1.AddEntry(utdId, entry1);

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer, true);
    ASSERT_TRUE(ret);

    PasteDataRecord obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetMimeType(), obj2.GetMimeType());
    auto entry2 = obj2.GetEntry(utdId);
    ASSERT_NE(entry2, nullptr);
    auto pixelMap2 = entry2->ConvertToPixelMap();
    ASSERT_NE(pixelMap2, nullptr);
    EXPECT_TRUE(pixelMap->IsSameImage(*pixelMap2));
}

/**
 * @tc.name: EncodeRemoteTest005
 * @tc.desc: test encode & decode remote want
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, EncodeRemoteTest005, TestSize.Level0)
{
    std::shared_ptr<AAFwk::Want> want = std::make_shared<AAFwk::Want>();
    want->SetUri(uri_);

    PasteDataRecord obj1;
    std::string utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::OPENHARMONY_WANT);
    auto entry1 = std::make_shared<PasteDataEntry>();
    entry1->SetValue(want);
    entry1->SetUtdId(utdId);
    entry1->SetMimeType(MIMETYPE_TEXT_WANT);
    obj1.AddEntry(utdId, entry1);

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer, true);
    ASSERT_TRUE(ret);

    PasteDataRecord obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetMimeType(), obj2.GetMimeType());
    auto entry2 = obj2.GetEntry(utdId);
    ASSERT_NE(entry2, nullptr);
    auto entryValue = entry2->GetValue();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<AAFwk::Want>>(entryValue));
    auto want2 = std::get<std::shared_ptr<AAFwk::Want>>(entryValue);
    ASSERT_NE(want2, nullptr);
    EXPECT_STREQ(want2->ToString().c_str(), want->ToString().c_str());
}

/**
 * @tc.name: EncodeRemoteTest006
 * @tc.desc: test encode & decode remote custom
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataRecordTest, EncodeRemoteTest006, TestSize.Level0)
{
    std::string mimeType = "openharmony.styled-string";
    std::vector<uint8_t> array = {0x01, 0x02, 0x03};
    auto customData = std::make_shared<MineCustomData>();
    customData->AddItemData(mimeType, array);

    PasteDataRecord::Builder builder(mimeType);
    builder.SetCustomData(customData);

    auto obj1 = builder.Build();
    ASSERT_NE(obj1, nullptr);

    std::vector<uint8_t> buffer;
    bool ret = obj1->Encode(buffer, true);
    ASSERT_TRUE(ret);

    PasteDataRecord obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1->GetMimeType(), obj2.GetMimeType());
    auto entry2 = obj2.GetEntryByMimeType(mimeType);
    ASSERT_NE(entry2, nullptr);
    auto entryValue = entry2->GetValue();
    ASSERT_TRUE(std::holds_alternative<std::vector<uint8_t>>(entryValue));
    auto array2 = std::get<std::vector<uint8_t>>(entryValue);
    EXPECT_EQ(array2, array);
}

/**
 * @tc.name: AddUriEntryTest001
 * @tc.desc: AddUriEntryTest001
 * @tc.type: FUNC
 * @tc.require:entries
 * @tarowang
 */
HWTEST_F(PasteDataRecordTest, AddUriEntryTest001, TestSize.Level2)
{
    auto tempPasteboard = std::make_shared<PasteDataRecord>();
    EXPECT_NE(tempPasteboard, nullptr);

    tempPasteboard->AddUriEntry();
}

/**
 * @tc.name: NewMultiTypeRecordTest002
 * @tc.desc: NewMultiTypeRecordTest002
 * @tc.type: FUNC
 * @tc.require:entries
 * @tarowang
 */
HWTEST_F(PasteDataRecordTest, NewMultiTypeRecordTest002, TestSize.Level2)
{
    std::shared_ptr<std::map<std::string, std::shared_ptr<EntryValue>>>values = nullptr;
    std::string recordMimeType = "text/pain";

    auto result = PasteDataRecord::NewMultiTypeRecord(values, recordMimeType);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(result->GetEntries().empty());
}

/**
 * @tc.name: NewMultiTypeRecordTest003
 * @tc.desc: NewMultiTypeRecordTest003
 * @tc.type: FUNC
 * @tc.require:entries
 * @tarowang
 */
HWTEST_F(PasteDataRecordTest, NewMultiTypeRecordTest003, TestSize.Level2)
{

    auto values = std::make_shared<std::map<std::string, std::shared_ptr<EntryValue>>>();
    std::string recordMimeType = "";

    auto result = PasteDataRecord::NewMultiTypeRecord(values, recordMimeType);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(result->GetEntries().empty());
}

/**
 * @tc.name: DecodeTLVTest001
 * @tc.desc: DecodeTLVTest001
 * @tc.type: FUNC
 * @tc.require:entries
 * @tarowang
 */
HWTEST_F(PasteDataRecordTest, DecodeTLVTest001, TestSize.Level2)
{
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    EXPECT_NE(record, nullptr);
    std::vector<std::uint8_t> buffer;
    ReadOnlyBuffer buff(buffer);
    bool ret = record->DecodeTLV(buff);

    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: GetUriPermissionTest001
 * @tc.desc: GetUriPermissionTest001
 * @tc.type: FUNC
 * @tc.require:entries
 * @tarowang
 */
HWTEST_F(PasteDataRecordTest, GetUriPermissionTest001, TestSize.Level2)
{
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    EXPECT_NE(record, nullptr);
    record->SetUriPermission(PasteDataRecord::READ_PERMISSION);
    auto ret = record->GetUriPermission();
    EXPECT_EQ(ret, PasteDataRecord::READ_PERMISSION);
}

/**
 * @tc.name: SetForm001
 * @tc.desc: SetForm001;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataRecordTest, SetForm001, TestSize.Level0)
{
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    EXPECT_NE(record, nullptr);
    uint32_t from = 1;
    record->SetFrom(from);
    EXPECT_EQ(record->from_, from);
}
} // namespace OHOS::MiscServices
