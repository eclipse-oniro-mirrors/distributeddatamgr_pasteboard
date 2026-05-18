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
#include <variant>

#include "entry_getter.h"
#include "pasteboard_client.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pixel_map.h"

namespace OHOS::MiscServices {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::UDMF;
using namespace OHOS::AAFwk;
using namespace OHOS::Media;
static std::string text_ = "test";
static std::string uri_ = "uri";
static std::string fileType_ = "test";
static std::string html_ = "<div class='disable'>helloWorld</div>";
static std::string link_ = "http://abc.com";
static std::string plainTextUtdId_ = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::PLAIN_TEXT);
static std::string htmlUtdId_ = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HTML);
static std::string fileUriUtdId_ = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::FILE_URI);
static std::string pixelMapUtdId_ = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::SYSTEM_DEFINED_PIXEL_MAP);
static std::string linkUtdId_ = UDMF::UtdUtils::GetUtdIdFromUtdEnum(UDMF::HYPERLINK);
class PasteboardMultiTypeUnifiedDataDelayTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static UDMF::ValueType InitPlainUds();
    static UDMF::ValueType InitHtmlUds();
    static UDMF::ValueType InitFileUriUds();
    static UDMF::ValueType InitPixelMapUds();
    static UDMF::ValueType InitLinkUds();

    void CheckPlainUds(const UDMF::ValueType &value);
    void CheckHtmlUds(const UDMF::ValueType &value);
    void CheckFileUriUds(const UDMF::ValueType &value);
    void CheckPixelMapUds(const UDMF::ValueType &value);
    void CheckLinkUds(const UDMF::ValueType &value);
};

void PasteboardMultiTypeUnifiedDataDelayTest::SetUpTestCase(void)
{
    PasteboardClient::GetInstance()->Clear();
}

void PasteboardMultiTypeUnifiedDataDelayTest::TearDownTestCase(void) { }

void PasteboardMultiTypeUnifiedDataDelayTest::SetUp() { }

void PasteboardMultiTypeUnifiedDataDelayTest::TearDown() { }

UDMF::ValueType PasteboardMultiTypeUnifiedDataDelayTest::InitPlainUds()
{
    Object plainUds;
    plainUds.value_[UDMF::UNIFORM_DATA_TYPE] = plainTextUtdId_;
    plainUds.value_[UDMF::CONTENT] = text_;
    return std::make_shared<Object>(plainUds);
}

UDMF::ValueType PasteboardMultiTypeUnifiedDataDelayTest::InitHtmlUds()
{
    Object htmlObject;
    htmlObject.value_[UDMF::UNIFORM_DATA_TYPE] = htmlUtdId_;
    htmlObject.value_[UDMF::HTML_CONTENT] = html_;
    return std::make_shared<Object>(htmlObject);
}

UDMF::ValueType PasteboardMultiTypeUnifiedDataDelayTest::InitFileUriUds()
{
    Object fileUriObject;
    fileUriObject.value_[UDMF::UNIFORM_DATA_TYPE] = fileUriUtdId_;
    fileUriObject.value_[UDMF::FILE_URI_PARAM] = uri_;
    fileUriObject.value_[UDMF::FILE_TYPE] = fileType_;
    return std::make_shared<Object>(fileUriObject);
}

UDMF::ValueType PasteboardMultiTypeUnifiedDataDelayTest::InitPixelMapUds()
{
    Object object;
    object.value_[UDMF::UNIFORM_DATA_TYPE] = pixelMapUtdId_;
    uint32_t color[100] = { 3, 7, 9, 9, 7, 6 };
    InitializationOptions opts = {
        {5, 7},
        PixelFormat::ARGB_8888, PixelFormat::ARGB_8888
    };
    std::unique_ptr<PixelMap> pixelMap = PixelMap::Create(color, sizeof(color) / sizeof(color[0]), opts);
    std::shared_ptr<PixelMap> pixelMapIn = move(pixelMap);
    object.value_[UDMF::PIXEL_MAP] = pixelMapIn;
    return std::make_shared<Object>(object);
}

UDMF::ValueType PasteboardMultiTypeUnifiedDataDelayTest::InitLinkUds()
{
    Object linkObject;
    linkObject.value_[UDMF::UNIFORM_DATA_TYPE] = linkUtdId_;
    linkObject.value_[UDMF::URL] = link_;
    return std::make_shared<Object>(linkObject);
}

class EntryGetterImpl : public UDMF::EntryGetter {
public:
    UDMF::ValueType GetValueByType(const std::string &utdId) override;
};

UDMF::ValueType EntryGetterImpl::GetValueByType(const std::string &utdId)
{
    if (utdId == plainTextUtdId_) {
        return PasteboardMultiTypeUnifiedDataDelayTest::InitPlainUds();
    }
    if (utdId == htmlUtdId_) {
        return PasteboardMultiTypeUnifiedDataDelayTest::InitHtmlUds();
    }
    if (utdId == fileUriUtdId_) {
        return PasteboardMultiTypeUnifiedDataDelayTest::InitFileUriUds();
    }
    if (utdId == pixelMapUtdId_) {
        return PasteboardMultiTypeUnifiedDataDelayTest::InitPixelMapUds();
    }
    if (utdId == linkUtdId_) {
        return PasteboardMultiTypeUnifiedDataDelayTest::InitLinkUds();
    }
    return nullptr;
}

void PasteboardMultiTypeUnifiedDataDelayTest::CheckPlainUds(const UDMF::ValueType &value)
{
    ASSERT_NE(std::get_if<std::shared_ptr<Object>>(&value), nullptr);
    auto obj = std::get<std::shared_ptr<Object>>(value);
    ASSERT_NE(obj, nullptr);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::UNIFORM_DATA_TYPE]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::UNIFORM_DATA_TYPE]), plainTextUtdId_);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::CONTENT]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::CONTENT]), text_);
}

void PasteboardMultiTypeUnifiedDataDelayTest::CheckHtmlUds(const UDMF::ValueType &value)
{
    ASSERT_NE(std::get_if<std::shared_ptr<Object>>(&value), nullptr);
    auto obj = std::get<std::shared_ptr<Object>>(value);
    ASSERT_NE(obj, nullptr);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::UNIFORM_DATA_TYPE]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::UNIFORM_DATA_TYPE]), htmlUtdId_);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::HTML_CONTENT]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::HTML_CONTENT]), html_);
}

void PasteboardMultiTypeUnifiedDataDelayTest::CheckFileUriUds(const UDMF::ValueType &value)
{
    ASSERT_NE(std::get_if<std::shared_ptr<Object>>(&value), nullptr);
    auto obj = std::get<std::shared_ptr<Object>>(value);
    ASSERT_NE(obj, nullptr);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::UNIFORM_DATA_TYPE]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::UNIFORM_DATA_TYPE]), fileUriUtdId_);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::FILE_URI_PARAM]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::FILE_URI_PARAM]), uri_);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::FILE_TYPE]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::FILE_TYPE]), fileType_);
}

void PasteboardMultiTypeUnifiedDataDelayTest::CheckPixelMapUds(const UDMF::ValueType &value)
{
    ASSERT_NE(std::get_if<std::shared_ptr<Object>>(&value), nullptr);
    auto obj = std::get<std::shared_ptr<Object>>(value);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::UNIFORM_DATA_TYPE]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::UNIFORM_DATA_TYPE]), pixelMapUtdId_);
    auto pixelMap = std::get_if<std::shared_ptr<PixelMap>>(&obj->value_[UDMF::PIXEL_MAP]);
    ASSERT_NE(pixelMap, nullptr);
    ImageInfo imageInfo = {};
    (*pixelMap)->GetImageInfo(imageInfo);
    ASSERT_EQ(imageInfo.size.height, 7);
    ASSERT_EQ(imageInfo.size.width, 5);
    ASSERT_EQ(imageInfo.pixelFormat, PixelFormat::ARGB_8888);
}

void PasteboardMultiTypeUnifiedDataDelayTest::CheckLinkUds(const UDMF::ValueType &value)
{
    ASSERT_NE(std::get_if<std::shared_ptr<Object>>(&value), nullptr);
    auto obj = std::get<std::shared_ptr<Object>>(value);
    ASSERT_NE(obj, nullptr);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::UNIFORM_DATA_TYPE]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::UNIFORM_DATA_TYPE]), linkUtdId_);
    ASSERT_NE(std::get_if<std::string>(&obj->value_[UDMF::URL]), nullptr);
    ASSERT_EQ(std::get<std::string>(obj->value_[UDMF::URL]), link_);
}

/**
 * @tc.name: SetMultiTypeUnifiedDataDelayTest001
 * @tc.desc: add empty entry with entry getter to pasteboard
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardMultiTypeUnifiedDataDelayTest, SetMultiTypeUnifiedDataDelayTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMultiTypeUnifiedDataDelayTest001 start");
    UnifiedData inputData;
    std::shared_ptr<UnifiedRecord> inputRecord = std::make_shared<UnifiedRecord>();
    std::vector<std::string> inputTypes;
    inputTypes.emplace_back(plainTextUtdId_);
    inputTypes.emplace_back(htmlUtdId_);
    inputTypes.emplace_back(fileUriUtdId_);
    inputTypes.emplace_back(pixelMapUtdId_);
    inputTypes.emplace_back(linkUtdId_);
    std::shared_ptr<EntryGetter> entryGetter = std::make_shared<EntryGetterImpl>();
    inputRecord->SetEntryGetter(inputTypes, entryGetter);
    inputData.AddRecord(inputRecord);
    auto status = PasteboardClient::GetInstance()->SetUdsdData(inputData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));

    UnifiedData outputData;
    status = PasteboardClient::GetInstance()->GetUdsdData(outputData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto inputRecords = inputData.GetRecords();
    auto outputRecords = outputData.GetRecords();
    ASSERT_EQ(outputRecords.size(), inputRecords.size());
    auto outputRecord = outputData.GetRecordAt(0);
    ASSERT_NE(outputRecord, nullptr);
    auto outputTypes = outputRecord->GetUtdIds();
    ASSERT_EQ(outputTypes.size(), inputTypes.size());

    ASSERT_TRUE(outputTypes.find(plainTextUtdId_) != outputTypes.end());
    CheckPlainUds(outputRecord->GetEntry(plainTextUtdId_));

    ASSERT_TRUE(outputTypes.find(htmlUtdId_) != outputTypes.end());
    CheckHtmlUds(outputRecord->GetEntry(htmlUtdId_));

    ASSERT_TRUE(outputTypes.find(fileUriUtdId_) != outputTypes.end());
    CheckFileUriUds(outputRecord->GetEntry(fileUriUtdId_));

    ASSERT_TRUE(outputTypes.find(pixelMapUtdId_) != outputTypes.end());
    CheckPixelMapUds(outputRecord->GetEntry(pixelMapUtdId_));

    ASSERT_TRUE(outputTypes.find(linkUtdId_) != outputTypes.end());
    CheckLinkUds(outputRecord->GetEntry(linkUtdId_));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMultiTypeUnifiedDataDelayTest001 end");
}

/**
 * @tc.name: SetMultiTypeUnifiedDataDelayTest002
 * @tc.desc: add empty and valid entry with entry getter to pasteboard
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardMultiTypeUnifiedDataDelayTest, SetMultiTypeUnifiedDataDelayTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMultiTypeUnifiedDataDelayTest002 start");
    UnifiedData inputData;
    std::shared_ptr<UnifiedRecord> inputRecord = std::make_shared<UnifiedRecord>();
    inputRecord->AddEntry(plainTextUtdId_, InitPlainUds());
    inputRecord->AddEntry(pixelMapUtdId_, InitPixelMapUds());
    std::vector<std::string> inputTypes;
    inputTypes.emplace_back(htmlUtdId_);
    inputTypes.emplace_back(fileUriUtdId_);
    inputTypes.emplace_back(linkUtdId_);
    std::shared_ptr<EntryGetter> entryGetter = std::make_shared<EntryGetterImpl>();
    inputRecord->SetEntryGetter(inputTypes, entryGetter);
    inputData.AddRecord(inputRecord);
    auto status = PasteboardClient::GetInstance()->SetUdsdData(inputData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));

    UnifiedData outputData;
    status = PasteboardClient::GetInstance()->GetUdsdData(outputData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto inputRecords = inputData.GetRecords();
    auto outputRecords = outputData.GetRecords();
    ASSERT_EQ(outputRecords.size(), inputRecords.size());
    auto outputRecord = outputData.GetRecordAt(0);
    ASSERT_NE(outputRecord, nullptr);
    auto outputTypes = outputRecord->GetUtdIds();
    auto tempTypes = inputRecord->GetUtdIds();
    ASSERT_EQ(outputTypes.size(), tempTypes.size());

    ASSERT_TRUE(outputTypes.find(plainTextUtdId_) != outputTypes.end());
    CheckPlainUds(outputRecord->GetEntry(plainTextUtdId_));

    ASSERT_TRUE(outputTypes.find(htmlUtdId_) != outputTypes.end());
    CheckHtmlUds(outputRecord->GetEntry(htmlUtdId_));

    ASSERT_TRUE(outputTypes.find(fileUriUtdId_) != outputTypes.end());
    CheckFileUriUds(outputRecord->GetEntry(fileUriUtdId_));

    ASSERT_TRUE(outputTypes.find(pixelMapUtdId_) != outputTypes.end());
    CheckPixelMapUds(outputRecord->GetEntry(pixelMapUtdId_));

    ASSERT_TRUE(outputTypes.find(linkUtdId_) != outputTypes.end());
    CheckLinkUds(outputRecord->GetEntry(linkUtdId_));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMultiTypeUnifiedDataDelayTest002 end");
}

/**
 * @tc.name: SetMultiTypeUnifiedDataDelayTest003
 * @tc.desc: add more unified record with entry getter to pasteboard
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardMultiTypeUnifiedDataDelayTest, SetMultiTypeUnifiedDataDelayTest003, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMultiTypeUnifiedDataDelayTest003 start");
    UnifiedData inputData;
    std::shared_ptr<UnifiedRecord> inputRecord1 = std::make_shared<UnifiedRecord>();
    std::vector<std::string> inputTypes1;
    inputTypes1.emplace_back(plainTextUtdId_);
    inputTypes1.emplace_back(htmlUtdId_);
    std::shared_ptr<EntryGetter> entryGetter1 = std::make_shared<EntryGetterImpl>();
    inputRecord1->SetEntryGetter(inputTypes1, entryGetter1);
    inputData.AddRecord(inputRecord1);
    std::shared_ptr<UnifiedRecord> inputRecord2 = std::make_shared<UnifiedRecord>();
    std::vector<std::string> inputTypes2;
    inputTypes2.emplace_back(fileUriUtdId_);
    inputTypes2.emplace_back(pixelMapUtdId_);
    std::shared_ptr<EntryGetter> entryGetter2 = std::make_shared<EntryGetterImpl>();
    inputRecord2->SetEntryGetter(inputTypes2, entryGetter2);
    inputData.AddRecord(inputRecord2);
    std::shared_ptr<UnifiedRecord> inputRecord3 = std::make_shared<UnifiedRecord>();
    std::vector<std::string> inputTypes3;
    inputTypes3.emplace_back(linkUtdId_);
    std::shared_ptr<EntryGetter> entryGetter3 = std::make_shared<EntryGetterImpl>();
    inputRecord3->SetEntryGetter(inputTypes3, entryGetter3);
    inputData.AddRecord(inputRecord3);
    auto status = PasteboardClient::GetInstance()->SetUdsdData(inputData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    UnifiedData outputData;
    status = PasteboardClient::GetInstance()->GetUdsdData(outputData);
    ASSERT_EQ(status, static_cast<int32_t>(PasteboardError::E_OK));
    auto inputRecords = inputData.GetRecords();
    auto outputRecords = outputData.GetRecords();
    ASSERT_EQ(outputRecords.size(), inputRecords.size());
    auto outputRecord1 = outputData.GetRecordAt(0);
    ASSERT_NE(outputRecord1, nullptr);
    auto outputTypes1 = outputRecord1->GetUtdIds();
    ASSERT_EQ(outputTypes1.size(), inputTypes1.size());
    ASSERT_TRUE(outputTypes1.find(plainTextUtdId_) != outputTypes1.end());
    CheckPlainUds(outputRecord1->GetEntry(plainTextUtdId_));
    ASSERT_TRUE(outputTypes1.find(htmlUtdId_) != outputTypes1.end());
    CheckHtmlUds(outputRecord1->GetEntry(htmlUtdId_));
    auto outputRecord2 = outputData.GetRecordAt(1);
    ASSERT_NE(outputRecord2, nullptr);
    auto outputTypes2 = outputRecord2->GetUtdIds();
    ASSERT_GE(outputTypes2.size(), inputTypes2.size());
    ASSERT_TRUE(outputTypes2.find(fileUriUtdId_) != outputTypes2.end());
    CheckFileUriUds(outputRecord2->GetEntry(fileUriUtdId_));
    ASSERT_TRUE(outputTypes2.find(pixelMapUtdId_) != outputTypes2.end());
    CheckPixelMapUds(outputRecord2->GetEntry(pixelMapUtdId_));
    auto outputRecord3 = outputData.GetRecordAt(2);
    ASSERT_NE(outputRecord3, nullptr);
    auto outputTypes3 = outputRecord3->GetUtdIds();
    ASSERT_EQ(outputTypes3.size(), inputTypes3.size());
    ASSERT_TRUE(outputTypes3.find(linkUtdId_) != outputTypes3.end());
    CheckLinkUds(outputRecord3->GetEntry(linkUtdId_));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "SetMultiTypeUnifiedDataDelayTest003 end");
}
} // namespace OHOS::MiscServices
