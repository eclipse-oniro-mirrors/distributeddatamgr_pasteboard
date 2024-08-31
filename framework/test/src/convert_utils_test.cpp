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

#include "convert_utils.h"

#include <gtest/gtest.h>
#include <paste_data.h>
#include <unified_data.h>

#include "paste_data_entry.h"
#include "tlv_object.h"
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
    std::vector<uint8_t> rawData2_ = { 1, 2, 3, 4, 5, 6, 7, 8, 9};

    void CheckEntries(const std::vector<std::shared_ptr<PasteDataEntry>>& entries);
    void CheckPlainUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckFileUriUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckPixelMapUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckHtmlUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckLinkUds(const std::shared_ptr<PasteDataEntry> entry);
    void CheckCustomEntry(const std::shared_ptr<PasteDataEntry> entry);

    void InitDataWithEntries(UDMF::UnifiedData& data);
    void InitDataWithPlainEntry(UDMF::UnifiedData& data);
    void InitDataWithHtmlEntry(UDMF::UnifiedData& data);
    void InitDataWithFileUriEntry(UDMF::UnifiedData& data);
    void InitDataWithPixelMapEntry(UDMF::UnifiedData& data);
    void InitDataWitCustomEntry(UDMF::UnifiedData& data);
    void InitDataWitSameCustomEntry(UDMF::UnifiedData& data);

    void AddPlainUdsEntry(UDMF::UnifiedRecord& record);
    void AddFileUriUdsEntry(UDMF::UnifiedRecord& record);
    void AddHtmlUdsEntry(UDMF::UnifiedRecord& record);
    void AddPixelMapUdsEntry(UDMF::UnifiedRecord& record);
    void AddLinkUdsEntry(UDMF::UnifiedRecord& record);
    void AddCustomEntry(UDMF::UnifiedRecord& record);
    void AddCustomEntries(UDMF::UnifiedRecord& record);

    static PasteData TlvData(const std::shared_ptr<PasteData>& data);
};

void ConvertUtilsTest::SetUpTestCase(void) {}

void ConvertUtilsTest::TearDownTestCase(void) {}

void ConvertUtilsTest::SetUp(void) {}

void ConvertUtilsTest::TearDown(void) {}

PasteData ConvertUtilsTest::TlvData(const std::shared_ptr<PasteData>& data)
{
    std::vector<std::uint8_t> buffer;
    data->Init(buffer);
    data->Encode(buffer);
    PasteData decodePasteData;
    decodePasteData.Decode(buffer);
    return decodePasteData;
}

void ConvertUtilsTest::AddPlainUdsEntry(UDMF::UnifiedRecord& record)
{
    Object plainUds;
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    plainUds.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    plainUds.value_[UDMF::CONTENT] = text_;
    record.AddEntry(utdId, std::make_shared<Object>(plainUds));
}

void ConvertUtilsTest::AddFileUriUdsEntry(UDMF::UnifiedRecord& record)
{
    Object fileUriobject;
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    fileUriobject.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    fileUriobject.value_[UDMF::FILE_URI_PARAM] = uri_;
    fileUriobject.value_[UDMF::FILE_TYPE] = "";
    record.AddEntry(utdId, std::make_shared<Object>(fileUriobject));
}

void ConvertUtilsTest::AddHtmlUdsEntry(UDMF::UnifiedRecord& record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    Object htmlobject;
    htmlobject.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    htmlobject.value_[UDMF::HTML_CONTENT] = html_;
    record.AddEntry(utdId, std::make_shared<Object>(htmlobject));
}

void ConvertUtilsTest::AddLinkUdsEntry(UDMF::UnifiedRecord& record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK);
    Object linkObject;
    linkObject.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    linkObject.value_[UDMF::URL] = link_;
    record.AddEntry(utdId, std::make_shared<Object>(linkObject));
}

void ConvertUtilsTest::AddCustomEntry(UDMF::UnifiedRecord& record)
{
    record.AddEntry(appUtdId1_, rawData1_);
}

void ConvertUtilsTest::AddCustomEntries(UDMF::UnifiedRecord& record)
{
    record.AddEntry(appUtdId1_, rawData1_);
    record.AddEntry(appUtdId2_, rawData2_);
}

void ConvertUtilsTest::AddPixelMapUdsEntry(UDMF::UnifiedRecord& record)
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    Object object;
    object.value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    object.value_[UDMF::PIXEL_MAP] = pixelMapIn;
    record.AddEntry(utdId, std::make_shared<Object>(object));
}

void ConvertUtilsTest::InitDataWithPlainEntry(UDMF::UnifiedData& data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddPlainUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithHtmlEntry(UDMF::UnifiedData& data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddHtmlUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithFileUriEntry(UDMF::UnifiedData& data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddFileUriUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWithPixelMapEntry(UDMF::UnifiedData& data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddPixelMapUdsEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWitCustomEntry(UDMF::UnifiedData& data)
{
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>();
    AddCustomEntry(*record);
    data.AddRecord(record);
    auto size = data.GetRecords().size();
    ASSERT_EQ(1, size);
    auto entriesSize = data.GetRecordAt(0)->GetEntries()->size();
    ASSERT_EQ(1, entriesSize);
}

void ConvertUtilsTest::InitDataWitSameCustomEntry(UDMF::UnifiedData& data)
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

void ConvertUtilsTest::InitDataWithEntries(UDMF::UnifiedData& data)
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

void ConvertUtilsTest::CheckEntries(const std::vector<std::shared_ptr<PasteDataEntry>>& entries)
{
    for (auto const& entry : entries) {
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
    auto plain = record->GetPlainText();
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
    auto plain = record->GetHtmlText();
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
    auto pixelMap = record->GetPixelMap();
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
    auto plain = record->GetPlainText();
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
} // namespace OHOS::MiscServices