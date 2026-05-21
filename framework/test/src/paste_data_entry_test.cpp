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

#include "convert_utils.h"
#include "pixel_map.h"
#include "unified_meta.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
using UDType = UDMF::UDType;
class PasteDataEntryTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    PasteDataEntry InitPlainTextEntry();
    PasteDataEntry InitPixelMapEntry();
    PasteDataEntry InitUriEntry();
    PasteDataEntry InitWantEntry();
    PasteDataEntry InitHtmlEntry();
    void CheckPlainUds(std::shared_ptr<PasteDataEntry> entry);
    void CheckPixelMapUds(std::shared_ptr<PasteDataEntry> entry);
    std::string text_ = "test_string";
    std::string uri_ = "file://test_uri";
    std::string html_ = "<div class='disable'>helloWorld</div>";
    int32_t width_ = 5;
    int32_t height_ = 7;
};

void PasteDataEntryTest::SetUpTestCase(void) { }

void PasteDataEntryTest::TearDownTestCase(void) { }

void PasteDataEntryTest::SetUp(void) { }

void PasteDataEntryTest::TearDown(void) { }

PasteDataEntry PasteDataEntryTest::InitPlainTextEntry()
{
    auto udsObject = std::make_shared<Object>();
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    udsObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udsObject->value_[UDMF::CONTENT] = text_;
    return { utdId, udsObject };
}

PasteDataEntry PasteDataEntryTest::InitUriEntry()
{
    auto udsObject = std::make_shared<Object>();
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    udsObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udsObject->value_[UDMF::FILE_URI_PARAM] = uri_;
    udsObject->value_[UDMF::FILE_TYPE] = "";
    return { utdId, udsObject };
}

PasteDataEntry PasteDataEntryTest::InitWantEntry()
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::OPENHARMONY_WANT);
    using namespace OHOS::AAFwk;
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string idKey = "id";
    int32_t idValue = 123;
    std::string deviceKey = "deviceId_key";
    want->SetParam(idKey, idValue);
    return { utdId, want };
}

PasteDataEntry PasteDataEntryTest::InitHtmlEntry()
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    auto udsObject = std::make_shared<Object>();
    udsObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    udsObject->value_[UDMF::HTML_CONTENT] = html_;
    return { utdId, udsObject };
}

PasteDataEntry PasteDataEntryTest::InitPixelMapEntry()
{
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    auto udsObject = std::make_shared<Object>();
    udsObject->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    udsObject->value_[UDMF::PIXEL_MAP] = pixelMapIn;
    return { utdId, udsObject };
}

void PasteDataEntryTest::CheckPlainUds(const std::shared_ptr<PasteDataEntry> entry)
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

void PasteDataEntryTest::CheckPixelMapUds(const std::shared_ptr<PasteDataEntry> entry)
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

/**
 * @tc.name: Convert001
 * @tc.desc: convert to plainText;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataEntryTest, Convert001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert001 start");
    auto entry = InitPlainTextEntry();
    auto str = entry.ConvertToPlainText();
    ASSERT_NE(nullptr, str);
    EXPECT_EQ(text_, *str);

    entry = InitHtmlEntry();
    auto html = entry.ConvertToHtml();
    ASSERT_NE(nullptr, html);
    EXPECT_EQ(html_, *html);

    entry = InitUriEntry();
    auto uri = entry.ConvertToUri();
    ASSERT_NE(nullptr, uri);
    EXPECT_EQ(uri_, uri->ToString());

    entry = InitWantEntry();
    auto want = entry.ConvertToWant();
    ASSERT_NE(nullptr, want);
    int32_t idValue1 = want->GetIntParam("id", 0);
    ASSERT_EQ(idValue1, 123);

    entry = InitPixelMapEntry();
    auto pixelMap = entry.ConvertToPixelMap();
    ASSERT_NE(nullptr, pixelMap);
    ImageInfo imageInfo = {};
    pixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == height_);
    ASSERT_TRUE(imageInfo.size.width == width_);
    ASSERT_TRUE(imageInfo.pixelFormat == PixelFormat::ARGB_8888);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert001 end");
}

/**
 * @tc.name: Convert002
 * @tc.desc: convert to html;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataEntryTest, Convert002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert002 start");
    PasteDataEntry entry;
    std::string utdId = "NotHTML";
    entry.SetUtdId(utdId);
    auto html = entry.ConvertToHtml();
    EXPECT_EQ(html, nullptr);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    entry.SetUtdId(utdId);
    std::string value_str = "TestString";
    entry.SetValue(value_str);
    html = entry.ConvertToHtml();
    EXPECT_EQ(*html, "TestString");

    int32_t value_int = 123;
    entry.SetValue(value_int);
    html = entry.ConvertToHtml();
    EXPECT_EQ(html, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert002 end");
}

/**
 * @tc.name: Convert003
 * @tc.desc: convert to uri;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataEntryTest, Convert003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert003 start");
    PasteDataEntry entry;
    std::string utdId = "NotURI";
    entry.SetUtdId(utdId);
    auto uri = entry.ConvertToUri();
    EXPECT_EQ(uri, nullptr);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    entry.SetUtdId(utdId);
    std::string value_str = "TestString";
    entry.SetValue(value_str);
    auto value = entry.GetValue();
    auto res = std::get<std::string>(value);
    uri = entry.ConvertToUri();
    EXPECT_EQ(*uri, Uri(res));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert003 end");
}

/**
 * @tc.name: Convert004
 * @tc.desc: convert to want;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataEntryTest, Convert004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert004 start");
    PasteDataEntry entry;
    std::string utdId = "NotWANT";
    entry.SetUtdId(utdId);
    auto want = entry.ConvertToWant();
    EXPECT_EQ(want, nullptr);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::OPENHARMONY_WANT);
    entry.SetUtdId(utdId);
    std::string value_str = "TestString";
    entry.SetValue(value_str);
    want = entry.ConvertToWant();
    EXPECT_EQ(want, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert004 end");
}

/**
 * @tc.name: Convert005
 * @tc.desc: convert to PixelMap;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataEntryTest, Convert005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert005 start");
    PasteDataEntry entry;
    std::string utdId = "NotPixelMap";
    entry.SetUtdId(utdId);
    auto pixelMap = entry.ConvertToPixelMap();
    EXPECT_EQ(pixelMap, nullptr);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    entry.SetUtdId(utdId);
    auto pixelmap_ = std::make_shared<Media::PixelMap>();
    entry.SetValue(pixelmap_);
    pixelMap = entry.ConvertToPixelMap();
    EXPECT_EQ(pixelMap, pixelmap_);

    std::string value_str = "TestString";
    entry.SetValue(value_str);
    pixelMap = entry.ConvertToPixelMap();
    EXPECT_EQ(pixelMap, nullptr);

    auto object = std::make_shared<Object>();
    object->value_[UDMF::HTML_CONTENT] = html_;
    entry.SetValue(object);
    pixelMap = entry.ConvertToPixelMap();
    EXPECT_EQ(pixelMap, nullptr);

    object->value_[UDMF::UNIFORM_DATA_TYPE] = html_;
    entry.SetValue(object);
    pixelMap = entry.ConvertToPixelMap();
    EXPECT_EQ(pixelMap, nullptr);

    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    entry.SetValue(object);
    pixelMap = entry.ConvertToPixelMap();
    EXPECT_EQ(pixelMap, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert005 end");
}

/**
 * @tc.name: Convert006
 * @tc.desc: convert to CustomData;
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author: tarowang
 */
HWTEST_F(PasteDataEntryTest, Convert006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert006 start");
    PasteDataEntry entry;
    std::string value_str = "TestString";
    entry.SetValue(value_str);
    auto customData = entry.ConvertToCustomData();
    EXPECT_EQ(customData, nullptr);

    auto object = std::make_shared<Object>();
    object->value_[UDMF::HTML_CONTENT] = html_;
    entry.SetValue(object);
    customData = entry.ConvertToCustomData();
    EXPECT_EQ(customData, nullptr);

    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    object->value_[UDMF::UNIFORM_DATA_TYPE] = utdId;
    entry.SetValue(object);
    customData = entry.ConvertToCustomData();
    EXPECT_EQ(customData, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Convert006 end");
}

/**
 * @tc.name: EntriesTest001
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(PasteDataEntryTest, EntryTlvTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTlvTest001 start");
    PasteDataEntry entry;
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    entry.SetUtdId(utdId);
    entry.SetMimeType(MIMETYPE_TEXT_PLAIN);
    entry.SetValue(text_);

    std::vector<std::uint8_t> buffer;
    entry.Encode(buffer);
    PasteDataEntry decodePasteEntry;
    auto ret = decodePasteEntry.Decode(buffer);

    ASSERT_EQ(ret, true);
    ASSERT_EQ(decodePasteEntry.GetUtdId(), utdId);
    ASSERT_EQ(decodePasteEntry.GetMimeType(), MIMETYPE_TEXT_PLAIN);
    auto value = decodePasteEntry.GetValue();
    auto str = std::get_if<std::string>(&value);
    ASSERT_NE(str, nullptr);
    ASSERT_EQ(text_, *str);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTlvTest001 end");
}

/**
 * @tc.name: EntryTlvTest002
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(PasteDataEntryTest, EntryTlvTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTlvTest002 start");
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    auto entry = InitPlainTextEntry();
    std::vector<std::uint8_t> buffer;
    entry.Encode(buffer);
    PasteDataEntry decodePasteEntry;
    auto ret = decodePasteEntry.Decode(buffer);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(decodePasteEntry.GetUtdId(), utdId);
    ASSERT_EQ(decodePasteEntry.GetMimeType(), MIMETYPE_TEXT_PLAIN);
    CheckPlainUds(std::make_shared<PasteDataEntry>(decodePasteEntry));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTlvTest002 end");
}

/**
 * @tc.name: EntryTlvTest003
 * @tc.desc:
 * @tc.type: EntryTlvTest003 with pixelMap
 * @tc.require:
 * @tc.author:tarowang
 */
HWTEST_F(PasteDataEntryTest, EntryTlvTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTlvTest003 start");
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    auto entry = InitPixelMapEntry();
    std::vector<std::uint8_t> buffer;
    entry.Encode(buffer);
    PasteDataEntry decodePasteEntry;
    auto ret = decodePasteEntry.Decode(buffer);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(decodePasteEntry.GetUtdId(), utdId);
    ASSERT_EQ(decodePasteEntry.GetMimeType(), MIMETYPE_PIXELMAP);
    CheckPixelMapUds(std::make_shared<PasteDataEntry>(decodePasteEntry));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTlvTest003 end");
}

/**
 * @tc.name: EntryTest001
 * @tc.desc: Whether to include the target content
 * @tc.type: FUNC
 * @tc.require: entries
 * @tc.author:tarowang
 */
HWTEST_F(PasteDataEntryTest, EntryTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTest001 start");
    PasteDataEntry entry;
    auto utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
    EXPECT_EQ(entry.HasContent(utdId), false);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK);
    EXPECT_EQ(entry.HasContent(utdId), false);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
    EXPECT_EQ(entry.HasContent(utdId), false);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
    EXPECT_EQ(entry.HasContent(utdId), false);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    EXPECT_EQ(entry.HasContent(utdId), false);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::OPENHARMONY_WANT);
    EXPECT_EQ(entry.HasContent(utdId), false);

    utdId = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::ENTITY);
    EXPECT_EQ(entry.HasContent(utdId), false);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTest001 end");
}

/**
 * @tc.name: EntryTest002
 * @tc.desc: convert to string
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author:tarowang
 */
HWTEST_F(PasteDataEntryTest, EntryTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTest002 start");
    CommonUtils utils;
    UDType uDType = UDType::PLAIN_TEXT;
    std::string expect = MIMETYPE_TEXT_PLAIN;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::HYPERLINK;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::HTML;
    expect = MIMETYPE_TEXT_HTML;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::FILE;
    expect = MIMETYPE_TEXT_URI;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::IMAGE;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::VIDEO;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::AUDIO;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::FOLDER;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::FILE_URI;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::SYSTEM_DEFINED_PIXEL_MAP;
    expect = MIMETYPE_PIXELMAP;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::OPENHARMONY_WANT;
    expect = MIMETYPE_TEXT_WANT;
    EXPECT_EQ(utils.Convert(uDType), expect);

    uDType = UDType::ENTITY;
    EXPECT_EQ(utils.Convert(uDType), "general.entity");
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTest002 end");
}

/**
 * @tc.name: EntryTest003
 * @tc.desc: convert to UDType
 * @tc.type: FUNC
 * @tc.require:entries
 * @tc.author:tarowang
 */
HWTEST_F(PasteDataEntryTest, EntryTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTest003 start");
    CommonUtils utils;
    int32_t uDType = 1;
    std::string mimeType = MIMETYPE_TEXT_URI;
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::OBJECT);

    uDType = UDMF::UD_BUTT;
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::FILE_URI);

    mimeType = MIMETYPE_TEXT_PLAIN;
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::PLAIN_TEXT);

    mimeType = MIMETYPE_TEXT_HTML;
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::HTML);

    mimeType = MIMETYPE_TEXT_WANT;
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::OPENHARMONY_WANT);

    mimeType = MIMETYPE_PIXELMAP;
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::SYSTEM_DEFINED_PIXEL_MAP);

    mimeType = "general.entity";
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::ENTITY);

    mimeType = "test";
    EXPECT_EQ(utils.Convert(uDType, mimeType), UDMF::APPLICATION_DEFINED_RECORD);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "EntryTest003 end");
}
} // namespace OHOS::MiscServices
