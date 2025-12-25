/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "message_parcel_warp.h"
#include "pasteboard_delay_manager.h"
#include "pasteboard_error.h"
#include "pasteboard_hilog.h"
#include "pasteboard_service.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

const std::string UTDID_PLAIN_TEXT = "general.plain-text";
const std::string UTDID_HYPERLINK = "general.hyperlink";
const std::string UTDID_HTML = "general.html";
const std::string UTDID_FILE_URI = "general.file-uri";
const std::string UTDID_PIXEL_MAP = "openharmony.pixel-map";

std::shared_mutex PasteboardService::pasteDataMutex_;

class PasteboardDelayManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PasteboardDelayManagerTest::SetUpTestCase()
{
}

void PasteboardDelayManagerTest::TearDownTestCase()
{
}

void PasteboardDelayManagerTest::SetUp()
{
}

void PasteboardDelayManagerTest::TearDown()
{
}

class EntryGetterImpl : public IPasteboardEntryGetter {
public:
    int32_t GetRecordValueByType(uint32_t recordId, PasteDataEntry &entry) override
    {
        constexpr uint32_t recordId1 = 1;
        constexpr uint32_t recordId2 = 2;
        constexpr uint64_t entrySize1 = 1000;
        if (recordId == recordId1) {
            return static_cast<int32_t>(PasteboardError::INVALID_RECORD_ID);
        }
        if (recordId == recordId2) {
            entry.rawDataSize_ = entrySize1;
            return static_cast<int32_t>(PasteboardError::E_OK);
        }
        uint64_t entrySize2 = MessageParcelWarp::GetRawDataSize() - 1;
        entry.rawDataSize_ = entrySize2;
        return static_cast<int32_t>(PasteboardError::E_OK);
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }
};

/**
 * @tc.name: GetEntryPriorityTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayManagerTest, GetEntryPriorityTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetEntryPriorityTest001 start");
    uint8_t priority = DelayManager::GetEntryPriority(UTDID_PLAIN_TEXT);
    EXPECT_EQ(priority, 1);

    priority = DelayManager::GetEntryPriority(UTDID_HYPERLINK);
    EXPECT_EQ(priority, 2);

    priority = DelayManager::GetEntryPriority(UTDID_HTML);
    EXPECT_EQ(priority, 3);

    priority = DelayManager::GetEntryPriority(UTDID_FILE_URI);
    EXPECT_EQ(priority, 4);

    priority = DelayManager::GetEntryPriority(UTDID_PIXEL_MAP);
    EXPECT_EQ(priority, 5);

    priority = DelayManager::GetEntryPriority("customType");
    EXPECT_EQ(priority, 255);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetEntryPriorityTest001 end");
}

/**
 * @tc.name: GetAllDelayEntryInfoTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayManagerTest, GetAllDelayEntryInfoTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllDelayEntryInfoTest001 start");
    PasteData pasteData;
    auto infoList = DelayManager::GetAllDelayEntryInfo(pasteData);
    EXPECT_EQ(infoList.size(), 0);

    PasteDataRecord record;
    auto entry = std::make_shared<PasteDataEntry>();
    entry->SetUtdId(UTDID_PLAIN_TEXT);
    record.AddEntry(UTDID_PLAIN_TEXT, entry);
    pasteData.AddRecord(record);
    infoList = DelayManager::GetAllDelayEntryInfo(pasteData);
    EXPECT_EQ(infoList.size(), 0);

    pasteData.records_[0]->SetDelayRecordFlag(true);
    infoList = DelayManager::GetAllDelayEntryInfo(pasteData);
    EXPECT_EQ(infoList.size(), 1);

    pasteData.records_[0]->entries_ = {nullptr};
    infoList = DelayManager::GetAllDelayEntryInfo(pasteData);
    EXPECT_EQ(infoList.size(), 0);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllDelayEntryInfoTest001 end");
}

/**
 * @tc.name: GetAllDelayEntryInfoTest002
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayManagerTest, GetAllDelayEntryInfoTest002, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllDelayEntryInfoTest002 start");
    PasteData pasteData;
    PasteDataRecord record;
    record.SetDelayRecordFlag(true);

    auto entryText = std::make_shared<PasteDataEntry>();
    entryText->SetUtdId(UTDID_PLAIN_TEXT);
    record.AddEntry(UTDID_PLAIN_TEXT, entryText);

    auto entryPixelMap = std::make_shared<PasteDataEntry>();
    entryPixelMap->SetUtdId(UTDID_PIXEL_MAP);
    record.AddEntry(UTDID_PIXEL_MAP, entryPixelMap);

    auto entryHtml = std::make_shared<PasteDataEntry>();
    entryHtml->SetUtdId(UTDID_HTML);
    record.AddEntry(UTDID_HTML, entryHtml);

    auto entryUri = std::make_shared<PasteDataEntry>();
    entryUri->SetUtdId(UTDID_FILE_URI);
    record.AddEntry(UTDID_FILE_URI, entryUri);

    auto entryLink = std::make_shared<PasteDataEntry>();
    entryLink->SetUtdId(UTDID_HYPERLINK);
    record.AddEntry(UTDID_HYPERLINK, entryLink);

    auto entryCustom = std::make_shared<PasteDataEntry>();
    entryCustom->SetUtdId("customType");
    record.AddEntry("customType", entryCustom);

    pasteData.AddRecord(record);
    auto infoList = DelayManager::GetAllDelayEntryInfo(pasteData);
    EXPECT_EQ(infoList.size(), 6);
    
    std::string expectUtdid[] = {
        UTDID_PLAIN_TEXT, UTDID_HYPERLINK, UTDID_HTML, UTDID_FILE_URI, UTDID_PIXEL_MAP, "customType"
    };
    for (uint32_t i = 0; i < infoList.size(); ++i) {
        EXPECT_EQ(infoList[i].recordId, 1);
        EXPECT_STREQ(infoList[i].entry->GetUtdId().c_str(), expectUtdid[i].c_str());
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetAllDelayEntryInfoTest002 end");
}

/**
 * @tc.name: GetPrimaryDelayEntryInfoTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayManagerTest, GetPrimaryDelayEntryInfoTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPrimaryDelayEntryInfoTest001 start");
    PasteData pasteData;
    PasteDataRecord record1;
    record1.SetDelayRecordFlag(true);
    PasteDataRecord record2;
    record2.SetDelayRecordFlag(true);

    auto entryText = std::make_shared<PasteDataEntry>();
    entryText->SetUtdId(UTDID_PLAIN_TEXT);
    record1.AddEntry(UTDID_PLAIN_TEXT, entryText);

    auto entryPixelMap = std::make_shared<PasteDataEntry>();
    entryPixelMap->SetUtdId(UTDID_PIXEL_MAP);
    record1.AddEntry(UTDID_PIXEL_MAP, entryPixelMap);

    auto entryHtml = std::make_shared<PasteDataEntry>();
    entryHtml->SetUtdId(UTDID_HTML);
    record1.AddEntry(UTDID_HTML, entryHtml);

    auto entryUri = std::make_shared<PasteDataEntry>();
    entryUri->SetUtdId(UTDID_FILE_URI);
    record2.AddEntry(UTDID_FILE_URI, entryUri);

    auto entryLink = std::make_shared<PasteDataEntry>();
    entryLink->SetUtdId(UTDID_HYPERLINK);
    record2.AddEntry(UTDID_HYPERLINK, entryLink);

    auto entryCustom = std::make_shared<PasteDataEntry>();
    entryCustom->SetUtdId("customType");
    record2.AddEntry("customType", entryCustom);

    pasteData.AddRecord(record1);
    pasteData.AddRecord(record2);
    auto infoList = DelayManager::GetPrimaryDelayEntryInfo(pasteData);
    EXPECT_EQ(infoList.size(), 2);
    EXPECT_EQ(infoList[0].recordId, 1);
    EXPECT_STREQ(infoList[0].entry->GetUtdId().c_str(), UTDID_PLAIN_TEXT.c_str());
    EXPECT_EQ(infoList[1].recordId, 2);
    EXPECT_STREQ(infoList[1].entry->GetUtdId().c_str(), UTDID_FILE_URI.c_str());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetPrimaryDelayEntryInfoTest001 end");
}

/**
 * @tc.name: GetLocalEntryValueTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(PasteboardDelayManagerTest, GetLocalEntryValueTest001, TestSize.Level1)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalEntryValueTest001 start");
    uint64_t initDataSize = 100;
    uint64_t finalDataSize = 1100;
    std::vector<DelayEntryInfo> delayEntryInfos = {};
    sptr<IPasteboardEntryGetter> entryGetter = nullptr;
    PasteData pasteData;
    pasteData.rawDataSize_ = initDataSize;
    DelayManager::GetLocalEntryValue(delayEntryInfos, entryGetter, pasteData);
    EXPECT_EQ(pasteData.rawDataSize_, initDataSize);

    entryGetter = sptr<EntryGetterImpl>::MakeSptr();
    DelayManager::GetLocalEntryValue(delayEntryInfos, entryGetter, pasteData);
    EXPECT_EQ(pasteData.rawDataSize_, initDataSize);

    auto entry1 = std::make_shared<PasteDataEntry>();
    auto entry2 = std::make_shared<PasteDataEntry>();
    auto entry3 = std::make_shared<PasteDataEntry>();
    auto entry4 = std::make_shared<PasteDataEntry>();
    entry4->SetValue("text content");
    std::vector<std::shared_ptr<PasteDataEntry>> entryList = {entry1, entry2, entry3, entry4};
    for (uint32_t i = 0; i < entryList.size(); ++i) {
        entryList[i]->SetUtdId(UTDID_PLAIN_TEXT);
        delayEntryInfos.push_back({1, i + 1, entryList[i]});
    }

    DelayManager::GetLocalEntryValue(delayEntryInfos, entryGetter, pasteData);
    EXPECT_EQ(pasteData.rawDataSize_, finalDataSize);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_SERVICE, "GetLocalEntryValueTest001 end");
}
} // namespace OHOS::MiscServices
