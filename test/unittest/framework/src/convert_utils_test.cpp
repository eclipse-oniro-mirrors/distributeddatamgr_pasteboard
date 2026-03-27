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
#include <paste_data.h>
#include <unified_data.h>

#include "convert_utils.h"
#include "paste_data_entry.h"
#include "pasteboard_hilog.h"
#include "unified_meta.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
class ConvertUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    std::string text_ = "test";
    std::string extraText_ = "extr";
    std::string uri_ = "";
    std::string html_ = "<div class='disable'>helloWorld</div>";
    std::string link_ = "http://abc.com";
    std::string appUtdId1_ = "appdefined-mytype1";
    std::string appUtdId2_ = "appdefined-mytype2";
    std::vector<uint8_t> rawData1_ = { 1, 2, 3, 4, 5, 6, 7, 8 };
    std::vector<uint8_t> rawData2_ = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    void CheckEntries(const std::vector<std::shared_ptr<PasteDataEntry>> &entries);
    void CheckPlainUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckFileUriUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckPixelMapUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckHtmlUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckLinkUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckCustomEntry(const std::shared_ptr<PasteDataEntry> entry);

    void InitDataWithEntries(UDMF::UnifiedData &data);
    void InitDataWithPlainEntry(UDMF::UnifiedData &data);
    void InitDataWithHtmlEntry(UDMF::UnifiedData &data);
    void InitDataWithFileUriEntry(UDMF::UnifiedData &data);
    void InitDataWithPixelMapEntry(UDMF::UnifiedData &data);
    void InitDataWitCustomEntry(UDMF::UnifiedData &data);
    void InitDataWitSameCustomEntry(UDMF::UnifiedData &data);

    void AddPlainUdsEntry(UDMF::UnifiedRecord &record);
    void AddFileUriUdsEntry(UDMF::UnifiedRecord &record);
    void AddHtmlUdsEntry(UDMF::UnifiedRecord &record);
    void AddPixelMapUdsEntry(UDMF::UnifiedRecord &record);
    void AddLinkUdsEntry(UDMF::UnifiedRecord &record);
    void AddCustomEntry(UDMF::UnifiedRecord &record);
    void AddCustomEntries(UDMF::UnifiedRecord &record);

    static PasteData TlvData(const std::shared_ptr<PasteData> &data);
};

void ConvertUtilsTest::SetUpTestCase(void) { }

void ConvertUtilsTest::TearDownTestCase(void) { }

void ConvertUtilsTest::SetUp(void) { }

void ConvertUtilsTest::TearDown(void) { }

PasteData ConvertUtilsTest::TlvData(const std::shared_ptr<PasteData> &data)
{
    std::vector<std::uint8_t> buffer;
    data->Encode(buffer);
    PasteData decodePasteData;
    decodePasteData.Decode(buffer);
    return decodePasteData;
}

void ConvertUtilsTest::AddPlainUdsEntry(UDMF::UnifiedRecord &record)
{
    Object plainUds;
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    plainUds.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    plainUds.value_[UDMF::CONTENT] = text_;
    record.AddEntry(utdId, std::make_shared<Object>(plainUds));
}

void ConvertUtilsTest::AddFileUriUdsEntry(UDMF::UnifiedRecord &record)
{
    Object fileUriobject;
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    fileUriobject.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    fileUriobject.value_[UDMF::FILE_URI_PARAM] = uri_;
    fileUriobject.value_[UDMF::FILE_TYPE] = "";
    record.AddEntry(utdId, std::make_shared<Object>(fileUriobject));
}

void ConvertUtilsTest::AddHtmlUdsEntry(UDMF::UnifiedRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    Object htmlobject;
    htmlobject.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    htmlobject.value_[UDMF::HTML_CONTENT] = html_;
    record.AddEntry(utdId, std::make_shared<Object>(htmlobject));
}

void ConvertUtilsTest::AddLinkUdsEntry(UDMF::UnifiedRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK);
    Object linkObject;
    linkObject.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    linkObject.value_[UDMF::URL] = link_;
    record.AddEntry(utdId, std::make_shared<Object>(linkObject));
}

void ConvertUtilsTest::AddCustomEntry(UDMF::UnifiedRecord &record)
{
    record.AddEntry(appUtdId1_, rawData1_);
}

void ConvertUtilsTest::AddCustomEntries(UDMF::UnifiedRecord &record)
{
    record.AddEntry(appUtdId1_, rawData1_);
    record.AddEntry(appUtdId2_, rawData2_);
}

void ConvertUtilsTest::AddPixelMapUdsEntry(UDMF::UnifiedRecord &record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    Object object;
    object.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    object.value_[UDMF::PIXEL_MAP] = pixelMapIn;
    record.AddEntry(utdId, std::make_shared<Object>(object));
}

void ConvertUtilsTest::InitDataWithPlainEntry(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddPlainUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithHtmlEntry(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddHtmlUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithFileUriEntry(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddFileUriUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithPixelMapEntry(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddPixelMapUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWitCustomEntry(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddCustomEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWitSameCustomEntry(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddCustomEntry(*record);
    record->AddEntry(appUtdId1_, rawData2_);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithEntries(UDMF::UnifiedData &data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddPlainUdsEntry(*record);
    AddHtmlUdsEntry(*record);
    AddFileUriUdsEntry(*record);
    AddLinkUdsEntry(*record);
    AddCustomEntries(*record);
    auto entriesSize = record->GetEntries()->size();
    ASSERT_EQ(6, entriesSize);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
}

void ConvertUtilsTest::CheckEntries(const std::vector<std::shared_ptr<PasteDataEntry>> &entries)
{
    for (auto const &entry : entries) {
        if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT)) {
            CheckPlainUds(entry);
        } else if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI)) {
            CheckFileUriUds(entry);
        } else if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP)) {
            CheckPixelMapUds(entry);
        } else if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK)) {
            CheckLinkUds(entry);
        } else if (entry->GetUtdId() == UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML)) {
            CheckHtmlUds(entry);
        } else {
            CheckCustomEntry(entry);
        }
    }
}

void ConvertUtilsTest::CheckPlainUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_NE(entry, nullptr);
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

void ConvertUtilsTest::CheckFileUriUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_NE(entry, nullptr);
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

void ConvertUtilsTest::CheckHtmlUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_NE(entry, nullptr);
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

void ConvertUtilsTest::CheckPixelMapUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_NE(entry, nullptr);
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
    ASSERT_TRUE(imageInfo.size.height == 7);
    ASSERT_TRUE(imageInfo.size.width == 5);
    ASSERT_TRUE(imageInfo.pixelFormat == PixelFormat::ARGB_8888);
}

void ConvertUtilsTest::CheckLinkUds(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_NE(entry, nullptr);
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

void ConvertUtilsTest::CheckCustomEntry(const std::shared_ptr<PasteDataEntry> entry)
{
    ASSERT_NE(entry, nullptr);
    ASSERT_EQ(entry->GetUtdId(), entry->GetMimeType());
    auto decodeValue = entry->GetValue();
    auto object = std::get_if<std::vector<uint8_t>>(&decodeValue);
    ASSERT_NE(object, nullptr);
    if (entry->GetUtdId() == appUtdId1_) {
        ASSERT_EQ(*object, rawData1_);
    } else {
        ASSERT_EQ(*object, rawData2_);
    }
}

/**
 * @tc.name: PlainEntryTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(ConvertUtilsTest, PlainEntryTest001, TestSize.Level0)
{
    UDMF::UnifiedData data;
    InitDataWithPlainEntry(data);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    auto pasteData = ConvertUtils::Convert(data);
    auto decodePasteData = TlvData(pasteData);
    ASSERT_EQ(1, decodePasteData.GetRecordCount());
    auto record = decodePasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::PLAIN_TEXT);
    auto plain = record->GetPlainTextV0();
    ASSERT_EQ(*plain, text_);
    auto entries = record->GetEntries();
    ASSERT_EQ(entries.size(), entriesSize);
    CheckEntries(entries);
}

/**
 * @tc.name: HtmlEntryTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(ConvertUtilsTest, HtmlEntryTest001, TestSize.Level0)
{
    UDMF::UnifiedData data;
    InitDataWithHtmlEntry(data);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    auto pasteData = ConvertUtils::Convert(data);
    auto decodePasteData = TlvData(pasteData);
    ASSERT_EQ(1, decodePasteData.GetRecordCount());
    auto record = decodePasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_HTML);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::HTML);
    auto plain = record->GetHtmlTextV0();
    ASSERT_EQ(*plain, html_);
    auto entries = record->GetEntries();
    ASSERT_EQ(entries.size(), entriesSize);
    CheckEntries(entries);
}

/**
 * @tc.name: PixelMapEntryTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(ConvertUtilsTest, PixelMapEntryTest001, TestSize.Level0)
{
    UDMF::UnifiedData data;
    InitDataWithPixelMapEntry(data);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    auto pasteData = ConvertUtils::Convert(data);
    auto decodePasteData = TlvData(pasteData);
    ASSERT_EQ(1, decodePasteData.GetRecordCount());
    auto record = decodePasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_PIXELMAP);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::SYSTEM_DEFINED_PIXEL_MAP);
    auto pixelMap = record->GetPixelMapV0();
    ASSERT_NE(pixelMap, nullptr);
    ImageInfo imageInfo = {};
    pixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == 7);
    ASSERT_TRUE(imageInfo.size.width == 5);
    ASSERT_TRUE(imageInfo.pixelFormat == PixelFormat::ARGB_8888);
    auto entries = record->GetEntries();
    ASSERT_EQ(entries.size(), entriesSize);
    CheckEntries(entries);
}

/**
 * @tc.name: EntriesTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(ConvertUtilsTest, EntriesTest001, TestSize.Level0)
{
    UDMF::UnifiedData data;
    InitDataWithEntries(data);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    auto pasteData = ConvertUtils::Convert(data);
    auto decodePasteData = TlvData(pasteData);
    ASSERT_EQ(1, decodePasteData.GetRecordCount());
    auto record = decodePasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::PLAIN_TEXT);
    auto plain = record->GetPlainTextV0();
    ASSERT_NE(plain, nullptr);
    ASSERT_EQ(*plain, text_);

    auto entries = record->GetEntries();
    ASSERT_EQ(entries.size(), entriesSize);
    CheckEntries(entries);
}

/**
 * @tc.name: SameTypeEntryTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(ConvertUtilsTest, SameTypeEntryTest001, TestSize.Level0)
{
    UDMF::UnifiedData data;
    InitDataWitSameCustomEntry(data);
    auto pasteData = ConvertUtils::Convert(data);
    auto decodePasteData = TlvData(pasteData);
    ASSERT_EQ(1, decodePasteData.GetRecordCount());
    auto record = decodePasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, appUtdId1_);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::APPLICATION_DEFINED_RECORD);
    auto customData = record->GetCustomData();
    ASSERT_NE(customData, nullptr);
    auto rawData = customData->GetItemData();
    ASSERT_EQ(rawData.size(), 1);
    ASSERT_EQ(rawData[appUtdId1_], rawData2_);
}

/**
 * @tc.name: ConvertPropertyTest001
 * @tc.desc: Test ConvertProperty function when properties is nullptr
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest001 start");
    std::shared_ptr<UDMF::UnifiedDataProperties> properties = nullptr;
    UDMF::UnifiedData unifiedData;
    PasteDataProperty result;

    result.shareOption = InApp;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, 0);

    result.shareOption = LocalDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, 0);

    result.shareOption = CrossDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, 0);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest001 end");
}

/**
 * @tc.name: ConvertPropertyTest002
 * @tc.desc: Test ConvertProperty function when properties->shareOptions is UDMF::IN_APP
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest002 start");
    std::shared_ptr<UDMF::UnifiedDataProperties> properties = std::make_shared<UDMF::UnifiedDataProperties>();
    UDMF::UnifiedData unifiedData;
    PasteDataProperty result;

    properties->shareOptions = UDMF::IN_APP;
    result.shareOption = LocalDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, InApp);
    result.shareOption = CrossDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, InApp);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest002 end");
}

/**
 * @tc.name: ConvertPropertyTest003
 * @tc.desc: Test ConvertProperty function when properties->shareOptions is UDMF::CROSS_APP
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest003 start");
    std::shared_ptr<UDMF::UnifiedDataProperties> properties = std::make_shared<UDMF::UnifiedDataProperties>();
    UDMF::UnifiedData unifiedData;
    PasteDataProperty result;

    properties->shareOptions = UDMF::CROSS_APP;
    result.shareOption = InApp;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, LocalDevice);
    result.shareOption = CrossDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, LocalDevice);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest003 end");
}

/**
 * @tc.name: ConvertPropertyTest004
 * @tc.desc: Test ConvertProperty function when properties->shareOptions is UDMF::CROSS_DEVICE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest004 start");
    std::shared_ptr<UDMF::UnifiedDataProperties> properties = std::make_shared<UDMF::UnifiedDataProperties>();
    UDMF::UnifiedData unifiedData;
    PasteDataProperty result;

    properties->shareOptions = UDMF::CROSS_DEVICE;
    result.shareOption = InApp;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, CrossDevice);
    result.shareOption = LocalDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, CrossDevice);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest004 end");
}

/**
 * @tc.name: ConvertPropertyTest005
 * @tc.desc: Test ConvertProperty function when properties->shareOptions is UDMF::SHARE_OPTIONS_BUTT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest005 start");
    std::shared_ptr<UDMF::UnifiedDataProperties> properties = std::make_shared<UDMF::UnifiedDataProperties>();
    UDMF::UnifiedData unifiedData;
    PasteDataProperty result;

    properties->shareOptions = UDMF::SHARE_OPTIONS_BUTT;
    result.shareOption = InApp;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, CrossDevice);
    result.shareOption = LocalDevice;
    result = ConvertUtils::ConvertProperty(properties, unifiedData);
    EXPECT_EQ(result.shareOption, CrossDevice);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest005 end");
}

/**
 * @tc.name: ConvertPropertyTest006
 * @tc.desc: Test ConvertProperty function when properties.shareOption is InApp
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest006 start");
    PasteDataProperty properties;
    std::shared_ptr<UDMF::UnifiedDataProperties> result = std::make_shared<UDMF::UnifiedDataProperties>();

    properties.shareOption = InApp;
    result->shareOptions = UDMF::CROSS_APP;
    result = ConvertUtils::ConvertProperty(properties);
    EXPECT_EQ(result->shareOptions, UDMF::IN_APP);
    result->shareOptions = UDMF::CROSS_DEVICE;
    result = ConvertUtils::ConvertProperty(properties);
    EXPECT_EQ(result->shareOptions, UDMF::IN_APP);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest006 end");
}

/**
 * @tc.name: ConvertPropertyTest007
 * @tc.desc: Test ConvertProperty function when properties.shareOption is LocalDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest007, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest007 start");
    PasteDataProperty properties;
    std::shared_ptr<UDMF::UnifiedDataProperties> result = std::make_shared<UDMF::UnifiedDataProperties>();

    properties.shareOption = LocalDevice;
    result->shareOptions = UDMF::IN_APP;
    result = ConvertUtils::ConvertProperty(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_APP);
    result->shareOptions = UDMF::CROSS_DEVICE;
    result = ConvertUtils::ConvertProperty(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_APP);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest007 end");
}

/**
 * @tc.name: ConvertPropertyTest008
 * @tc.desc: Test ConvertProperty function when properties.shareOption is CrossDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPropertyTest008, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest008 start");
    PasteDataProperty properties;
    std::shared_ptr<UDMF::UnifiedDataProperties> result = std::make_shared<UDMF::UnifiedDataProperties>();

    properties.shareOption = CrossDevice;
    result->shareOptions = UDMF::IN_APP;
    result = ConvertUtils::ConvertProperty(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_DEVICE);
    result->shareOptions = UDMF::CROSS_APP;
    result = ConvertUtils::ConvertProperty(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_DEVICE);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertPropertyTest008 end");
}

/**
 * @tc.name: ConvertPlainTextTest
 * @tc.desc: Test convert function when UNIFROM_DATA_TYPE is PLAIN_TEXT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPlainTextTest, TestSize.Level0)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>(utdId, text_);
    ASSERT_NE(entry, nullptr);
    record->AddEntry(utdId, entry);
    ASSERT_NE(record, nullptr);
    UDMF::ValueType result = ConvertUtils::Convert(entry, record);
    auto object = std::get_if<std::shared_ptr<Object>>(&result);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    if (std::holds_alternative<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE])) {
        auto& typeValue = std::get<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE]);
        ASSERT_NE(&typeValue, nullptr);
        ASSERT_EQ(typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT));
    }
    if (std::holds_alternative<std::string>(objectValue[UDMF::CONTENT])) {
        auto& value = std::get<std::string>(objectValue[UDMF::CONTENT]);
        ASSERT_NE(&value, nullptr);
        ASSERT_EQ(value, text_);
    }
}

/**
 * @tc.name: ConvertHtmlTest
 * @tc.desc: Test convert function when UNIFROM_DATA_TYPE is HTML
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertHtmlTest, TestSize.Level0)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>(utdId, html_);
    ASSERT_NE(entry, nullptr);
    record->AddEntry(utdId, entry);
    ASSERT_NE(record, nullptr);
    UDMF::ValueType result = ConvertUtils::Convert(entry, record);
    auto object = std::get_if<std::shared_ptr<Object>>(&result);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    if (std::holds_alternative<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE])) {
        auto& typeValue = std::get<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE]);
        ASSERT_NE(&typeValue, nullptr);
        ASSERT_EQ(typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML));
    }
    if (std::holds_alternative<std::string>(objectValue[UDMF::CONTENT])) {
        auto& value = std::get<std::string>(objectValue[UDMF::CONTENT]);
        ASSERT_NE(&value, nullptr);
        ASSERT_EQ(value, html_);
    }
}

/**
 * @tc.name: ConvertFileUriTest
 * @tc.desc: Test convert function when UNIFROM_DATA_TYPE is FILE_URI
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertFileUriTest, TestSize.Level0)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>(utdId, uri_);
    ASSERT_NE(entry, nullptr);
    record->AddEntry(utdId, entry);
    ASSERT_NE(record, nullptr);
    UDMF::ValueType result = ConvertUtils::Convert(entry, record);
    auto object = std::get_if<std::shared_ptr<Object>>(&result);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    if (std::holds_alternative<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE])) {
        auto& typeValue = std::get<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE]);
        ASSERT_NE(&typeValue, nullptr);
        ASSERT_EQ(typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI));
    }
    if (std::holds_alternative<std::string>(objectValue[UDMF::CONTENT])) {
        auto& value = std::get<std::string>(objectValue[UDMF::CONTENT]);
        ASSERT_NE(&value, nullptr);
        ASSERT_EQ(value, uri_);
    }
}

/**
 * @tc.name: ConvertPixelMapTest
 * @tc.desc: Test convert function when UNIFROM_DATA_TYPE is SYSTEM_DEFINED_PIXEL_MAP
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ConvertUtilsTest, ConvertPixelMapTest, TestSize.Level0)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    std::shared_ptr<PasteDataRecord> record = std::make_shared<PasteDataRecord>();
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>(utdId, pixelMapIn);
    ASSERT_NE(entry, nullptr);
    record->AddEntry(utdId, entry);
    ASSERT_NE(record, nullptr);
    UDMF::ValueType result = ConvertUtils::Convert(entry, record);
    auto object = std::get_if<std::shared_ptr<Object>>(&result);
    ASSERT_NE(object, nullptr);
    auto objectValue = (*object)->value_;
    if (std::holds_alternative<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE])) {
        auto& typeValue = std::get<std::string>(objectValue[UDMF::UNIFORM_DATA_TYPE]);
        ASSERT_NE(&typeValue, nullptr);
        ASSERT_EQ(typeValue, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP));
    }
    if (std::holds_alternative<std::string>(objectValue[UDMF::CONTENT])) {
        auto& value = std::get<std::string>(objectValue[UDMF::CONTENT]);
        ASSERT_NE(&value, nullptr);
    }

    auto inPixelMap = record->GetPixelMapV0();
    ASSERT_NE(inPixelMap, nullptr);
    ImageInfo imageInfo = {};
    inPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == 7);
    ASSERT_TRUE(imageInfo.size.width == 5);
    ASSERT_TRUE(imageInfo.pixelFormat == PixelFormat::ARGB_8888);
}
} // namespace OHOS::MiscServices