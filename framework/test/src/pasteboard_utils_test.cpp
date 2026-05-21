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

#include "application_defined_record.h"
#include "audio.h"
#include "folder.h"
#include "html.h"
#include "image.h"
#include "link.h"
#include "pasteboard_utils.h"
#include "plain_text.h"
#include "pasteboard_hilog.h"
#include "pixel_map.h"
#include "system_defined_appitem.h"
#include "system_defined_form.h"
#include "system_defined_pixelmap.h"
#include "video.h"
namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
using UnifiedDataProperties = UDMF::UnifiedDataProperties;
class PasteboardUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    UDMF::UnifiedData InitTextData();
    UDMF::UnifiedData InitPlainData();
    UDMF::UnifiedData InitHtmlData();
    UDMF::UnifiedData InitWantData();
    UDMF::UnifiedData InitLinkData();
    UDMF::UnifiedData InitFileData();
    UDMF::UnifiedData InitImageData();
    UDMF::UnifiedData InitVideoData();
    UDMF::UnifiedData InitAudioData();
    UDMF::UnifiedData InitFolderData();
    UDMF::UnifiedData InitSystemRecordData();
    UDMF::UnifiedData InitSystemAppItemData();
    UDMF::UnifiedData InitSystemFormData();
    UDMF::UnifiedData InitSystemPixelMapData();
    UDMF::UnifiedData InitAppDefinedData();

protected:
    Details details_;
    std::vector<uint8_t> rawData_;
    std::string text_;
    std::string extraText_;
    std::string uri_;
};

void PasteboardUtilsTest::SetUpTestCase(void) { }

void PasteboardUtilsTest::TearDownTestCase(void) { }

void PasteboardUtilsTest::SetUp(void)
{
    rawData_ = { 1, 2, 3, 4, 5, 6, 7, 8 };
    details_.insert({ "keyString", "string_test" });
    details_.insert({ "keyInt32", 1 });
    details_.insert({ "keyBool", true });
    details_.insert({ "KeyU8Array", rawData_ });
    details_.insert({ "KeyDouble", 1.234 });
}

void PasteboardUtilsTest::TearDown(void) { }

UDMF::UnifiedData PasteboardUtilsTest::InitTextData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Text> textRecord = std::make_shared<UDMF::Text>();
    textRecord->SetDetails(details_);
    data.AddRecord(textRecord);
    return data;
}

UDMF::UnifiedData PasteboardUtilsTest::InitPlainData()
{
    text_ = "helloWorld_plainText";
    extraText_ = "helloWorld_plainAbstract";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::PlainText> plainTextRecord = std::make_shared<UDMF::PlainText>(text_, extraText_);
    plainTextRecord->SetDetails(details_);
    data.AddRecord(plainTextRecord);
    auto unifiedDataProperties = std::make_shared<UnifiedDataProperties>();
    unifiedDataProperties->isRemote = true;
    data.SetProperties(unifiedDataProperties);
    return data;
}

UDMF::UnifiedData PasteboardUtilsTest::InitHtmlData()
{
    text_ = "<div class='disable'>helloWorld</div>";
    extraText_ = "helloWorld_plainAbstract";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Html> htmlRecord = std::make_shared<UDMF::Html>(text_, extraText_);
    htmlRecord->SetDetails(details_);
    data.AddRecord(htmlRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitWantData()
{
    using namespace OHOS::AAFwk;
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string idKey = "id";
    int32_t idValue = 123;
    std::string deviceKey = "deviceId_key";
    want->SetParam(idKey, idValue);
    std::shared_ptr<UDMF::UnifiedRecord> wantRecord =
        std::make_shared<UDMF::UnifiedRecord>(UDMF::OPENHARMONY_WANT, want);
    UDMF::UnifiedData data;
    data.AddRecord(wantRecord);
    return data;
}

UDMF::UnifiedData PasteboardUtilsTest::InitLinkData()
{
    text_ = "https://www.test.com";
    extraText_ = "https://www.test.com/content";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Link> linkRecord = std::make_shared<UDMF::Link>(text_, extraText_);
    linkRecord->SetDetails(details_);
    data.AddRecord(linkRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitFileData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE);
    uri_ = "file:/uri";
    std::shared_ptr<UDMF::File> fileRecord = std::make_shared<UDMF::File>(uri_);
    fileRecord->SetDetails(details_);
    data.AddRecord(fileRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitImageData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::IMAGE);
    uri_ = "file:/image";
    std::shared_ptr<UDMF::Image> imageRecord = std::make_shared<UDMF::Image>(uri_);
    imageRecord->SetDetails(details_);
    data.AddRecord(imageRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitVideoData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::VIDEO);
    uri_ = "file:/Video";
    std::shared_ptr<UDMF::Video> videoRecord = std::make_shared<UDMF::Video>(uri_);
    videoRecord->SetDetails(details_);
    data.AddRecord(videoRecord);

    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitAudioData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::AUDIO);
    uri_ = "file:/Audio";
    std::shared_ptr<UDMF::Audio> audioRecord = std::make_shared<UDMF::Audio>(uri_);
    audioRecord->SetDetails(details_);
    data.AddRecord(audioRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitFolderData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FOLDER);
    uri_ = "file:/Folder";
    std::shared_ptr<UDMF::Folder> folderRecord = std::make_shared<UDMF::Folder>(uri_);
    folderRecord->SetDetails(details_);
    data.AddRecord(folderRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitSystemRecordData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::SystemDefinedRecord> systemRecord = std::make_shared<UDMF::SystemDefinedRecord>();
    systemRecord->SetDetails(details_);
    data.AddRecord(systemRecord);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitSystemAppItemData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::SystemDefinedAppItem> systemDefinedAppItem1 = std::make_shared<UDMF::SystemDefinedAppItem>();
    std::string appId = "appId";
    std::string appIconId = "appIconId";
    std::string appName = "appName";
    std::string appLabelId = "appLabelId";
    std::string bundleName = "bundleName";
    std::string abilityName = "abilityName";
    systemDefinedAppItem1->SetDetails(details_);
    systemDefinedAppItem1->SetAppId(appId);
    systemDefinedAppItem1->SetAppName(appName);
    systemDefinedAppItem1->SetAppIconId(appIconId);
    systemDefinedAppItem1->SetAppLabelId(appLabelId);
    systemDefinedAppItem1->SetBundleName(bundleName);
    systemDefinedAppItem1->SetAbilityName(abilityName);
    systemDefinedAppItem1->SetType(UDMF::SYSTEM_DEFINED_APP_ITEM);
    data.AddRecord(systemDefinedAppItem1);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitSystemFormData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::SystemDefinedForm> form = std::make_shared<UDMF::SystemDefinedForm>();
    int32_t formId = 1;
    std::string formName = "formName";
    std::string module = "module";
    std::string bundleName = "bundleName";
    std::string abilityName = "abilityName";
    form->SetDetails(details_);
    form->SetFormId(formId);
    form->SetFormName(formName);
    form->SetAbilityName(abilityName);
    form->SetBundleName(bundleName);
    form->SetModule(module);
    form->SetType(UDMF::SYSTEM_DEFINED_FORM);
    data.AddRecord(form);
    return data;
}
UDMF::UnifiedData PasteboardUtilsTest::InitSystemPixelMapData()
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    std::shared_ptr<UDMF::UnifiedRecord> pixelMapRecord =
        std::make_shared<UDMF::SystemDefinedPixelMap>(UDMF::SYSTEM_DEFINED_PIXEL_MAP, pixelMapIn);
    UDMF::UnifiedData data;
    data.AddRecord(pixelMapRecord);
    return data;
}

UDMF::UnifiedData PasteboardUtilsTest::InitAppDefinedData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::ApplicationDefinedRecord> appRecord = std::make_shared<UDMF::ApplicationDefinedRecord>();
    std::map<std::string, std::vector<uint8_t>> customData;
    customData[UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::APPLICATION_DEFINED_RECORD)] = rawData_;
    appRecord->SetApplicationDefinedType(UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::APPLICATION_DEFINED_RECORD));
    appRecord->SetRawData(rawData_);
    data.AddRecord(appRecord);
    return data;
}

/**
 * @tc.name: Text2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Text2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Text2PasteRecord001 start");
    auto data = InitTextData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::TEXT));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::TEXT);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::TEXT);
    auto newPlainRecord = static_cast<UDMF::Text *>(newRecord.get());
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Text2PasteRecord001 end");
}

/**
 * @tc.name: Text2PasteRecord002
 * @tc.desc: textRecord is nullptr
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Text2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Text2PasteRecord002 start");
    std::shared_ptr<UDMF::Text> textRecord;
    auto pasteRecord = PasteboardUtils::Text2PasteRecord(textRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Text(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Text2PasteRecord002 end");
}

/**
 * @tc.name: PlainText2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, PlainText2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PlainText2PasteRecord001 start");
    auto data = InitPlainData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::PLAIN_TEXT);
    auto entries = record->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto link = std::make_shared<UDMF::PlainText>(UDMF::PLAIN_TEXT, entryValue);
    ASSERT_EQ(link->GetContent(), text_);
    ASSERT_EQ(link->GetAbstract(), extraText_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::PLAIN_TEXT);
    auto newPlainRecord = static_cast<UDMF::PlainText *>(newRecord.get());
    auto newPlainText = newPlainRecord->GetContent();
    auto newAbstract = newPlainRecord->GetAbstract();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newPlainText, text_);
    ASSERT_EQ(newAbstract, extraText_);
    ASSERT_EQ(newDetails, details_);
    auto unifiedProp = newData->GetProperties();
    ASSERT_EQ(unifiedProp->isRemote, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PlainText2PasteRecord001 end");
}

/**
 * @tc.name: PlainText2PasteRecord002
 * @tc.desc: plainTextRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, PlainText2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PlainText2PasteRecord002 start");
    std::shared_ptr<UDMF::PlainText> plainTextRecord;
    auto pasteRecord = PasteboardUtils::PlainText2PasteRecord(plainTextRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2PlaintText(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PlainText2PasteRecord002 end");
}

/**
 * @tc.name: Html2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Html2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Html2PasteRecord001 start");
    auto data = InitHtmlData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_HTML);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::HTML);
    auto entries = record->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto link = std::make_shared<UDMF::Html>(UDMF::HTML, entryValue);
    ASSERT_EQ(link->GetHtmlContent(), text_);
    ASSERT_EQ(link->GetPlainContent(), extraText_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::HTML);
    auto newPlainRecord = static_cast<UDMF::Html *>(newRecord.get());
    auto newPlainText = newPlainRecord->GetHtmlContent();
    auto newAbstract = newPlainRecord->GetPlainContent();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newPlainText, text_);
    ASSERT_EQ(newAbstract, extraText_);
    ASSERT_EQ(newDetails, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Html2PasteRecord001 end");
}

/**
 * @tc.name: Html2PasteRecord002
 * @tc.desc: htmlRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Html2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Html2PasteRecord002 start");
    std::shared_ptr<UDMF::Html> htmlRecord;
    auto pasteRecord = PasteboardUtils::Html2PasteRecord(htmlRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Html(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Html2PasteRecord002 end");
}

/**
 * @tc.name: Link2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Link2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Link2PasteRecord001 start");
    auto data = InitLinkData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::HYPERLINK);
    auto entries = record->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto link = std::make_shared<UDMF::Link>(UDMF::HYPERLINK, entryValue);
    ASSERT_EQ(link->GetUrl(), text_);
    ASSERT_EQ(link->GetDescription(), extraText_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::HYPERLINK);
    auto newPlainRecord = static_cast<UDMF::Link *>(newRecord.get());
    auto newUrl = newPlainRecord->GetUrl();
    auto newDescription = newPlainRecord->GetDescription();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newUrl, text_);
    ASSERT_EQ(newDescription, extraText_);
    ASSERT_EQ(newDetails, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Link2PasteRecord001 end");
}

/**
 * @tc.name: Link2PasteRecord002
 * @tc.desc: linkRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Link2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Link2PasteRecord002 start");
    std::shared_ptr<UDMF::Link> linkRecord;
    auto pasteRecord = PasteboardUtils::Link2PasteRecord(linkRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Link(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Link2PasteRecord002 end");
}

/**
 * @tc.name: Want2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Want2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Want2PasteRecord001 start");
    auto data = InitWantData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_WANT);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, int32_t(UDMF::UDType::OPENHARMONY_WANT));
    auto want1 = record->GetWant();
    int32_t idValue1 = want1->GetIntParam("id", 0);
    ASSERT_EQ(idValue1, 123);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::OPENHARMONY_WANT);
    auto recordValue = newRecord->GetValue();
    auto wantValue = std::get_if<std::shared_ptr<OHOS::AAFwk::Want>>(&recordValue);
    ASSERT_NE(wantValue, nullptr);
    int32_t idValue2 = (*(wantValue))->GetIntParam("id", 0);
    ASSERT_EQ(idValue2, 123);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Want2PasteRecord001 end");
}

/**
 * @tc.name: Want2PasteRecord002
 * @tc.desc: wantRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Want2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Want2PasteRecord002 start");
    std::shared_ptr<UDMF::UnifiedRecord> wantRecord;
    auto pasteRecord = PasteboardUtils::Want2PasteRecord(wantRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Want(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Want2PasteRecord002 end");
}

/**
 * @tc.name: File2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, File2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "File2PasteRecord001 start");
    auto data = InitFileData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::FILE);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::FILE);
    auto newFileRecord = static_cast<UDMF::File *>(newRecord.get());
    auto newDetails = newFileRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newFileRecord->GetUri();
    ASSERT_EQ(uri2, uri_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "File2PasteRecord001 end");
}

/**
 * @tc.name: File2PasteRecord002
 * @tc.desc: fileRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, File2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "File2PasteRecord002 start");
    std::shared_ptr<UDMF::File> fileRecord;
    auto pasteRecord = PasteboardUtils::File2PasteRecord(fileRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2File(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "File2PasteRecord002 end");
}

/**
 * @tc.name: Image2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Image2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Image2PasteRecord001 start");
    auto data = InitImageData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::IMAGE);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::IMAGE);
    auto newImageRecord = static_cast<UDMF::Image *>(newRecord.get());
    auto newDetails = newImageRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newImageRecord->GetUri();
    ASSERT_EQ(uri2, uri_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Image2PasteRecord001 end");
}

/**
 * @tc.name: Image2PasteRecord002
 * @tc.desc: imageRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Image2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Image2PasteRecord002 start");
    std::shared_ptr<UDMF::Image> imageRecord;
    auto pasteRecord = PasteboardUtils::Image2PasteRecord(imageRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Image(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Image2PasteRecord002 end");
}

/**
 * @tc.name: Audio2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Audio2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Audio2PasteRecord001 start");
    auto data = InitAudioData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::AUDIO);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::AUDIO);
    auto newAudioRecord = static_cast<UDMF::Audio *>(newRecord.get());
    auto newDetails = newAudioRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newAudioRecord->GetUri();
    ASSERT_EQ(uri2, uri_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Audio2PasteRecord001 end");
}

/**
 * @tc.name: Audio2PasteRecord002
 * @tc.desc: audioRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Audio2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Audio2PasteRecord002 start");
    std::shared_ptr<UDMF::Audio> audioRecord;
    auto pasteRecord = PasteboardUtils::Audio2PasteRecord(audioRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Audio(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Audio2PasteRecord002 end");
}

/**
 * @tc.name: Video2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Video2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Video2PasteRecord001 start");
    auto data = InitVideoData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::VIDEO);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::VIDEO);
    auto newVideoRecord = static_cast<UDMF::Video *>(newRecord.get());
    auto newDetails = newVideoRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newVideoRecord->GetUri();
    ASSERT_EQ(uri2, uri_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Video2PasteRecord001 end");
}

/**
 * @tc.name: Video2PasteRecord002
 * @tc.desc: videoRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Video2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Video2PasteRecord002 start");
    std::shared_ptr<UDMF::Video> videoRecord;
    auto pasteRecord = PasteboardUtils::Video2PasteRecord(videoRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Video(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Video2PasteRecord002 end");
}

/**
 * @tc.name: Folder2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Folder2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Folder2PasteRecord001 start");
    auto data = InitFolderData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::FOLDER);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::FOLDER);
    auto newFolderRecord = static_cast<UDMF::Folder *>(newRecord.get());
    auto newDetails = newFolderRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newFolderRecord->GetUri();
    ASSERT_EQ(uri2, uri_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Folder2PasteRecord001 end");
}

/**
 * @tc.name: Folder2PasteRecord002
 * @tc.desc: folderRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Folder2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Folder2PasteRecord002 start");
    std::shared_ptr<UDMF::Folder> folderRecord;
    auto pasteRecord = PasteboardUtils::Folder2PasteRecord(folderRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Folder(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Folder2PasteRecord002 end");
}

/**
 * @tc.name: SystemDefined2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, SystemDefined2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SystemDefined2PasteRecord001 start");
    auto data = InitSystemRecordData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_RECORD));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::SYSTEM_DEFINED_RECORD);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_RECORD);
    auto newSystemRecord = static_cast<UDMF::SystemDefinedRecord *>(newRecord.get());
    ASSERT_EQ(newSystemRecord->GetDetails(), details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SystemDefined2PasteRecord001 end");
}

/**
 * @tc.name: SystemDefined2PasteRecord002
 * @tc.desc: systemRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, SystemDefined2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SystemDefined2PasteRecord002 start");
    std::shared_ptr<UDMF::SystemDefinedRecord> systemRecord;
    auto pasteRecord = PasteboardUtils::SystemDefined2PasteRecord(systemRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2SystemDefined(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SystemDefined2PasteRecord002 end");
}

/**
 * @tc.name: AppItem2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, AppItem2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppItem2PasteRecord001 start");
    auto data = InitSystemAppItemData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_APP_ITEM));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::SYSTEM_DEFINED_APP_ITEM);
    auto details1 = record->GetDetails();
    ASSERT_NE(details1, nullptr);
    ASSERT_EQ(*details1, details_);
    auto entries = record->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto newAppItem1 = std::make_shared<UDMF::SystemDefinedAppItem>(UDMF::SYSTEM_DEFINED_APP_ITEM, entryValue);
    ASSERT_EQ(newAppItem1->GetAppId(), "appId");
    ASSERT_EQ(newAppItem1->GetAppIconId(), "appIconId");
    ASSERT_EQ(newAppItem1->GetAppName(), "appName");
    ASSERT_EQ(newAppItem1->GetAppLabelId(), "appLabelId");
    ASSERT_EQ(newAppItem1->GetBundleName(), "bundleName");
    ASSERT_EQ(newAppItem1->GetAbilityName(), "abilityName");

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_APP_ITEM);
    auto newAppItem = static_cast<UDMF::SystemDefinedAppItem *>(newRecord.get());
    ASSERT_EQ(newAppItem->GetAppId(), "appId");
    ASSERT_EQ(newAppItem->GetAppIconId(), "appIconId");
    ASSERT_EQ(newAppItem->GetAppName(), "appName");
    ASSERT_EQ(newAppItem->GetAppLabelId(), "appLabelId");
    ASSERT_EQ(newAppItem->GetBundleName(), "bundleName");
    ASSERT_EQ(newAppItem->GetAbilityName(), "abilityName");
    ASSERT_EQ(newAppItem->GetDetails(), details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppItem2PasteRecord001 end");
}

/**
 * @tc.name: AppItem2PasteRecord002
 * @tc.desc: systemDefinedAppItem is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, AppItem2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppItem2PasteRecord002 start");
    std::shared_ptr<UDMF::SystemDefinedAppItem> systemDefinedAppItem;
    auto pasteRecord = PasteboardUtils::AppItem2PasteRecord(systemDefinedAppItem);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2AppItem(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppItem2PasteRecord002 end");
}

/**
 * @tc.name: Form2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Form2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Form2PasteRecord001 start");
    auto data = InitSystemFormData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_FORM));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::SYSTEM_DEFINED_FORM);
    auto details1 = record->GetDetails();
    auto content = *(record->GetSystemDefinedContent());
    ASSERT_EQ(*details1, details_);
    auto formId1 = std::get<std::int32_t>(content["formId"]);
    auto formName1 = std::get<std::string>(content["formName"]);
    auto module1 = std::get<std::string>(content["module"]);
    auto bundleName1 = std::get<std::string>(content["bundleName"]);
    auto abilityName1 = std::get<std::string>(content["abilityName"]);
    ASSERT_EQ(1, formId1);
    ASSERT_EQ("formName", formName1);
    ASSERT_EQ("module", module1);
    ASSERT_EQ("bundleName", bundleName1);
    ASSERT_EQ("abilityName", abilityName1);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_FORM);
    auto newForm = static_cast<UDMF::SystemDefinedForm *>(newRecord.get());
    ASSERT_EQ(newForm->GetFormId(), formId1);
    ASSERT_EQ(newForm->GetFormName(), formName1);
    ASSERT_EQ(newForm->GetModule(), module1);
    ASSERT_EQ(newForm->GetBundleName(), bundleName1);
    ASSERT_EQ(newForm->GetAbilityName(), abilityName1);
    ASSERT_EQ(newForm->GetDetails(), details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Form2PasteRecord001 end");
}

/**
 * @tc.name: Form2PasteRecord002
 * @tc.desc: form is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, Form2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Form2PasteRecord002 start");
    std::shared_ptr<UDMF::SystemDefinedForm> form;
    auto pasteRecord = PasteboardUtils::Form2PasteRecord(form);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2Form(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "Form2PasteRecord002 end");
}

/**
 * @tc.name: PixelMap2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, PixelMap2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PixelMap2PasteRecord001 start");
    auto data = InitSystemPixelMapData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_PIXELMAP);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, int32_t(UDMF::UDType::SYSTEM_DEFINED_PIXEL_MAP));
    auto newPixelMap = record->GetPixelMapV0();
    ASSERT_TRUE(newPixelMap != nullptr);
    ImageInfo imageInfo = {};
    newPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == 7);
    ASSERT_TRUE(imageInfo.size.width == 5);
    ASSERT_TRUE(imageInfo.pixelFormat == PixelFormat::ARGB_8888);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    auto recordValue = newRecord->GetValue();
    auto newPixelMap1 = std::get_if<std::shared_ptr<OHOS::Media::PixelMap>>(&recordValue);
    ASSERT_NE(newPixelMap1, nullptr);
    ImageInfo imageInfo1 = {};
    (*newPixelMap1)->GetImageInfo(imageInfo1);
    ASSERT_TRUE(imageInfo1.size.height == imageInfo.size.height);
    ASSERT_TRUE(imageInfo1.size.width == imageInfo.size.width);
    ASSERT_TRUE(imageInfo1.pixelFormat == imageInfo.pixelFormat);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PixelMap2PasteRecord001 end");
}

/**
 * @tc.name: PixelMap2PasteRecord002
 * @tc.desc: pixelMapRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, PixelMap2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PixelMap2PasteRecord002 start");
    std::shared_ptr<UDMF::UnifiedRecord> pixelMapRecord;
    auto pasteRecord = PasteboardUtils::PixelMap2PasteRecord(pixelMapRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::PasteRecord2PixelMap(pasteRecord);
    ASSERT_EQ(ret, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "PixelMap2PasteRecord002 end");
}

/**
 * @tc.name: AppDefined2PasteRecord001
 * @tc.desc: pasteData is local data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, AppDefined2PasteRecord001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppDefined2PasteRecord001 start");
    auto data = InitAppDefinedData();
    auto pasteData = PasteboardUtils::GetInstance().Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::APPLICATION_DEFINED_RECORD));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::APPLICATION_DEFINED_RECORD);
    auto items = record->GetCustomData()->GetItemData();
    std::map<std::string, std::vector<uint8_t>> customData {
        {type, rawData_}
    };
    ASSERT_EQ(items, customData);

    auto newData = PasteboardUtils::GetInstance().Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::APPLICATION_DEFINED_RECORD);
    auto newSystemRecord = static_cast<UDMF::ApplicationDefinedRecord *>(newRecord.get());
    ASSERT_EQ(newSystemRecord->GetRawData(), rawData_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppDefined2PasteRecord001 end");
}

/**
 * @tc.name: AppDefined2PasteRecord002
 * @tc.desc: appRecord is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, AppDefined2PasteRecord002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppDefined2PasteRecord002 start");
    std::shared_ptr<UDMF::ApplicationDefinedRecord> appRecord;
    auto pasteRecord = PasteboardUtils::AppDefined2PasteRecord(appRecord);
    ASSERT_EQ(pasteRecord, nullptr);

    auto ret = PasteboardUtils::Custom2AppDefined(pasteRecord);
    ASSERT_EQ(ret.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "AppDefined2PasteRecord002 end");
}

/**
 * @tc.name: ConvertTest001
 * @tc.desc: Traverse UDType.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, ConvertTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ConvertTest001 start");
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UDType::PLAIN_TEXT), MIMETYPE_TEXT_PLAIN);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UDType::HTML), MIMETYPE_TEXT_HTML);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UDType::FILE_URI), MIMETYPE_TEXT_URI);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UDType::SYSTEM_DEFINED_PIXEL_MAP), MIMETYPE_PIXELMAP);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UDType::OPENHARMONY_WANT), MIMETYPE_TEXT_WANT);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ConvertTest001 end");
}

/**
 * @tc.name: ConvertTest002
 * @tc.desc: Traverse MIMEType.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUtilsTest, ConvertTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ConvertTest002 start");
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, MIMETYPE_TEXT_URI), UDMF::FILE);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, MIMETYPE_TEXT_PLAIN), UDMF::PLAIN_TEXT);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, MIMETYPE_TEXT_HTML), UDMF::HTML);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, MIMETYPE_TEXT_WANT), UDMF::OPENHARMONY_WANT);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, MIMETYPE_PIXELMAP), UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, text_), UDMF::UD_BUTT);
    std::string mimeType = "general.jpeg-2000";
    ASSERT_EQ(PasteboardUtils::Convert(UDMF::UD_BUTT, mimeType), UDMF::JPEG2000);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ConvertTest002 end");
}

/**
 * @tc.name: ConvertShareOptionTest001
 * @tc.desc: Test Convert function when properties.shareOptions is UDMF::IN_APP
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest001 start");
    UDMF::UnifiedDataProperties properties;
    PasteDataProperty result;

    properties.shareOptions = UDMF::IN_APP;
    result.shareOption = LocalDevice;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, InApp);
    result.shareOption = CrossDevice;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, InApp);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest001 end");
}

/**
 * @tc.name: ConvertShareOptionTest002
 * @tc.desc: Test Convert function when properties.shareOptions is UDMF::CROSS_APP
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest002 start");
    UDMF::UnifiedDataProperties properties;
    PasteDataProperty result;

    properties.shareOptions = UDMF::CROSS_APP;
    result.shareOption = InApp;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, LocalDevice);
    result.shareOption = CrossDevice;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, LocalDevice);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest002 end");
}

/**
 * @tc.name: ConvertShareOptionTest003
 * @tc.desc: Test Convert function when properties.shareOptions is UDMF::CROSS_DEVICE
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest003 start");
    UDMF::UnifiedDataProperties properties;
    PasteDataProperty result;

    properties.shareOptions = UDMF::CROSS_DEVICE;
    result.shareOption = InApp;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, CrossDevice);
    result.shareOption = LocalDevice;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, CrossDevice);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest003 end");
}

/**
 * @tc.name: ConvertShareOptionTest004
 * @tc.desc: Test Convert function when properties.shareOptions is UDMF::SHARE_OPTIONS_BUTT
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest004, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest004 start");
    UDMF::UnifiedDataProperties properties;
    PasteDataProperty result;

    properties.shareOptions = UDMF::SHARE_OPTIONS_BUTT;
    result.shareOption = InApp;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, CrossDevice);
    result.shareOption = LocalDevice;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result.shareOption, CrossDevice);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest004 end");
}

/**
 * @tc.name: ConvertShareOptionTest005
 * @tc.desc: Test Convert function when properties.shareOption is InApp
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest005, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest005 start");
    PasteDataProperty properties;
    std::shared_ptr<UDMF::UnifiedDataProperties> result = std::make_shared<UDMF::UnifiedDataProperties>();

    properties.shareOption = InApp;
    result->shareOptions = UDMF::CROSS_APP;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result->shareOptions, UDMF::IN_APP);
    result->shareOptions = UDMF::CROSS_DEVICE;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result->shareOptions, UDMF::IN_APP);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest005 end");
}

/**
 * @tc.name: ConvertShareOptionTest006
 * @tc.desc: Test Convert function when properties.shareOption is LocalDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest006, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest006 start");
    PasteDataProperty properties;
    std::shared_ptr<UDMF::UnifiedDataProperties> result = std::make_shared<UDMF::UnifiedDataProperties>();

    properties.shareOption = LocalDevice;
    result->shareOptions = UDMF::IN_APP;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_APP);
    result->shareOptions = UDMF::CROSS_DEVICE;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_APP);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest006 end");
}

/**
 * @tc.name: ConvertShareOptionTest007
 * @tc.desc: Test Convert function when properties.shareOption is CrossDevice
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, ConvertShareOptionTest007, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest007 start");
    PasteDataProperty properties;
    std::shared_ptr<UDMF::UnifiedDataProperties> result = std::make_shared<UDMF::UnifiedDataProperties>();

    properties.shareOption = CrossDevice;
    result->shareOptions = UDMF::IN_APP;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_DEVICE);
    result->shareOptions = UDMF::CROSS_APP;
    result = PasteboardUtils::Convert(properties);
    EXPECT_EQ(result->shareOptions, UDMF::CROSS_DEVICE);

    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "ConvertShareOptionTest007 end");
}

/**
 * @tc.name: DeduplicateVectorTest
 * @tc.desc: Remove duplicate value from string vector
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(PasteboardUtilsTest, DeduplicateVectorTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DeduplicateVectorTest start");
    const std::vector<std::string> vec1(3, "test");
    const std::vector<std::string> vec2(1, "test");
    std::vector<std::string> result = PasteboardUtils::GetInstance().DeduplicateVector(vec1);
    EXPECT_EQ(std::equal(result.begin(), result.end(), vec2.begin()), true);
    const std::vector<std::string> vec3 = {"a", "b", "c", "a", "b", "c"};
    std::vector<std::string> vec4 = {"a", "b", "c"};
    result = PasteboardUtils::GetInstance().DeduplicateVector(vec3);
    EXPECT_EQ(result.size(), vec4.size());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "DeduplicateVectorTest end");
}

} // namespace OHOS::MiscServices
