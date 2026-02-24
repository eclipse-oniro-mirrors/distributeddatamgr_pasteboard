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
#include "pasteboard_error.h"
#include "pixel_map.h"
#include "plain_text.h"
#include "system_defined_appitem.h"
#include "system_defined_form.h"
#include "system_defined_pixelmap.h"
#include "system_defined_record.h"
#include "text.h"
#include "unified_data.h"
#include "unified_record.h"
#include "video.h"
#include "want.h"

using namespace OHOS::AAFwk;
using namespace OHOS::Media;
using namespace OHOS::MiscServices;
using namespace OHOS::UDMF;
using namespace testing::ext;
namespace OHOS::Test {
class PasteboardClientUdmfDelayTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;

    void SetUnifiedData();
    void SetTextUnifiedData();
    void SetPlainTextUnifiedData();
    void SetLinkUnifiedData();
    void SetHtmlUnifiedData();
    void SetFileUnifiedData();
    void SetImageUnifiedData();
    void SetVideoUnifiedData();
    void SetAudioUnifiedData();
    void SetFolderUnifiedData();
    void SetSysRecordUnifiedData();
    void SetSysFormUnifiedData();
    void SetSysAppItemUnifiedData();
    void SetAppRecordUnifiedData();
    void SetWantUnifiedData();
    void SetPixelMapUnifiedData();

    void CompareDetails(const UDDetails &details);

    static PasteData pasteData_;
    static UnifiedData unifiedData_;
};

PasteData PasteboardClientUdmfDelayTest::pasteData_;
UnifiedData PasteboardClientUdmfDelayTest::unifiedData_;

class PasteboardDelayGetterImpl : public MiscServices::PasteboardDelayGetter {
public:
    void GetPasteData(const std::string &type, PasteData &data) override
    {
        (void)type;
        data = PasteboardClientUdmfDelayTest::pasteData_;
    }

    void GetUnifiedData(const std::string &type, UnifiedData &data) override
    {
        (void)type;
        data = PasteboardClientUdmfDelayTest::unifiedData_;
    }
};

void PasteboardClientUdmfDelayTest::SetUpTestCase() { }

void PasteboardClientUdmfDelayTest::TearDownTestCase() { }

void PasteboardClientUdmfDelayTest::SetUp()
{
    PasteboardClient::GetInstance()->Clear();
}

void PasteboardClientUdmfDelayTest::TearDown() { }

void PasteboardClientUdmfDelayTest::SetUnifiedData()
{
    UnifiedData delayData;
    std::shared_ptr<PasteboardDelayGetter> delayGetter = std::make_shared<PasteboardDelayGetterImpl>();
    auto status = PasteboardClient::GetInstance()->SetUnifiedData(delayData, delayGetter);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
}

void PasteboardClientUdmfDelayTest::SetTextUnifiedData()
{
    auto text = std::make_shared<Text>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    text->SetDetails(details);
    UnifiedData data;
    data.AddRecord(text);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetPlainTextUnifiedData()
{
    auto plainText = std::make_shared<PlainText>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    plainText->SetDetails(details);
    plainText->SetContent("content");
    plainText->SetAbstract("abstract");
    UnifiedData data;
    data.AddRecord(plainText);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetLinkUnifiedData()
{
    auto link = std::make_shared<Link>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    link->SetDetails(details);
    link->SetUrl("url");
    link->SetDescription("description");
    UnifiedData data;
    data.AddRecord(link);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetHtmlUnifiedData()
{
    auto html = std::make_shared<Html>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    html->SetDetails(details);
    html->SetHtmlContent("htmlContent");
    html->SetPlainContent("plainContent");
    UnifiedData data;
    data.AddRecord(html);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetFileUnifiedData()
{
    auto file = std::make_shared<File>();
    file->SetUri("uri");
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    file->SetDetails(details);
    UnifiedData data;
    data.AddRecord(file);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetImageUnifiedData()
{
    auto image = std::make_shared<Image>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    image->SetDetails(details);
    image->SetUri("uri");
    UnifiedData data;
    data.AddRecord(image);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetVideoUnifiedData()
{
    auto video = std::make_shared<Video>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    video->SetDetails(details);
    video->SetUri("uri");
    UnifiedData data;
    data.AddRecord(video);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetAudioUnifiedData()
{
    auto audio = std::make_shared<Audio>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    audio->SetDetails(details);
    audio->SetUri("uri");
    UnifiedData data;
    data.AddRecord(audio);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetFolderUnifiedData()
{
    auto folder = std::make_shared<Folder>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    folder->SetDetails(details);
    folder->SetUri("uri");
    UnifiedData data;
    data.AddRecord(folder);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetSysRecordUnifiedData()
{
    auto systemDefinedRecord = std::make_shared<SystemDefinedRecord>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    systemDefinedRecord->SetDetails(details);
    UnifiedData data;
    data.AddRecord(systemDefinedRecord);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetSysFormUnifiedData()
{
    auto systemDefinedForm = std::make_shared<SystemDefinedForm>();
    UDDetails details;
    int32_t formId = 1;
    details.insert({ "udmf_key", "udmf_value" });
    systemDefinedForm->SetDetails(details);
    systemDefinedForm->SetFormId(formId);
    systemDefinedForm->SetFormName("formName");
    systemDefinedForm->SetModule("module");
    systemDefinedForm->SetAbilityName("abilityName");
    systemDefinedForm->SetBundleName("bundleName");
    UnifiedData data;
    data.AddRecord(systemDefinedForm);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetSysAppItemUnifiedData()
{
    auto systemDefinedAppItem = std::make_shared<SystemDefinedAppItem>();
    UDDetails details;
    details.insert({ "udmf_key", "udmf_value" });
    systemDefinedAppItem->SetDetails(details);
    systemDefinedAppItem->SetAppId("appId");
    systemDefinedAppItem->SetAppName("appName");
    systemDefinedAppItem->SetAppIconId("appIconId");
    systemDefinedAppItem->SetAppLabelId("appLabelId");
    systemDefinedAppItem->SetBundleName("bundleName");
    systemDefinedAppItem->SetAbilityName("abilityName");
    UnifiedData data;
    data.AddRecord(systemDefinedAppItem);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetAppRecordUnifiedData()
{
    auto applicationDefinedRecord = std::make_shared<ApplicationDefinedRecord>();
    applicationDefinedRecord->SetApplicationDefinedType("applicationDefinedType");
    std::vector<uint8_t> rawData = { 1, 2, 3, 4, 5 };
    applicationDefinedRecord->SetRawData(rawData);
    UnifiedData data;
    data.AddRecord(applicationDefinedRecord);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetWantUnifiedData()
{
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string idKey = "id";
    int32_t idValue = 456;
    std::string deviceKey = "deviceId_key";
    std::string deviceValue = "deviceId_value";
    want->SetParam(idKey, idValue);
    want->SetParam(deviceKey, deviceValue);
    std::shared_ptr<UDMF::UnifiedRecord> record = std::make_shared<UDMF::UnifiedRecord>(UDMF::OPENHARMONY_WANT, want);
    UnifiedData data;
    data.AddRecord(record);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::SetPixelMapUnifiedData()
{
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    std::shared_ptr<UnifiedRecord> record =
        std::make_shared<SystemDefinedPixelMap>(SYSTEM_DEFINED_PIXEL_MAP, pixelMapIn);
    UnifiedData data;
    data.AddRecord(record);
    unifiedData_ = data;
    SetUnifiedData();
}

void PasteboardClientUdmfDelayTest::CompareDetails(const UDDetails &details)
{
    for (const auto &detail : details) {
        auto key = detail.first;
        ASSERT_EQ(key, "udmf_key");
        auto value = detail.second;
        auto str = std::get<std::string>(value);
        ASSERT_EQ(str, "udmf_value");
    }
}

/**
 * @tc.name: SetTextDataTest001
 * @tc.desc: SetUnifiedData of Text with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetTextDataTest001, TestSize.Level1)
{
    SetTextUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::TEXT);
    auto text = static_cast<Text *>(unifiedRecord.get());
    ASSERT_NE(text, nullptr);
    CompareDetails(text->GetDetails());

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), "general.text");
}

/**
 * @tc.name: SetPlainTextDataTest001
 * @tc.desc: SetUnifiedData of PlainText with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetPlainTextDataTest001, TestSize.Level1)
{
    SetPlainTextUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::PLAIN_TEXT);
    auto text = static_cast<Text *>(unifiedRecord.get());
    ASSERT_NE(text, nullptr);
    CompareDetails(text->GetDetails());
    auto plainText1 = static_cast<PlainText *>(unifiedRecord.get());
    ASSERT_NE(plainText1, nullptr);
    EXPECT_EQ(plainText1->GetContent(), "content");
    EXPECT_EQ(plainText1->GetAbstract(), "abstract");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_PLAIN);
    auto plainText2 = pasteData.GetPrimaryText();
    ASSERT_NE(plainText2, nullptr);
    ASSERT_EQ(*plainText2, "content");
}

/**
 * @tc.name: SetLinkDataTest001
 * @tc.desc: SetUnifiedData of Link with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetLinkDataTest001, TestSize.Level1)
{
    SetLinkUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::HYPERLINK);
    auto text = static_cast<Text *>(unifiedRecord.get());
    ASSERT_NE(text, nullptr);
    CompareDetails(text->GetDetails());
    auto link = static_cast<Link *>(unifiedRecord.get());
    ASSERT_NE(link, nullptr);
    EXPECT_EQ(link->GetUrl(), "url");
    EXPECT_EQ(link->GetDescription(), "description");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_PLAIN);
    auto plainText = pasteData.GetPrimaryText();
    ASSERT_NE(plainText, nullptr);
    ASSERT_EQ(*plainText, "url");
}

/**
 * @tc.name: SetHtmlDataTest001
 * @tc.desc: SetUnifiedData of Html with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetHtmlDataTest001, TestSize.Level1)
{
    SetHtmlUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::HTML);
    auto text = static_cast<Text *>(unifiedRecord.get());
    ASSERT_NE(text, nullptr);
    CompareDetails(text->GetDetails());
    auto html = static_cast<Html *>(unifiedRecord.get());
    ASSERT_NE(html, nullptr);
    EXPECT_EQ(html->GetHtmlContent(), "htmlContent");
    EXPECT_EQ(html->GetPlainContent(), "plainContent");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_HTML);
    auto htmlText = pasteRecord->GetHtmlTextV0();
    ASSERT_NE(htmlText, nullptr);
    ASSERT_EQ(*htmlText, "htmlContent");
}

/**
 * @tc.name: SetFileDataTest001
 * @tc.desc: SetUnifiedData of File with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetFileDataTest001, TestSize.Level1)
{
    SetFileUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::FILE);
    auto file = static_cast<File *>(unifiedRecord.get());
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(file->GetUri(), "uri");
    CompareDetails(file->GetDetails());

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_URI);
    auto uri = pasteRecord->GetUriV0();
    ASSERT_NE(uri, nullptr);
    ASSERT_EQ(uri->ToString(), "uri");
}

/**
 * @tc.name: SetImageDataTest001
 * @tc.desc: SetUnifiedData of Image with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetImageDataTest001, TestSize.Level1)
{
    SetImageUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::IMAGE);
    auto file = static_cast<File *>(unifiedRecord.get());
    ASSERT_NE(file, nullptr);
    CompareDetails(file->GetDetails());
    auto image = static_cast<Image *>(unifiedRecord.get());
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->GetUri(), "uri");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_URI);
    auto uri = pasteRecord->GetUriV0();
    ASSERT_NE(uri, nullptr);
    ASSERT_EQ(uri->ToString(), "uri");
}

/**
 * @tc.name: SetVideoDataTest001
 * @tc.desc: SetUnifiedData of Video with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetVideoDataTest001, TestSize.Level1)
{
    SetVideoUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::VIDEO);
    auto file = static_cast<File *>(unifiedRecord.get());
    ASSERT_NE(file, nullptr);
    CompareDetails(file->GetDetails());
    auto video = static_cast<Video *>(unifiedRecord.get());
    ASSERT_NE(video, nullptr);
    EXPECT_EQ(video->GetUri(), "uri");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_URI);
    auto uri = pasteRecord->GetUriV0();
    ASSERT_NE(uri, nullptr);
    ASSERT_EQ(uri->ToString(), "uri");
}

/**
 * @tc.name: SetAudioDataTest001
 * @tc.desc: SetUnifiedData of Audio with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetAudioDataTest001, TestSize.Level1)
{
    SetAudioUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::AUDIO);
    auto file = static_cast<File *>(unifiedRecord.get());
    ASSERT_NE(file, nullptr);
    CompareDetails(file->GetDetails());
    auto audio = static_cast<Audio *>(unifiedRecord.get());
    ASSERT_NE(audio, nullptr);
    EXPECT_EQ(audio->GetUri(), "uri");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_URI);
    auto uri = pasteRecord->GetUriV0();
    ASSERT_NE(uri, nullptr);
    ASSERT_EQ(uri->ToString(), "uri");
}

/**
 * @tc.name: SetFolderDataTest001
 * @tc.desc: Set UnifiedData of Folder with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetFolderDataTest001, TestSize.Level1)
{
    SetFolderUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::FOLDER);
    auto file = static_cast<File *>(unifiedRecord.get());
    ASSERT_NE(file, nullptr);
    CompareDetails(file->GetDetails());
    auto folder = static_cast<Folder *>(unifiedRecord.get());
    ASSERT_NE(folder, nullptr);
    EXPECT_EQ(folder->GetUri(), "uri");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    ASSERT_EQ(pasteRecord->GetMimeType(), MIMETYPE_TEXT_URI);
    auto uri = pasteRecord->GetUriV0();
    ASSERT_NE(uri, nullptr);
    ASSERT_EQ(uri->ToString(), "uri");
}

/**
 * @tc.name: SetSysRecordDataTest001
 * @tc.desc: SetUnifiedData of SysRecord with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetSysRecordDataTest001, TestSize.Level1)
{
    SetSysRecordUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::SYSTEM_DEFINED_RECORD);
    auto systemDefinedRecord = static_cast<SystemDefinedRecord *>(unifiedRecord.get());
    ASSERT_NE(systemDefinedRecord, nullptr);
    CompareDetails(systemDefinedRecord->GetDetails());

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    auto customData = pasteRecord->GetCustomData();
    ASSERT_NE(customData, nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_EQ(itemData.size(), 1);
    auto item = itemData.find("SystemDefinedType");
    ASSERT_NE(item, itemData.end());
}

/**
 * @tc.name: SetSysFormDataTest001
 * @tc.desc: SetUnifiedData of SysForm with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetSysFormDataTest001, TestSize.Level1)
{
    SetSysFormUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::SYSTEM_DEFINED_FORM);
    auto systemDefinedRecord = static_cast<SystemDefinedRecord *>(unifiedRecord.get());
    ASSERT_NE(systemDefinedRecord, nullptr);
    CompareDetails(systemDefinedRecord->GetDetails());
    auto systemDefinedForm = static_cast<SystemDefinedForm *>(unifiedRecord.get());
    ASSERT_NE(systemDefinedForm, nullptr);
    ASSERT_EQ(systemDefinedForm->GetFormId(), 1);
    ASSERT_EQ(systemDefinedForm->GetFormName(), "formName");
    ASSERT_EQ(systemDefinedForm->GetBundleName(), "bundleName");
    ASSERT_EQ(systemDefinedForm->GetAbilityName(), "abilityName");
    ASSERT_EQ(systemDefinedForm->GetModule(), "module");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    auto customData = pasteRecord->GetCustomData();
    ASSERT_NE(customData, nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_EQ(itemData.size(), 1);
    auto item = itemData.find("openharmony.form");
    ASSERT_NE(item, itemData.end());
}

/**
 * @tc.name: SetSysAppItemDataTest001
 * @tc.desc: SetUnifiedData of SysAppItem with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetSysAppItemDataTest001, TestSize.Level1)
{
    SetSysAppItemUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::SYSTEM_DEFINED_APP_ITEM);
    auto systemDefinedRecord = static_cast<SystemDefinedRecord *>(unifiedRecord.get());
    ASSERT_NE(systemDefinedRecord, nullptr);
    CompareDetails(systemDefinedRecord->GetDetails());
    auto systemDefinedAppItem = static_cast<SystemDefinedAppItem *>(unifiedRecord.get());
    ASSERT_NE(systemDefinedAppItem, nullptr);
    ASSERT_EQ(systemDefinedAppItem->GetAppId(), "appId");
    ASSERT_EQ(systemDefinedAppItem->GetAppName(), "appName");
    ASSERT_EQ(systemDefinedAppItem->GetBundleName(), "bundleName");
    ASSERT_EQ(systemDefinedAppItem->GetAbilityName(), "abilityName");
    ASSERT_EQ(systemDefinedAppItem->GetAppIconId(), "appIconId");
    ASSERT_EQ(systemDefinedAppItem->GetAppLabelId(), "appLabelId");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    auto details1 = pasteRecord->GetDetails();
    auto entries = pasteRecord->GetEntries();
    ASSERT_NE(entries.size(), 0);
    auto entryValue = entries.front()->GetValue();
    auto newAppItem1 = std::make_shared<UDMF::SystemDefinedAppItem>(UDMF::SYSTEM_DEFINED_APP_ITEM, entryValue);
    ASSERT_EQ(newAppItem1->GetAppId(), "appId");
    ASSERT_EQ(newAppItem1->GetAppIconId(), "appIconId");
    ASSERT_EQ(newAppItem1->GetAppName(), "appName");
    ASSERT_EQ(newAppItem1->GetAppLabelId(), "appLabelId");
    ASSERT_EQ(newAppItem1->GetBundleName(), "bundleName");
    ASSERT_EQ(newAppItem1->GetAbilityName(), "abilityName");
}

/**
 * @tc.name: SetAppRecordDataTest001
 * @tc.desc: Set UnifiedData of AppRecord with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetAppRecordDataTest001, TestSize.Level1)
{
    SetAppRecordUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::APPLICATION_DEFINED_RECORD);
    auto applicationDefinedRecord = static_cast<ApplicationDefinedRecord *>(unifiedRecord.get());
    ASSERT_NE(applicationDefinedRecord, nullptr);
    ASSERT_EQ(applicationDefinedRecord->GetApplicationDefinedType(), "applicationDefinedType");
    auto outputRawData = applicationDefinedRecord->GetRawData();
    std::vector<uint8_t> inputRawData = { 1, 2, 3, 4, 5 };
    ASSERT_EQ(outputRawData.size(), inputRawData.size());
    for (uint32_t i = 0; i < outputRawData.size(); ++i) {
        ASSERT_EQ(outputRawData[i], inputRawData[i]);
    }

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetRecordAt(0);
    ASSERT_NE(pasteRecord, nullptr);
    auto customData = pasteRecord->GetCustomData();
    ASSERT_NE(customData, nullptr);
    auto itemData = customData->GetItemData();
    ASSERT_EQ(itemData.size(), 1);
    auto item = itemData.find("applicationDefinedType");
    ASSERT_NE(item, itemData.end());
}

/**
 * @tc.name: SetWantDataTest001
 * @tc.desc: Set UnifiedData of Want with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetWantDataTest001, TestSize.Level1)
{
    SetWantUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::OPENHARMONY_WANT);
    auto value = unifiedRecord->GetValue();
    auto want = std::get_if<std::shared_ptr<Want>>(&value);
    ASSERT_NE(want, nullptr);
    auto idValue1 = (*(want))->GetIntParam("id", 0);
    ASSERT_EQ(idValue1, 456);
    auto deviceValue1 = (*(want))->GetStringParam("deviceId_key");
    ASSERT_EQ(deviceValue1, "deviceId_value");

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetPrimaryWant();
    ASSERT_NE(pasteRecord, nullptr);
    auto idValue2 = pasteRecord->GetIntParam("id", 0);
    ASSERT_EQ(idValue2, 456);
    auto deviceValue2 = pasteRecord->GetStringParam("deviceId_key");
    ASSERT_EQ(deviceValue2, "deviceId_value");
}

/**
 * @tc.name: SetPixelMapDataTest001
 * @tc.desc: Set UnifiedData of PixelMap with delay getter and GetUnifiedData and GetPasteData
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(PasteboardClientUdmfDelayTest, SetPixelMapDataTest001, TestSize.Level1)
{
    SetPixelMapUnifiedData();

    UnifiedData unifiedData;
    auto status = PasteboardClient::GetInstance()->GetUnifiedData(unifiedData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    std::shared_ptr<UnifiedRecord> unifiedRecord = unifiedData.GetRecordAt(0);
    ASSERT_NE(unifiedRecord, nullptr);
    auto type = unifiedRecord->GetType();
    ASSERT_EQ(type, UDType::SYSTEM_DEFINED_PIXEL_MAP);
    auto value = unifiedRecord->GetValue();
    auto pixelMap = std::get_if<std::shared_ptr<PixelMap>>(&value);
    ASSERT_NE(pixelMap, nullptr);
    ImageInfo imageInfo1 = {};
    (*pixelMap)->GetImageInfo(imageInfo1);
    ASSERT_EQ(imageInfo1.size.height, 7);
    ASSERT_EQ(imageInfo1.size.width, 5);
    ASSERT_EQ(imageInfo1.pixelFormat, PixelFormat::ARGB_8888);

    PasteData pasteData;
    status = PasteboardClient::GetInstance()->GetPasteData(pasteData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto pasteRecord = pasteData.GetPrimaryPixelMap();
    ASSERT_NE(pasteRecord, nullptr);
    ImageInfo imageInfo2 = {};
    pasteRecord->GetImageInfo(imageInfo2);
    ASSERT_EQ(imageInfo2.size.height, 7);
    ASSERT_EQ(imageInfo2.size.width, 5);
    ASSERT_EQ(imageInfo2.pixelFormat, PixelFormat::ARGB_8888);
}
} // namespace OHOS::Test