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
#include "pasteboard_client.h"
#include "pixel_map.h"
#include "plain_text.h"
#include "system_defined_appitem.h"
#include "system_defined_form.h"
#include "system_defined_pixelmap.h"
#include "video.h"
#include "pasteboard_hilog.h"
namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
class PasteboardUnifiedDataTest : public testing::Test {
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

void PasteboardUnifiedDataTest::SetUpTestCase(void) { }

void PasteboardUnifiedDataTest::TearDownTestCase(void) { }

void PasteboardUnifiedDataTest::SetUp(void)
{
    rawData_ = { 1, 2, 3, 4, 5, 6, 7, 8 };
    details_.insert({ "keyString", "string_test" });
    details_.insert({ "keyInt32", 1 });
    details_.insert({ "keyBool", true });
    details_.insert({ "KeyU8Array", rawData_ });
    details_.insert({ "KeyDouble", 1.234 });
}

void PasteboardUnifiedDataTest::TearDown(void) { }

UDMF::UnifiedData PasteboardUnifiedDataTest::InitTextData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Text> textRecord = std::make_shared<UDMF::Text>();
    textRecord->SetDetails(details_);
    data.AddRecord(textRecord);
    return data;
}

UDMF::UnifiedData PasteboardUnifiedDataTest::InitPlainData()
{
    text_ = "helloWorld_plainText";
    extraText_ = "helloWorld_plainAbstract";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::PlainText> plainTextRecord = std::make_shared<UDMF::PlainText>(text_, extraText_);
    plainTextRecord->SetDetails(details_);
    data.AddRecord(plainTextRecord);
    return data;
}

UDMF::UnifiedData PasteboardUnifiedDataTest::InitHtmlData()
{
    text_ = "<div class='disable'>helloWorld</div>";
    extraText_ = "helloWorld_plainAbstract";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Html> htmlRecord = std::make_shared<UDMF::Html>(text_, extraText_);
    htmlRecord->SetDetails(details_);
    data.AddRecord(htmlRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitWantData()
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

UDMF::UnifiedData PasteboardUnifiedDataTest::InitLinkData()
{
    text_ = "https://www.test.com";
    extraText_ = "https://www.test.com/content";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Link> linkRecord = std::make_shared<UDMF::Link>(text_, extraText_);
    linkRecord->SetDetails(details_);
    data.AddRecord(linkRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitFileData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE);
    uri_ = "file://uri";
    std::shared_ptr<UDMF::File> fileRecord = std::make_shared<UDMF::File>(uri_);
    fileRecord->SetDetails(details_);
    data.AddRecord(fileRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitImageData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::IMAGE);
    uri_ = "file://image";
    std::shared_ptr<UDMF::Image> imageRecord = std::make_shared<UDMF::Image>(uri_);
    imageRecord->SetDetails(details_);
    data.AddRecord(imageRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitVideoData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::VIDEO);
    uri_ = "file://Video";
    std::shared_ptr<UDMF::Video> videoRecord = std::make_shared<UDMF::Video>(uri_);
    videoRecord->SetDetails(details_);
    data.AddRecord(videoRecord);

    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitAudioData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::AUDIO);
    uri_ = "file://Audio";
    std::shared_ptr<UDMF::Audio> audioRecord = std::make_shared<UDMF::Audio>(uri_);
    audioRecord->SetDetails(details_);
    data.AddRecord(audioRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitFolderData()
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FOLDER);
    uri_ = "file://Folder";
    std::shared_ptr<UDMF::Folder> folderRecord = std::make_shared<UDMF::Folder>(uri_);
    folderRecord->SetDetails(details_);
    data.AddRecord(folderRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitSystemRecordData()
{
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::SystemDefinedRecord> systemRecord = std::make_shared<UDMF::SystemDefinedRecord>();
    systemRecord->SetDetails(details_);
    data.AddRecord(systemRecord);
    return data;
}
UDMF::UnifiedData PasteboardUnifiedDataTest::InitSystemAppItemData()
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
UDMF::UnifiedData PasteboardUnifiedDataTest::InitSystemFormData()
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
UDMF::UnifiedData PasteboardUnifiedDataTest::InitSystemPixelMapData()
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

UDMF::UnifiedData PasteboardUnifiedDataTest::InitAppDefinedData()
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
 * @tc.name: SetText001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetText001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetText001 start");
    auto data = InitTextData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::TEXT);
    auto newPlainRecord = static_cast<UDMF::Text *>(newRecord.get());
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::TEXT));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::TEXT);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetText001 end");
}

/**
 * @tc.name: SetPlainText001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetPlainText001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPlainText001 start");
    auto data = InitPlainData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::PLAIN_TEXT);
    auto newPlainRecord = static_cast<UDMF::PlainText *>(newRecord.get());
    auto newPlainText = newPlainRecord->GetContent();
    auto newAbstract = newPlainRecord->GetAbstract();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newPlainText, text_);
    ASSERT_EQ(newAbstract, extraText_);
    ASSERT_EQ(newDetails, details_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::PLAIN_TEXT);
    auto entries = record->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto plainText = std::make_shared<UDMF::PlainText>(UDMF::PLAIN_TEXT, entryValue);
    ASSERT_EQ(plainText->GetContent(), text_);
    ASSERT_EQ(plainText->GetAbstract(), extraText_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPlainText001 end");
}

/**
 * @tc.name: SetLink001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetLink001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetLink001 start");
    auto data = InitLinkData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::HYPERLINK);
    auto newPlainRecord = static_cast<UDMF::Link *>(newRecord.get());
    auto newUrl = newPlainRecord->GetUrl();
    auto newDescription = newPlainRecord->GetDescription();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newUrl, text_);
    ASSERT_EQ(newDescription, extraText_);
    ASSERT_EQ(newDetails, details_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetLink001 end");
}

/**
 * @tc.name: SetHtml001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetHtml001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetHtml001 start");
    auto data = InitHtmlData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::HTML);
    auto newPlainRecord = static_cast<UDMF::Html *>(newRecord.get());
    auto newPlainText = newPlainRecord->GetHtmlContent();
    auto newAbstract = newPlainRecord->GetPlainContent();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newPlainText, text_);
    ASSERT_EQ(newAbstract, extraText_);
    ASSERT_EQ(newDetails, details_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_HTML);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::HTML);
    auto entries = record->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto html = std::make_shared<UDMF::Html>(UDMF::HTML, entryValue);
    ASSERT_EQ(html->GetHtmlContent(), text_);
    ASSERT_EQ(html->GetPlainContent(), extraText_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetHtml001 end");
}

/**
 * @tc.name: SetWant001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetWant001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWant001 start");
    auto data = InitWantData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::OPENHARMONY_WANT);
    auto recordValue = newRecord->GetValue();
    auto wantValue = std::get_if<std::shared_ptr<OHOS::AAFwk::Want>>(&recordValue);
    ASSERT_NE(wantValue, nullptr);
    int32_t idValue2 = (*(wantValue))->GetIntParam("id", 0);
    ASSERT_EQ(idValue2, 123);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_WANT);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, int32_t(UDMF::UDType::OPENHARMONY_WANT));
    auto want1 = record->GetWant();
    int32_t idValue1 = want1->GetIntParam("id", 0);
    ASSERT_EQ(idValue1, 123);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetWant001 end");
}

/**
 * @tc.name: SetFile001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetFile001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetFile001 start");
    auto data = InitFileData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);

    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::FILE);
    auto newFileRecord = static_cast<UDMF::File *>(newRecord.get());
    auto newDetails = newFileRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newFileRecord->GetUri();
    ASSERT_EQ(uri2, uri_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::FILE);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetFile001 end");
}

/**
 * @tc.name: SetImage001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetImage001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetImage001 start");
    auto data = InitImageData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::IMAGE);
    auto newImageRecord = static_cast<UDMF::Image *>(newRecord.get());
    auto newDetails = newImageRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newImageRecord->GetUri();
    ASSERT_EQ(uri2, uri_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::IMAGE);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetImage001 end");
}

/**
 * @tc.name: SetAudio001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetAudio001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetAudio001 start");
    auto data = InitAudioData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::AUDIO);
    auto newAudioRecord = static_cast<UDMF::Audio *>(newRecord.get());
    auto newDetails = newAudioRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newAudioRecord->GetUri();
    ASSERT_EQ(uri2, uri_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::AUDIO);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetAudio001 end");
}

/**
 * @tc.name: SetVideo001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetVideo001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetVideo001 start");
    auto data = InitVideoData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::VIDEO);
    auto newVideoRecord = static_cast<UDMF::Video *>(newRecord.get());
    auto newDetails = newVideoRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newVideoRecord->GetUri();
    ASSERT_EQ(uri2, uri_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::VIDEO);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetVideo001 end");
}

/**
 * @tc.name: SetFolder001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetFolder001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetFolder001 start");
    auto data = InitFolderData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::FOLDER);
    auto newFolderRecord = static_cast<UDMF::Folder *>(newRecord.get());
    auto newDetails = newFolderRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newFolderRecord->GetUri();
    ASSERT_EQ(uri2, uri_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::FOLDER);
    auto uri1 = record->GetUriV0()->ToString();
    ASSERT_EQ(uri1, uri_);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetFolder001 end");
}

/**
 * @tc.name: SetSystemDefined001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetSystemDefined001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetSystemDefined001 start");
    auto data = InitSystemRecordData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_RECORD);
    auto newSystemRecord = static_cast<UDMF::SystemDefinedRecord *>(newRecord.get());
    ASSERT_EQ(newSystemRecord->GetDetails(), details_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_RECORD));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::SYSTEM_DEFINED_RECORD);
    auto details = record->GetDetails();
    ASSERT_EQ(*details, details_);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetSystemDefined001 end");
}

/**
 * @tc.name: SetAppItem001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetAppItem001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetAppItem001 start");
    auto data = InitSystemAppItemData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);

    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
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

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetAppItem001 end");
}

/**
 * @tc.name: SetForm001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetForm001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetForm001 start");
    auto data = InitSystemFormData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_FORM);
    auto newForm = static_cast<UDMF::SystemDefinedForm *>(newRecord.get());
    ASSERT_EQ(newForm->GetFormId(), 1);
    ASSERT_EQ(newForm->GetFormName(), "formName");
    ASSERT_EQ(newForm->GetModule(), "module");
    ASSERT_EQ(newForm->GetBundleName(), "bundleName");
    ASSERT_EQ(newForm->GetAbilityName(), "abilityName");
    ASSERT_EQ(newForm->GetDetails(), details_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
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
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetForm001 end");
}

/**
 * @tc.name: SetAppDefined001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetAppDefined001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetAppDefined001 start");
    auto data = InitAppDefinedData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);

    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::APPLICATION_DEFINED_RECORD);
    auto newSystemRecord = static_cast<UDMF::ApplicationDefinedRecord *>(newRecord.get());
    ASSERT_EQ(newSystemRecord->GetRawData(), rawData_);

    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::APPLICATION_DEFINED_RECORD));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::APPLICATION_DEFINED_RECORD);
    auto items = record->GetCustomData()->GetItemData();
    std::map<std::string, std::vector<uint8_t>> customData {
        {type, rawData_}
    };
    ASSERT_EQ(items, customData);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetAppDefined001 end");
}

/**
 * @tc.name: SetPixelMap001
 * @tc.desc: Get the source of the data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardUnifiedDataTest, SetPixelMap001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPixelMap001 start");
    auto data = InitSystemPixelMapData();
    PasteboardClient::GetInstance()->SetUnifiedData(data);
    // GetUnifiedData
    UDMF::UnifiedData newData;
    PasteboardClient::GetInstance()->GetUnifiedData(newData);
    ASSERT_EQ(1, newData.GetRecords().size());
    auto newRecord = newData.GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    auto recordValue = newRecord->GetValue();
    auto newPixelMap1 = std::get_if<std::shared_ptr<OHOS::Media::PixelMap>>(&recordValue);
    ASSERT_NE(newPixelMap1, nullptr);
    ImageInfo imageInfo1 = {};
    (*newPixelMap1)->GetImageInfo(imageInfo1);
    ASSERT_TRUE(imageInfo1.size.height == 7);
    ASSERT_TRUE(imageInfo1.size.width == 5);
    ASSERT_TRUE(imageInfo1.pixelFormat == PixelFormat::ARGB_8888);
    // GetPasteData
    PasteData pasteData;
    PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(1, pasteData.GetRecordCount());
    auto record = pasteData.GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_PIXELMAP);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, int32_t(UDMF::UDType::SYSTEM_DEFINED_PIXEL_MAP));
    auto newPixelMap = record->GetPixelMapV0();
    ASSERT_TRUE(newPixelMap != nullptr);
    ImageInfo imageInfo = {};
    newPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == imageInfo1.size.height);
    ASSERT_TRUE(imageInfo.size.width == imageInfo1.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == imageInfo1.pixelFormat);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetPixelMap001 end");
}
} // namespace OHOS::MiscServices
