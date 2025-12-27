/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "long_wrapper.h"
#include "pasteboard_client.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace testing::ext;
using namespace OHOS::AAFwk;
class TLVObjectTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static std::shared_ptr<PasteDataRecord> GenRecord(std::uint32_t index);
};

void TLVObjectTest::SetUpTestCase(void) { }

void TLVObjectTest::TearDownTestCase(void) { }

void TLVObjectTest::SetUp(void) { }

void TLVObjectTest::TearDown(void) { }

std::shared_ptr<PasteDataRecord> TLVObjectTest::GenRecord(std::uint32_t index)
{
    std::string indexStr = "";
    auto plainText = std::make_shared<std::string>("hello" + indexStr);
    auto htmlText = std::make_shared<std::string>("<span>hello" + indexStr + "</span>");
    auto uri = std::make_shared<OHOS::Uri>("dataability://hello" + indexStr + ".txt");
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string key = "id";
    int32_t id = 456;
    want->SetParam(key, id);

    PasteDataRecord::Builder builder(MIMETYPE_TEXT_HTML);
    auto record1 = builder.SetPlainText(plainText).SetHtmlText(htmlText).SetUri(uri).SetWant(want).Build();
    return record1;
}

/**
 * @tc.name: TLVOjbectTest001
 * @tc.desc: test tlv coder.
 * @tc.type: FUNC
 * @tc.require:AR000H5I1D
 * @tc.author: baoyayong
 */
HWTEST_F(TLVObjectTest, TLVOjbectTest001, TestSize.Level0)
{
    PasteData pasteData1;
    for (uint32_t i = 0; i < 10; ++i) {
        pasteData1.AddRecord(TLVObjectTest::GenRecord(i));
    }

    std::vector<uint8_t> buffer;
    auto ret = pasteData1.Encode(buffer);
    ASSERT_TRUE(ret);
    ASSERT_EQ(buffer.size(), pasteData1.CountTLV());

    PasteData pasteData2;
    ret = pasteData2.Decode(buffer);
    EXPECT_TRUE(ret);
    EXPECT_EQ(pasteData2.GetRecordCount(), pasteData1.GetRecordCount());

    for (uint32_t i = 0; i < 10; ++i) {
        auto record2 = pasteData2.GetRecordAt(i);
        auto record1 = pasteData1.GetRecordAt(i);
        EXPECT_EQ(*(record2->GetHtmlTextV0()), *(record1->GetHtmlTextV0()));
        EXPECT_EQ(*(record2->GetPlainTextV0()), *(record1->GetPlainTextV0()));
        EXPECT_TRUE(record2->GetUriV0()->ToString() == record1->GetUriV0()->ToString());
        EXPECT_EQ(record2->GetWant()->OperationEquals(*(record1->GetWant())), true);
    }
}

/**
 * @tc.name: TLVOjbectTest002
 * @tc.desc: test tlv coder.
 * @tc.type: FUNC
 * @tc.require:AR000H5I1D
 * @tc.author: baoyayong
 */
HWTEST_F(TLVObjectTest, TLVOjbectTest002, TestSize.Level0)
{
    std::shared_ptr<Want> want = std::make_shared<Want>();
    std::string key = "id";
    int32_t id = 456;
    Want wantIn = want->SetParam(key, id);
    auto pasteData1 = PasteboardClient::GetInstance()->CreateWantData(std::make_shared<Want>(wantIn));

    std::vector<uint8_t> buffer;
    auto ret = pasteData1->Encode(buffer);
    ASSERT_TRUE(ret);
    ASSERT_EQ(buffer.size(), pasteData1->CountTLV());

    PasteData pasteData2;
    ret = pasteData2.Decode(buffer);
    EXPECT_TRUE(ret);
    EXPECT_EQ(pasteData2.GetRecordCount(), pasteData1->GetRecordCount());

    auto record2 = pasteData2.GetRecordAt(0);
    auto record1 = pasteData1->GetRecordAt(0);
    EXPECT_EQ(record2->GetWant()->OperationEquals(*(record1->GetWant())), true);
}

/**
 * @tc.name: TLVOjbectTest003
 * @tc.desc: test tlv coder map.
 * @tc.type: FUNC
 * @tc.require:AR000H5I1D
 * @tc.author: baoyayong
 */
HWTEST_F(TLVObjectTest, TLVOjbectTest003, TestSize.Level0)
{
    std::string plainText = "plain text";
    auto pasteData1 = PasteboardClient::GetInstance()->CreatePlainTextData(plainText);
    std::vector<uint8_t> arrayBuffer(46);
    arrayBuffer = { 2, 7, 6, 8, 9 };
    std::string mimeType1 = "image/jpg";
    std::shared_ptr<MineCustomData> customData = std::make_shared<MineCustomData>();
    customData->AddItemData(mimeType1, arrayBuffer);
    PasteDataRecord::Builder builder(MIMETYPE_TEXT_PLAIN);
    std::shared_ptr<PasteDataRecord> pasteDataRecord = builder.SetCustomData(customData).Build();
    pasteData1->AddRecord(pasteDataRecord);

    std::vector<uint8_t> buffer;
    auto ret = pasteData1->Encode(buffer);
    ASSERT_TRUE(ret);
    ASSERT_EQ(buffer.size(), pasteData1->CountTLV());

    PasteData pasteData2;
    ret = pasteData2.Decode(buffer);
    EXPECT_TRUE(ret);
    EXPECT_EQ(pasteData2.GetRecordCount(), pasteData1->GetRecordCount());
    EXPECT_EQ(pasteData2.GetRecordCount(), pasteData1->GetRecordCount());

    auto record1 = pasteData1->GetRecordAt(0);
    auto record2 = pasteData2.GetRecordAt(0);
    ASSERT_TRUE(record2 != nullptr);
    auto custom2 = record2->GetCustomData();
    auto custom1 = record1->GetCustomData();
    ASSERT_TRUE(custom1 != nullptr && custom2 != nullptr);
    EXPECT_EQ(custom2->GetItemData().size(), custom1->GetItemData().size());
}

/**
 * @tc.name: TestPasteDataProperty
 * @tc.desc: PasteDataProperty
 * @tc.type: FUNC
 */
HWTEST_F(TLVObjectTest, TestPasteDataProperty, TestSize.Level0)
{
    PasteDataProperty obj1;
    obj1.mimeTypes = {
        MIMETYPE_PIXELMAP, MIMETYPE_TEXT_HTML, MIMETYPE_TEXT_PLAIN, MIMETYPE_TEXT_URI, MIMETYPE_TEXT_WANT,
    };
    obj1.additions.SetParam("key", AAFwk::Long::Box(1));
    obj1.tag = "tag";
    obj1.timestamp = 1;
    obj1.localOnly = true;
    obj1.shareOption = ShareOption::CrossDevice;
    obj1.tokenId = 1;
    obj1.isRemote = true;
    obj1.bundleName = "bundleName";
    obj1.setTime = "setTime";
    obj1.screenStatus = ScreenEvent::ScreenUnlocked;

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer);
    ASSERT_TRUE(ret);

    PasteDataProperty obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_TRUE(obj1.additions == obj2.additions);
    EXPECT_EQ(obj1.mimeTypes, obj2.mimeTypes);
    EXPECT_EQ(obj1.tag, obj2.tag);
    EXPECT_EQ(obj1.timestamp, obj2.timestamp);
    EXPECT_EQ(obj1.localOnly, obj2.localOnly);
    EXPECT_EQ(obj1.shareOption, obj2.shareOption);
    EXPECT_EQ(obj1.tokenId, obj2.tokenId);
    EXPECT_EQ(obj1.isRemote, obj2.isRemote);
    EXPECT_EQ(obj1.bundleName, obj2.bundleName);
    EXPECT_EQ(obj1.setTime, obj2.setTime);
    EXPECT_EQ(obj1.screenStatus, obj2.screenStatus);
}

/**
 * @tc.name: TestMineCustomData
 * @tc.desc: MineCustomData
 * @tc.type: FUNC
 */
HWTEST_F(TLVObjectTest, TestMineCustomData, TestSize.Level0)
{
    MineCustomData obj1;
    obj1.AddItemData("key", {1, 1, 1, 1});

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer);
    ASSERT_TRUE(ret);

    MineCustomData obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetItemData(), obj2.GetItemData());
}

/**
 * @tc.name: TestPasteDataEntry
 * @tc.desc: PasteDataEntry
 * @tc.type: FUNC
 */
HWTEST_F(TLVObjectTest, TestPasteDataEntry, TestSize.Level0)
{
    PasteDataEntry obj1;
    obj1.SetUtdId("utdId");
    obj1.SetMimeType("mimeType");
    auto udmfObject = std::make_shared<Object>();
    udmfObject->value_["key"] = "value";
    obj1.SetValue(udmfObject);

    std::vector<uint8_t> buffer;
    bool ret = obj1.Encode(buffer);
    ASSERT_TRUE(ret);

    PasteDataEntry obj2;
    ret = obj2.Decode(buffer);
    ASSERT_TRUE(ret);

    EXPECT_EQ(obj1.GetUtdId(), obj2.GetUtdId());
    EXPECT_EQ(obj1.GetMimeType(), obj2.GetMimeType());

    auto entryValue = obj2.GetValue();
    ASSERT_TRUE(std::holds_alternative<std::shared_ptr<Object>>(entryValue));
    auto udmfObject2 = std::get<std::shared_ptr<Object>>(entryValue);
    ASSERT_NE(udmfObject2, nullptr);
    EXPECT_EQ(udmfObject->value_, udmfObject2->value_);
}

/**
 * @tc.name: TestPasteDataRecord
 * @tc.desc: PasteDataRecord
 * @tc.type: FUNC
 */
HWTEST_F(TLVObjectTest, TestPasteDataRecord, TestSize.Level0)
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
    bool ret = obj1.Encode(buffer);
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
 * @tc.name: TestRecursiveGuard
 * @tc.desc: should decode failed when recursive overflow
 * @tc.type: FUNC
 */
HWTEST_F(TLVObjectTest, TestRecursiveGuard, TestSize.Level0)
{
    constexpr int32_t RECURSIVE_DEPTH = 15;
    std::shared_ptr<Object> objects[RECURSIVE_DEPTH];
    for (int32_t i = 0; i < RECURSIVE_DEPTH; ++i) {
        objects[i] = std::make_shared<Object>();
    }
    int32_t index = RECURSIVE_DEPTH - 1;
    objects[index]->value_["key"] = "value";
    for (--index; index >= 0; --index) {
        objects[index]->value_["key"] = objects[index + 1];
    }

    PasteData pasteData;
    PasteDataRecord record;
    std::shared_ptr<PasteDataEntry> entry = std::make_shared<PasteDataEntry>();
    entry->SetValue(objects[0]);
    record.AddEntryByMimeType("customType", entry);
    pasteData.AddRecord(record);

    std::vector<uint8_t> buffer;
    bool encodeSuccess = pasteData.Encode(buffer);
    EXPECT_TRUE(encodeSuccess);

    PasteData pasteData2;
    bool decodeSuccess = pasteData2.Decode(buffer);
    EXPECT_FALSE(decodeSuccess);
}
} // namespace OHOS::MiscServices
