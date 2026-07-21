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
// free space left in a full parcel so that the next field to be marshalled has no room and fails.
// rawDataSize is int64 (8 bytes), the following textDataSize/htmlDataSize/isDelayedData/
// isDelayedRecord/mimeTypesSize each take 4 bytes.
constexpr size_t FREE_FOR_RAW_DATA_SIZE = 0;
constexpr size_t FREE_FOR_TEXT_DATA_SIZE = 8;
constexpr size_t FREE_FOR_HTML_DATA_SIZE = 12;
constexpr size_t FREE_FOR_IS_DELAYED_DATA = 16;
constexpr size_t FREE_FOR_IS_DELAYED_RECORD = 20;
constexpr size_t FREE_FOR_MIME_TYPES_SIZE = 24;
constexpr size_t FREE_FOR_MIME_TYPE_STRING = 28;
} // namespace

class PasteDataInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static PasteDataInfo BuildPasteDataInfo();
    // fill the parcel so that only freeSize bytes remain writable, making the next write fail.
    static void ReserveParcelFreeSpace(Parcel &parcel, size_t freeSize);
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

void PasteDataInfoTest::ReserveParcelFreeSpace(Parcel &parcel, size_t freeSize)
{
    std::vector<uint8_t> filler(parcel.GetMaxCapacity() - freeSize, 0);
    EXPECT_TRUE(parcel.WriteBuffer(filler.data(), filler.size()));
}

/**
 * @tc.name: DefaultConstructTest001
 * @tc.desc: default constructor should init all fields
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, DefaultConstructTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DefaultConstructTest001 start");
    PasteDataInfo info;
    EXPECT_EQ(info.rawDataSize, 0);
    EXPECT_EQ(info.textDataSize, 0);
    EXPECT_EQ(info.htmlDataSize, 0);
    EXPECT_EQ(info.isDelayedData, false);
    EXPECT_EQ(info.isDelayedRecord, false);
    EXPECT_TRUE(info.mimeTypes.empty());
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "DefaultConstructTest001 end");
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
 * @tc.desc: assignment operator on different objects (this != &dataInfo) should copy all fields
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
 * @tc.name: OperatorAssignSelfTest001
 * @tc.desc: self assignment (this == &dataInfo) should hit the early return and keep fields unchanged
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, OperatorAssignSelfTest001, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignSelfTest001 start");
    const std::vector<std::string> expectMimeTypes = { "text/plain", "text/html" };
    PasteDataInfo info = BuildPasteDataInfo();
    PasteDataInfo &selfRef = info;
    info = selfRef;
    EXPECT_EQ(info.rawDataSize, TEST_RAW_DATA_SIZE);
    EXPECT_EQ(info.textDataSize, TEST_TEXT_DATA_SIZE);
    EXPECT_EQ(info.htmlDataSize, TEST_HTML_DATA_SIZE);
    EXPECT_EQ(info.isDelayedData, true);
    EXPECT_EQ(info.isDelayedRecord, true);
    EXPECT_EQ(info.mimeTypes, expectMimeTypes);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "OperatorAssignSelfTest001 end");
}

/**
 * @tc.name: MarshallingTest001
 * @tc.desc: marshalling and unmarshalling should keep all fields (with mimeTypes)
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
 * @tc.desc: marshalling and unmarshalling with empty mimeTypes (skip the mimeType write loop)
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
 * @tc.name: MarshallingRawDataSizeFailTest
 * @tc.desc: marshalling should fail when WriteInt64(rawDataSize) has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingRawDataSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingRawDataSizeFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_RAW_DATA_SIZE);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingRawDataSizeFailTest end");
}

/**
 * @tc.name: MarshallingTextDataSizeFailTest
 * @tc.desc: marshalling should fail when WriteInt32(textDataSize) has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingTextDataSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTextDataSizeFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_TEXT_DATA_SIZE);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingTextDataSizeFailTest end");
}

/**
 * @tc.name: MarshallingHtmlDataSizeFailTest
 * @tc.desc: marshalling should fail when WriteInt32(htmlDataSize) has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingHtmlDataSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingHtmlDataSizeFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_HTML_DATA_SIZE);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingHtmlDataSizeFailTest end");
}

/**
 * @tc.name: MarshallingIsDelayedDataFailTest
 * @tc.desc: marshalling should fail when WriteBool(isDelayedData) has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingIsDelayedDataFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingIsDelayedDataFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_IS_DELAYED_DATA);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingIsDelayedDataFailTest end");
}

/**
 * @tc.name: MarshallingIsDelayedRecordFailTest
 * @tc.desc: marshalling should fail when WriteBool(isDelayedRecord) has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingIsDelayedRecordFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingIsDelayedRecordFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_IS_DELAYED_RECORD);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingIsDelayedRecordFailTest end");
}

/**
 * @tc.name: MarshallingMimeTypesSizeFailTest
 * @tc.desc: marshalling should fail when WriteUint32(mimeTypesSize) has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingMimeTypesSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingMimeTypesSizeFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_MIME_TYPES_SIZE);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingMimeTypesSizeFailTest end");
}

/**
 * @tc.name: MarshallingMimeTypeStringFailTest
 * @tc.desc: marshalling should fail when WriteString(mimeType) in the loop has no room
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, MarshallingMimeTypeStringFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingMimeTypeStringFailTest start");
    PasteDataInfo info = BuildPasteDataInfo();
    Parcel parcel;
    ReserveParcelFreeSpace(parcel, FREE_FOR_MIME_TYPE_STRING);
    EXPECT_FALSE(info.Marshalling(parcel));
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "MarshallingMimeTypeStringFailTest end");
}

/**
 * @tc.name: UnmarshallingRawDataSizeFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadInt64(rawDataSize) fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingRawDataSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingRawDataSizeFailTest start");
    Parcel parcel;
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingRawDataSizeFailTest end");
}

/**
 * @tc.name: UnmarshallingTextDataSizeFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadInt32(textDataSize) fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingTextDataSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingTextDataSizeFailTest start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingTextDataSizeFailTest end");
}

/**
 * @tc.name: UnmarshallingHtmlDataSizeFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadInt32(htmlDataSize) fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingHtmlDataSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingHtmlDataSizeFailTest start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingHtmlDataSizeFailTest end");
}

/**
 * @tc.name: UnmarshallingIsDelayedDataFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadBool(isDelayedData) fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingIsDelayedDataFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingIsDelayedDataFailTest start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingIsDelayedDataFailTest end");
}

/**
 * @tc.name: UnmarshallingIsDelayedRecordFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadBool(isDelayedRecord) fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingIsDelayedRecordFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingIsDelayedRecordFailTest start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteBool(false));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingIsDelayedRecordFailTest end");
}

/**
 * @tc.name: UnmarshallingMimeTypesSizeFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadUint32(mimeTypesSize) fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingMimeTypesSizeFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingMimeTypesSizeFailTest start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteBool(false));
    EXPECT_TRUE(parcel.WriteBool(false));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingMimeTypesSizeFailTest end");
}

/**
 * @tc.name: UnmarshallingMimeTypeStringFailTest
 * @tc.desc: unmarshalling should return nullptr when ReadString(mimeType) in the loop fails
 * @tc.type: FUNC
 */
HWTEST_F(PasteDataInfoTest, UnmarshallingMimeTypeStringFailTest, TestSize.Level0)
{
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingMimeTypeStringFailTest start");
    Parcel parcel;
    EXPECT_TRUE(parcel.WriteInt64(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteInt32(0));
    EXPECT_TRUE(parcel.WriteBool(false));
    EXPECT_TRUE(parcel.WriteBool(false));
    // declare one mimeType but do not write it, ReadString should fail
    EXPECT_TRUE(parcel.WriteUint32(1));
    PasteDataInfo *info = PasteDataInfo::Unmarshalling(parcel);
    EXPECT_EQ(info, nullptr);
    PASTEBOARD_HILOGI(PASTEBOARD_MODULE_CLIENT, "UnmarshallingMimeTypeStringFailTest end");
}
} // namespace OHOS::MiscServices
