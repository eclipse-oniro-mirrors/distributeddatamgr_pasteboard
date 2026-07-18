/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "parcel.h"
#include "paste_data_info.h"
#include "pasteboard_hilog.h"

namespace OHOS::MiscServices {
using namespace testing::ext;

namespace {
constexpr int64_t TEST_RAW_DATA_SIZE = 0x100000000; // 4GB, exceeds INT32_MAX to verify no truncation
constexpr int32_t TEST_TEXT_DATA_SIZE = 200;
constexpr int32_t TEST_HTML_DATA_SIZE = 300;
constexpr uint32_t FIELD_NUM_BEFORE_MIME_SIZE = 5; // rawDataSize/textDataSize/htmlDataSize/two bools
} // namespace

class PasteDataInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static PasteDataInfo BuildPasteDataInfo();
};

void PasteDataInfoTest::SetUpTestCase(void) { }

void PasteDataInfoTest::TearDownTestCase(void) { }

void PasteDataInfoTest::SetUp(void) { }

void PasteDataInfoTest::TearDown(void) { }

PasteDataInfo PasteDataInfoTest::BuildPasteDataInfo()
{
    PasteDataInfo info;
    info.rawDataSize = TEST_RAW_DATA_SIZE;
    info.textDataSize = TEST_TEXT_DATA_SIZE;
    info.htmlDataSize = TEST_HTML_DATA_SIZE;
    info.isDelayedData = true;
    info.isDelayedRecord = true;
    info.mimeTypes = { "text/plain", "text/html" };
    return info;
}

/**
 * @tc.name: ConstructTest001
 * @tc.desc: default constructor should init all fields
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, ConstructTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ConstructTest001 start");
    PasteDataInfo info;
    EXPECT_EQ(info.rawDataSize, 0);
    EXPECT_EQ(info.textDataSize, 0);
    EXPECT_EQ(info.htmlDataSize, 0);
    EXPECT_EQ(info.isDelayedData, false);
    EXPECT_EQ(info.isDelayedRecord, false);
    EXPECT_TRUE(info.mimeTypes.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "ConstructTest001 end");
}

/**
 * @tc.name: CopyConstructTest001
 * @tc.desc: copy constructor should copy all fields
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, CopyConstructTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CopyConstructTest001 start");
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo copyInfo(info);
    EXPECT_EQ(copyInfo.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(copyInfo.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(copyInfo.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(copyInfo.isDelayedData, true);
    EXPECT_EQ(copyInfo.isDelayedRecord, true);
    EXPECT_EQ(copyInfo.mimeTypes, info.mimeTypes);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "CopyConstructTest001 end");
}

/**
 * @tc.name: OperatorAssignTest001
 * @tc.desc: assignment operator should copy all fields
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, OperatorAssignTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignTest001 start");
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo assignInfo;
    assignInfo = info;
    EXPECT_EQ(assignInfo.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(assignInfo.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(assignInfo.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(assignInfo.isDelayedData, true);
    EXPECT_EQ(assignInfo.isDelayedRecord, true);
    EXPECT_EQ(assignInfo.mimeTypes, info.mimeTypes);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignTest001 end");
}

/**
 * @tc.name: OperatorAssignTest002
 * @tc.desc: self assignment should keep all fields unchanged
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, OperatorAssignTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignTest002 start");
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo &selfRef = info;
    info = selfRef;
    EXPECT_EQ(info.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(info.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(info.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(info.isDelayedData, true);
    EXPECT_EQ(info.isDelayedRecord, true);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignTest002 end");
}

/**
 * @tc.name: MarshallingTest001
 * @tc.desc: marshalling and unmarshalling should keep all fields
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest001 start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));

    PasteDataInfo *newInfo = PasteDataInfo::Unmarshalling(parcel);
    ASSERT_NE(newInfo, nullptr);
    EXPECT_EQ(newInfo->rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(newInfo->textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(newInfo->htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(newInfo->isDelayedData, true);
    EXPECT_EQ(newInfo->isDelayedRecord, true);
    EXPECT_EQ(newInfo->mimeTypes, info.mimeTypes);
    delete newInfo;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest001 end");
}

/**
 * @tc.name: MarshallingTest002
 * @tc.desc: marshalling and unmarshalling with empty mimeTypes
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest002 start");
    PasteDataInfo info;
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));

    PasteDataInfo *newInfo = PasteDataInfo::Unmarshalling(parcel);
    ASSERT_NE(newInfo, nullptr);
    EXPECT_EQ(newInfo->rawDataSize, 0);
    EXPECT_EQ(newInfo->isDelayedData, false);
    EXPECT_EQ(newInfo->isDelayedRecord, false);
    EXPECT_TRUE(newInfo->mimeTypes.empty());
    delete newInfo;
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest002 end");
}

/**
 * @tc.name: MarshallingTest003
 * @tc.desc: marshalling should fail when parcel has no enough space for each field
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTest003, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest003 start");
    PasteDataInfo info = BuildPasteDataInfo();
    // rawDataSize takes 8 bytes, each int32/bool/uint32 field takes 4 bytes,
    // reserve free space to make every write branch fail in turn
    std::vector<size_t> freeSizes = { 0, 8, 12, 16, 20, 24, 28 };
    for (size_t freeSize : freeSizes) {
        Parcel parcel;
        std::vector<uint8_t> filler(parcel.GetMaxCapacity() - freeSize, 0);
        EXPECT_TRUE(parcel.WriteBuffer(filler.data(), filler.size()));
        EXPECT_FALSE(info.Marshalling(parcel));
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTest003 end");
}

/**
 * @tc.name: UnmarshallingTest001
 * @tc.desc: unmarshalling should return nullptr when parcel data is insufficient
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingTest001 start");
    // write 0~5 fields to make every read branch fail in turn, rawDataSize is int64 and the rest are 4-byte fields
    for (uint32_t fieldNum = 0; fieldNum <= FIELD_NUM_BEFORE_MIME_SIZE; ++fieldNum) {
        Parcel parcel;
        if (fieldNum > 0) {
            EXPECT_TRUE(parcel.WriteInt64(0));
        }
        for (uint32_t i = 1; i < fieldNum; ++i) {
            EXPECT_TRUE(parcel.WriteInt32(0));
        }
        PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
        EXPECT_EQ(info, nullptr);
    }
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingTest001 end");
}

/**
 * @tc.name: UnmarshallingTest002
 * @tc.desc: unmarshalling should return nullptr when mimeType string is missing
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingTest002, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingTest002 start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    for (uint32_t i = 1; i < FIELD_NUM_BEFORE_MIME_SIZE; ++i) {
        EXPECT_TRUE(parcel.WriteInt32(0));
    }
    // declare one mimeType but do not write it, ReadString should fail
    EXPECT_TRUE(parcel.WriteUint32(1));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingTest002 end");
}
} // namespace OHOS::MiscServices
