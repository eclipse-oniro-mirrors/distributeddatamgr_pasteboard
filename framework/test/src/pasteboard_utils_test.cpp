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

#include "pasteboard_utils.h"

#include <gtest/gtest.h>

#include "application_defined_record.h"
#include "audio.h"
#include "folder.h"
#include "html.h"
#include "image.h"
#include "link.h"
#include "plain_text.h"
#include "system_defined_appitem.h"
#include "system_defined_form.h"
#include "video.h"
namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace testing;
using namespace OHOS::Media;
class PasteboardUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    Details details_;
    std::vector<uint8_t> rawData_;
};

void PasteboardUtilsTest::SetUpTestCase(void) {}

void PasteboardUtilsTest::TearDownTestCase(void) {}

void PasteboardUtilsTest::SetUp(void)
{
    rawData_ = { 1, 2, 3, 4, 5, 6, 7, 8 };
    details_.insert({ "keyString", "string_test" });
    details_.insert({ "keyInt32", 1 });
    details_.insert({ "keyInt64", 99L });
    details_.insert({ "keyBool", true });
    details_.insert({ "KeyU8Array", rawData_ });
    details_.insert({ "KeyDouble", 1.234 });
}

void PasteboardUtilsTest::TearDown(void) {}

/**
* @tc.name: Text2PasteRecord001
* @tc.desc: pasteData is local data.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(PasteboardUtilsTest, Text2PasteRecord001, TestSize.Level0)
{
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::TEXT);
    std::shared_ptr<UDMF::Text> textRecord = std::make_shared<UDMF::Text>();
    textRecord->SetDetails(details_);
    data.AddRecord(textRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, typeStr);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::TEXT);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::TEXT);
    auto newPlainRecord = static_cast<UDMF::Text*>(newRecord.get());
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
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
    std::string text = "helloWorld_plainText";
    std::string abstract = "helloWorld_plainabstract";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::PlainText> plainTextRecord = std::make_shared<UDMF::PlainText>(text, abstract);
    plainTextRecord->SetDetails(details_);
    data.AddRecord(plainTextRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::PLAIN_TEXT);
    auto plain = record->GetPlainText();
    auto textContent = record->GetTextContent();
    ASSERT_EQ(*plain, text);
    ASSERT_EQ(textContent, abstract);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::PLAIN_TEXT);
    auto newPlainRecord = static_cast<UDMF::PlainText*>(newRecord.get());
    auto newPlainText = newPlainRecord->GetContent();
    auto newAbstract = newPlainRecord->GetAbstract();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newPlainText, text);
    ASSERT_EQ(newAbstract, abstract);
    ASSERT_EQ(newDetails, details_);
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
    std::string text = "<div class='disable'>helloWorld</div>";
    std::string content = "helloWorld_plainabstract";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Html> htmlRecord = std::make_shared<UDMF::Html>(text, content);
    htmlRecord->SetDetails(details_);
    data.AddRecord(htmlRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_HTML);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::HTML);
    auto plain = record->GetHtmlText();
    auto textContent = record->GetTextContent();
    ASSERT_EQ(*plain, text);
    ASSERT_EQ(textContent, content);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::HTML);
    auto newPlainRecord = static_cast<UDMF::Html*>(newRecord.get());
    auto newPlainText = newPlainRecord->GetHtmlContent();
    auto newAbstract = newPlainRecord->GetPlainContent();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newPlainText, text);
    ASSERT_EQ(newAbstract, content);
    ASSERT_EQ(newDetails, details_);
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
    std::string text = "https://www.huawei.com";
    std::string content = "https://www.huawei.com_content";
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::Link> linkRecord = std::make_shared<UDMF::Link>(text, content);
    linkRecord->SetDetails(details_);
    data.AddRecord(linkRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_PLAIN);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::HYPERLINK);
    auto plain = record->GetPlainText();
    auto textContent = record->GetTextContent();
    ASSERT_EQ(*plain, text);
    ASSERT_EQ(textContent, content);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::HYPERLINK);
    auto newPlainRecord = static_cast<UDMF::Link*>(newRecord.get());
    auto newUrl = newPlainRecord->GetUrl();
    auto newDescription = newPlainRecord->GetDescription();
    auto newDetails = newPlainRecord->GetDetails();
    ASSERT_EQ(newUrl, text);
    ASSERT_EQ(newDescription, content);
    ASSERT_EQ(newDetails, details_);
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
    using namespace OHOS::AAFwk;
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string idKey = "id";
    int32_t idValue = 123;
    std::string deviceKey = "deviceId_key";
    std::string deviceValue = "deviceId_value";
    want->SetParam(idKey, idValue);
    want->SetParam(deviceKey, deviceValue);
    std::shared_ptr<UDMF::UnifiedRecord> wantRecord =
        std::make_shared<UDMF::UnifiedRecord>(UDMF::OPENHARMONY_WANT, want);
    UDMF::UnifiedData data;
    data.AddRecord(wantRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_WANT);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, int32_t(UDMF::UDType::OPENHARMONY_WANT));
    auto want1 = record->GetWant();
    auto deviceValue1 = want1->GetStringParam(deviceKey);
    //    ASSERT_EQ(deviceValue1, deviceValue);
    int32_t idValue1 = want1->GetIntParam(idKey, 0);
    ASSERT_EQ(idValue1, idValue);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::OPENHARMONY_WANT);
    auto recordValue = newRecord->GetValue();
    auto wantValue = std::get_if<std::shared_ptr<OHOS::AAFwk::Want>>(&recordValue);
    ASSERT_NE(wantValue, nullptr);
    //    auto deviceValue2 = (*(wantValue))->GetDeviceId();
    //    ASSERT_EQ(deviceValue2, deviceValue);
    int32_t idValue2 = (*(wantValue))->GetIntParam(idKey, 0);
    ASSERT_EQ(idValue2, idValue);
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
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::FILE);
    std::string uri = "file:/uri";
    std::shared_ptr<UDMF::File> fileRecord = std::make_shared<UDMF::File>(uri);
    fileRecord->SetDetails(details_);
    data.AddRecord(fileRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::FILE);
    auto uri1 = record->GetUri()->ToString();
    ASSERT_EQ(uri1, uri);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::FILE);
    auto newFileRecord = static_cast<UDMF::File*>(newRecord.get());
    auto newDetails = newFileRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newFileRecord->GetUri();
    ASSERT_EQ(uri2, uri);
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
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::IMAGE);
    std::string uri = "file:/image";
    std::shared_ptr<UDMF::Image> imageRecord = std::make_shared<UDMF::Image>(uri);
    imageRecord->SetDetails(details_);
    data.AddRecord(imageRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::IMAGE);
    auto uri1 = record->GetUri()->ToString();
    ASSERT_EQ(uri1, uri);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::IMAGE);
    auto newImageRecord = static_cast<UDMF::Image*>(newRecord.get());
    auto newDetails = newImageRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newImageRecord->GetUri();
    ASSERT_EQ(uri2, uri);
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
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::AUDIO);
    std::string uri = "file:/Audio";
    std::shared_ptr<UDMF::Audio> audioRecord = std::make_shared<UDMF::Audio>(uri);
    audioRecord->SetDetails(details_);
    data.AddRecord(audioRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::AUDIO);
    auto uri1 = record->GetUri()->ToString();
    ASSERT_EQ(uri1, uri);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::AUDIO);
    auto newAudioRecord = static_cast<UDMF::Audio*>(newRecord.get());
    auto newDetails = newAudioRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newAudioRecord->GetUri();
    ASSERT_EQ(uri2, uri);
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
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::VIDEO);
    std::string uri = "file:/Video";
    std::shared_ptr<UDMF::Video> videoRecord = std::make_shared<UDMF::Video>(uri);
    videoRecord->SetDetails(details_);
    data.AddRecord(videoRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::VIDEO);
    auto uri1 = record->GetUri()->ToString();
    ASSERT_EQ(uri1, uri);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::VIDEO);
    auto newVideoRecord = static_cast<UDMF::Video*>(newRecord.get());
    auto newDetails = newVideoRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newVideoRecord->GetUri();
    ASSERT_EQ(uri2, uri);
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
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::FOLDER);
    std::string uri = "file:/Folder";
    std::shared_ptr<UDMF::Folder> folderRecord = std::make_shared<UDMF::Folder>(uri);
    folderRecord->SetDetails(details_);
    data.AddRecord(folderRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_TEXT_URI);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::UDType::FOLDER);
    auto uri1 = record->GetUri()->ToString();
    ASSERT_EQ(uri1, uri);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::FOLDER);
    auto newFolderRecord = static_cast<UDMF::Folder*>(newRecord.get());
    auto newDetails = newFolderRecord->GetDetails();
    ASSERT_EQ(newDetails, details_);
    auto uri2 = newFolderRecord->GetUri();
    ASSERT_EQ(uri2, uri);
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
    UDMF::UnifiedData data;
    std::shared_ptr<UDMF::SystemDefinedRecord> systemRecord = std::make_shared<UDMF::SystemDefinedRecord>();
    systemRecord->SetDetails(details_);
    data.AddRecord(systemRecord);

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UD_TYPE_MAP.at(UDMF::SYSTEM_DEFINED_RECORD));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::SYSTEM_DEFINED_RECORD);
    auto details1 = record->GetDetails();
    ASSERT_EQ(*details1, details_);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_RECORD);
    auto newSystemRecord = static_cast<UDMF::SystemDefinedRecord*>(newRecord.get());
    ASSERT_EQ(newSystemRecord->GetDetails(), details_);
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

    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);

    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UD_TYPE_MAP.at(UDMF::SYSTEM_DEFINED_APP_ITEM));
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::SYSTEM_DEFINED_APP_ITEM);
    auto details1 = record->GetDetails();
    auto content = *(record->GetSystemDefinedContent());
    ASSERT_EQ(*details1, details_);
    auto appIconId1 = std::get<std::string>(content["appIconId"]);
    auto appId1 = std::get<std::string>(content["appId"]);
    auto appName1 = std::get<std::string>(content["appName"]);
    auto appLabelId1 = std::get<std::string>(content["appLabelId"]);
    auto bundleName1 = std::get<std::string>(content["bundleName"]);
    auto abilityName1 = std::get<std::string>(content["abilityName"]);
    ASSERT_EQ(appId, appId1);
    ASSERT_EQ(appIconId, appIconId1);
    ASSERT_EQ(appName, appName1);
    ASSERT_EQ(appLabelId, appLabelId1);
    ASSERT_EQ(bundleName, bundleName1);
    ASSERT_EQ(abilityName, abilityName1);

    // 转回去
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_APP_ITEM);
    auto newAppItem = static_cast<UDMF::SystemDefinedAppItem*>(newRecord.get());
    ASSERT_EQ(newAppItem->GetAppId(), appId);
    ASSERT_EQ(newAppItem->GetAppIconId(), appIconId);
    ASSERT_EQ(newAppItem->GetAppName(), appName);
    ASSERT_EQ(newAppItem->GetAppLabelId(), appLabelId);
    ASSERT_EQ(newAppItem->GetBundleName(), bundleName);
    ASSERT_EQ(newAppItem->GetAbilityName(), abilityName);
    ASSERT_EQ(newAppItem->GetDetails(), details_);
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

    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, UDMF::UD_TYPE_MAP.at(UDMF::SYSTEM_DEFINED_FORM));
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
    ASSERT_EQ(formId, formId1);
    ASSERT_EQ(formName, formName1);
    ASSERT_EQ(module, module1);
    ASSERT_EQ(bundleName, bundleName1);
    ASSERT_EQ(abilityName, abilityName1);

    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_FORM);
    auto newForm = static_cast<UDMF::SystemDefinedForm*>(newRecord.get());
    ASSERT_EQ(newForm->GetFormId(), formId);
    ASSERT_EQ(newForm->GetFormName(), formName);
    ASSERT_EQ(newForm->GetModule(), module);
    ASSERT_EQ(newForm->GetBundleName(), bundleName);
    ASSERT_EQ(newForm->GetAbilityName(), abilityName);
    ASSERT_EQ(newForm->GetDetails(), details_);
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
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = { { 5, 7 }, PixelFormat::ARGB_8888, PixelFormat::ARGB_8888 };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    std::shared_ptr<UDMF::UnifiedRecord> pixelMapRecord =
        std::make_shared<UDMF::UnifiedRecord>(UDMF::SYSTEM_DEFINED_PIXEL_MAP, pixelMapIn);
    UDMF::UnifiedData data;
    data.AddRecord(pixelMapRecord);
    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, MIMETYPE_PIXELMAP);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, int32_t(UDMF::UDType::SYSTEM_DEFINED_PIXEL_MAP));
    auto newPixelMap = record->GetPixelMap();
    ASSERT_TRUE(newPixelMap != nullptr);
    ImageInfo imageInfo = {};
    newPixelMap->GetImageInfo(imageInfo);
    ASSERT_TRUE(imageInfo.size.height == opts.size.height);
    ASSERT_TRUE(imageInfo.size.width == opts.size.width);
    ASSERT_TRUE(imageInfo.pixelFormat == opts.pixelFormat);
    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::SYSTEM_DEFINED_PIXEL_MAP);
    auto recordValue = newRecord->GetValue();
    auto newPixelMap1 = std::get_if<std::shared_ptr<OHOS::Media::PixelMap>>(&recordValue);
    ASSERT_NE(newPixelMap1, nullptr);
    ImageInfo imageInfo1 = {};
    (*newPixelMap1)->GetImageInfo(imageInfo1);
    ASSERT_TRUE(imageInfo1.size.height == opts.size.height);
    ASSERT_TRUE(imageInfo1.size.width == opts.size.width);
    ASSERT_TRUE(imageInfo1.pixelFormat == opts.pixelFormat);
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
    UDMF::UnifiedData data;
    auto typeStr = UDMF::UD_TYPE_MAP.at(UDMF::APPLICATION_DEFINED_RECORD);
    std::shared_ptr<UDMF::ApplicationDefinedRecord> appRecord = std::make_shared<UDMF::ApplicationDefinedRecord>();
    std::vector<uint8_t> rawData1(8, 1);
    std::map<std::string, std::vector<uint8_t>> customData;
    customData[typeStr] = rawData1;
    appRecord->SetApplicationDefinedType(typeStr);
    appRecord->SetRawData(rawData1);
    data.AddRecord(appRecord);
    // Convert
    auto pasteData = PasteboardUtils::GetInstance()->Convert(data);
    ASSERT_EQ(1, pasteData->GetRecordCount());
    auto record = pasteData->GetRecordAt(0);
    auto type = record->GetMimeType();
    ASSERT_EQ(type, typeStr);
    auto udType = record->GetUDType();
    ASSERT_EQ(udType, UDMF::APPLICATION_DEFINED_RECORD);
    auto items = record->GetCustomData()->GetItemData();
    ASSERT_EQ(items, customData);
    // Convert
    auto newData = PasteboardUtils::GetInstance()->Convert(*pasteData);
    ASSERT_EQ(1, newData->GetRecords().size());
    auto newRecord = newData->GetRecordAt(0);
    auto newType = newRecord->GetType();
    ASSERT_EQ(newType, UDMF::APPLICATION_DEFINED_RECORD);
    auto newSystemRecord = static_cast<UDMF::ApplicationDefinedRecord*>(newRecord.get());
    ASSERT_EQ(newSystemRecord->GetRawData(), rawData1);
}
} // namespace OHOS::MiscServices